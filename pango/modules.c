/* Pango
 * modules.c:
 *
 * Copyright (C) 1999 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <limits.h>
#include <errno.h>

#include <gmodule.h>
#include <glib/gstdio.h>

#include "pango-enum-types.h"
#include "pango-modules.h"
#include "pango-impl-utils.h"
#include "modules.h"

typedef struct _PangoModule      PangoModule;
typedef struct _PangoModuleClass PangoModuleClass;

#define PANGO_TYPE_MODULE           (pango_module_get_type ())
#define PANGO_MODULE(module) (G_TYPE_CHECK_INSTANCE_CAST ((module), PANGO_TYPE_MODULE, PangoModule))
#define PANGO_IS_MODULE(module)  (G_TYPE_CHECK_INSTANCE_TYPE ((module), PANGO_TYPE_MODULE))

typedef struct _PangoMapInfo PangoMapInfo;
typedef struct _PangoEnginePair PangoEnginePair;
typedef struct _PangoSubmap PangoSubmap;

struct _PangoMap
{
  GArray *entries;
};

struct _PangoMapEntry
{
  GSList *exact;
  GSList *fallback;
};

struct _PangoMapInfo
{
  PangoLanguage *language;
  guint engine_type_id;
  guint render_type_id;
  PangoMap *map;
};

struct _PangoEnginePair
{
  PangoEngineInfo info;
  PangoModule *module;
  PangoEngine *engine;
};

struct _PangoModule
{
  GTypeModule parent_instance;

  char *path;
  GModule *library;

  void         (*list)   (PangoEngineInfo **engines, gint *n_engines);
  void         (*init)   (GTypeModule *module);
  void         (*exit)   (void);
  PangoEngine *(*create) (const gchar *id);
};

struct _PangoModuleClass
{
  GTypeModuleClass parent_class;
};

static GList *maps = NULL;
static GSList *registered_engines = NULL;
static GSList *dlloaded_engines = NULL;
static GHashTable *dlloaded_modules;

static GObjectClass *parent_class;

static void build_map    (PangoMapInfo *info);
static void init_modules (void);

static GType pango_module_get_type (void);

/**
 * pango_find_map:
 * @language: the language tag for which to find the map
 * @engine_type_id: the engine type for the map to find
 * @render_type_id: the render type for the map to find
 *
 * Locate a #PangoMap for a particular engine type and render
 * type. The resulting map can be used to determine the engine
 * for each character.
 *
 * Return value: the suitable #PangoMap.
 **/
PangoMap *
pango_find_map (PangoLanguage *language,
		guint          engine_type_id,
		guint          render_type_id)
{
  GList *tmp_list = maps;
  PangoMapInfo *map_info = NULL;
  gboolean found_earlier = FALSE;

  while (tmp_list)
    {
      map_info = tmp_list->data;
      if (map_info->engine_type_id == engine_type_id &&
	  map_info->render_type_id == render_type_id)
	{
	  if (map_info->language == language)
	    break;
	  else
	    found_earlier = TRUE;
	}

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      map_info = g_slice_new (PangoMapInfo);
      map_info->language = language;
      map_info->engine_type_id = engine_type_id;
      map_info->render_type_id = render_type_id;

      build_map (map_info);

      maps = g_list_prepend (maps, map_info);
    }
  else if (found_earlier)
    {
      /* Move the found map to the beginning of the list
       * for speed next time around if we had to do
       * any failing comparison. (No longer so important,
       * since we don't strcmp.)
       */
      maps = g_list_remove_link(maps, tmp_list);
      maps = g_list_prepend(maps, tmp_list->data);
      g_list_free_1(tmp_list);
    }

  return map_info->map;
}

static gboolean
pango_module_load (GTypeModule *module)
{
  PangoModule *pango_module = PANGO_MODULE (module);

  if (pango_module->path)
    {
      pango_module->library = g_module_open (pango_module->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
      if (!pango_module->library)
	{
	  g_warning ("%s", g_module_error());
	  return FALSE;
	}

      /* extract symbols from the lib */
      if (!g_module_symbol (pango_module->library, "script_engine_init",
			    (gpointer *)&pango_module->init) ||
	  !g_module_symbol (pango_module->library, "script_engine_exit",
			    (gpointer *)&pango_module->exit) ||
	  !g_module_symbol (pango_module->library, "script_engine_list",
			    (gpointer *)&pango_module->list) ||
	  !g_module_symbol (pango_module->library, "script_engine_create",
			    (gpointer *)&pango_module->create))
	{
	  g_warning ("%s", g_module_error());
	  g_module_close (pango_module->library);

	  return FALSE;
	}
    }

  /* call the module's init function to let it */
  /* setup anything it needs to set up. */
  pango_module->init (module);

  return TRUE;
}

static void
pango_module_unload (GTypeModule *module)
{
  PangoModule *pango_module = PANGO_MODULE (module);

  pango_module->exit();

  if (pango_module->path)
    {
      g_module_close (pango_module->library);
      pango_module->library = NULL;

      pango_module->init = NULL;
      pango_module->exit = NULL;
      pango_module->list = NULL;
      pango_module->create = NULL;
    }
}

/* This only will ever be called if an error occurs during
 * initialization
 */
static void
pango_module_finalize (GObject *object)
{
  PangoModule *module = PANGO_MODULE (object);

  g_free (module->path);

  parent_class->finalize (object);
}

static void
pango_module_class_init (PangoModuleClass *class)
{
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (class));

  module_class->load = pango_module_load;
  module_class->unload = pango_module_unload;

  gobject_class->finalize = pango_module_finalize;
}

static PANGO_DEFINE_TYPE (PangoModule, pango_module,
			  pango_module_class_init, NULL,
			  G_TYPE_TYPE_MODULE)

static PangoEngine *
pango_engine_pair_get_engine (PangoEnginePair *pair)
{
  if (!pair->engine)
    {
      if (g_type_module_use (G_TYPE_MODULE (pair->module)))
	{
	  pair->engine = pair->module->create (pair->info.id);
	  g_type_module_unuse (G_TYPE_MODULE (pair->module));
	}

      if (!pair->engine)
	{
	  /* If a module cannot be used, or doesn't not create an engine
	   * correctly, we print out an error containing module name and id,
	   * but to not flood the terminal with zillions of the message, we
	   * set a flag on the module to only err once per module.
	   */
	  static GQuark warned_quark = 0;

	  if (!warned_quark)
	    warned_quark = g_quark_from_static_string ("pango-module-warned");

	  if (!g_object_get_qdata (G_OBJECT (pair->module), warned_quark))
	    {
	      g_warning ("Failed to load Pango module '%s' for id '%s'", pair->module->path, pair->info.id);

	      g_object_set_qdata_full (G_OBJECT (pair->module), warned_quark,
				       GINT_TO_POINTER (1), NULL);
	    }
	}
    }

  return pair->engine;
}

static void
handle_included_module (PangoIncludedModule *included_module,
			GSList             **engine_list)
{
  PangoModule *module = g_object_new (PANGO_TYPE_MODULE, NULL);
  PangoEngineInfo *engine_info;
  int n_engines;
  int i;

  module->list = included_module->list;
  module->init = included_module->init;
  module->exit = included_module->exit;
  module->create = included_module->create;

  module->list (&engine_info, &n_engines);

  for (i = 0; i < n_engines; i++)
    {
      PangoEnginePair *pair = g_slice_new (PangoEnginePair);

      pair->info = engine_info[i];
      pair->module = module;
      pair->engine = NULL;

      *engine_list = g_slist_prepend (*engine_list, pair);
    }
}

static PangoModule *
find_or_create_module (const char *raw_path)
{
  PangoModule *module;
  char *path;

#if defined(G_OS_WIN32) && defined(LIBDIR)
  if (strncmp (raw_path,
	       LIBDIR "/pango/" MODULE_VERSION "/modules/",
	       strlen (LIBDIR "/pango/" MODULE_VERSION "/modules/")) == 0)
    {
      /* This is an entry put there by make install on the
       * packager's system. On Windows a prebuilt Pango
       * package can be installed in a random
       * location. The pango.modules file distributed in
       * such a package contains paths from the package
       * builder's machine. Replace the path with the real
       * one on this machine. */
      path =
	g_strconcat (pango_get_lib_subdirectory (),
		     "\\" MODULE_VERSION "\\modules\\",
		     raw_path + strlen (LIBDIR "/pango/" MODULE_VERSION "/modules/"),
		     NULL);
    }
  else
#endif
    {
      path = g_strdup (raw_path);
    }

  module = g_hash_table_lookup (dlloaded_modules, path);
  if (module)
    g_free (path);
  else
    {
      module = g_object_new (PANGO_TYPE_MODULE, NULL);
      module->path = path;
      g_hash_table_insert (dlloaded_modules, path, module);
    }

  return module;
}

static PangoScript
script_from_string (const char *str)
{
  static GEnumClass *class = NULL;
  GEnumValue *value;
  if (!class)
    class = g_type_class_ref (PANGO_TYPE_SCRIPT);

  value = g_enum_get_value_by_nick (class, str);
  if (!value)
    return PANGO_SCRIPT_INVALID_CODE;

  return value->value;
}

static void
script_info_free (PangoEngineScriptInfo *script_info, gpointer data)
{
  g_slice_free (PangoEngineScriptInfo, script_info);
}

static gboolean /* Returns true if succeeded, false if failed */
process_module_file (FILE *module_file)
{
  GString *line_buf = g_string_new (NULL);
  GString *tmp_buf = g_string_new (NULL);
  gboolean have_error = FALSE;

  while (pango_read_line (module_file, line_buf))
    {
      PangoEnginePair *pair = g_slice_new (PangoEnginePair);
      PangoEngineScriptInfo *script_info;
      PangoScript script;
      GList *scripts = NULL;
      GList *tmp_list;

      const char *p;
      char *q;
      int i;

      p = line_buf->str;

      if (!pango_skip_space (&p))
	{
	  g_slice_free (PangoEnginePair, pair);
	  continue;
	}

      i = 0;
      while (1)
	{
	  if (!pango_scan_string (&p, tmp_buf))
	    {
	      have_error = TRUE;
	      goto error;
	    }

	  switch (i)
	    {
	    case 0:
	      pair->module = find_or_create_module (tmp_buf->str);
	      break;
	    case 1:
	      pair->info.id = g_strdup (tmp_buf->str);
	      break;
	    case 2:
	      pair->info.engine_type = g_strdup (tmp_buf->str);
	      break;
	    case 3:
	      pair->info.render_type = g_strdup (tmp_buf->str);
	      break;
	    default:
	      q = strchr (tmp_buf->str, ':');
	      if (!q)
		{
		  have_error = TRUE;
		  goto error;
		}
	      *q = '\0';
	      script = script_from_string (tmp_buf->str);
	      if (script == PANGO_SCRIPT_INVALID_CODE)
		{
		  have_error = TRUE;
		  goto error;
		}

	      script_info = g_slice_new (PangoEngineScriptInfo);
	      script_info->script = script;
	      script_info->langs = g_strdup (q + 1);

	      scripts = g_list_prepend (scripts, script_info);
	    }

	  if (!pango_skip_space (&p))
	    break;

	  i++;
	}

      if (i<3)
	{
	  have_error = TRUE;
	  goto error;
	}

      scripts = g_list_reverse (scripts);
      pair->info.n_scripts = g_list_length (scripts);
      pair->info.scripts = g_new (PangoEngineScriptInfo, pair->info.n_scripts);

      tmp_list = scripts;
      for (i=0; i<pair->info.n_scripts; i++)
	{
	  pair->info.scripts[i] = *(PangoEngineScriptInfo *)tmp_list->data;
	  tmp_list = tmp_list->next;
	}

      pair->engine = NULL;

      dlloaded_engines = g_slist_prepend (dlloaded_engines, pair);

    error:
      g_list_foreach (scripts, (GFunc)script_info_free, NULL);
      g_list_free (scripts);

      if (have_error)
	{
	  g_printerr ("Error reading Pango modules file\n");
	  g_slice_free(PangoEnginePair, pair);
	  break;
	}
    }

  g_string_free (line_buf, TRUE);
  g_string_free (tmp_buf, TRUE);

  return !have_error;
}

static void
read_modules (void)
{
  FILE *module_file;

  char *file_str =  pango_config_key_get ("Pango/ModuleFiles");
  char **files;
  int n;

  dlloaded_modules = g_hash_table_new (g_str_hash, g_str_equal);

  if (!file_str)
    file_str = g_build_filename (pango_get_sysconf_subdirectory (),
				 "pango.modules",
				 NULL);

  files = pango_split_file_list (file_str);

  n = 0;
  while (files[n])
    n++;

  while (n-- > 0)
    {
      module_file = g_fopen (files[n], "r");
      if (module_file)
	{
	  process_module_file(module_file);
	  fclose(module_file);
	}
    }

  g_strfreev (files);
  g_free (file_str);

  dlloaded_engines = g_slist_reverse (dlloaded_engines);
}

static void
init_modules (void)
{
  static gboolean init = FALSE;
  int i;

  if (init)
    return;
  else
    init = TRUE;

  /* Make sure that the type system is initialized */
  g_type_init ();

  for (i = 0; _pango_included_lang_modules[i].list; i++)
    pango_module_register (&_pango_included_lang_modules[i]);
  read_modules ();
}

static void
map_add_engine (PangoMapInfo    *info,
		PangoEnginePair *pair)
{
  PangoMap *map = info->map;
  int i;

  for (i=0; i<pair->info.n_scripts; i++)
    {
      PangoScript script;
      PangoMapEntry *entry;
      gboolean is_exact = FALSE;

      if (pair->info.scripts[i].langs)
	{
	  if (pango_language_matches (info->language, pair->info.scripts[i].langs))
	    is_exact = TRUE;
	}

      script = pair->info.scripts[i].script;
      if ((guint)script >= map->entries->len)
	g_array_set_size (map->entries, script + 1);

      entry = &g_array_index (map->entries, PangoMapEntry, script);

      if (is_exact)
	entry->exact = g_slist_prepend (entry->exact, pair);
      else
	entry->fallback = g_slist_prepend (entry->fallback, pair);
    }
}

static void
map_add_engine_list (PangoMapInfo *info,
		     GSList       *engines,
		     const char   *engine_type,
		     const char   *render_type)
{
  GSList *tmp_list = engines;

  while (tmp_list)
    {
      PangoEnginePair *pair = tmp_list->data;
      tmp_list = tmp_list->next;

      if (strcmp (pair->info.engine_type, engine_type) == 0 &&
	  strcmp (pair->info.render_type, render_type) == 0)
	{
	  map_add_engine (info, pair);
	}
    }
}

static void
build_map (PangoMapInfo *info)
{
  const char *engine_type = g_quark_to_string (info->engine_type_id);
  const char *render_type = g_quark_to_string (info->render_type_id);

  init_modules();

  if (!dlloaded_engines && !registered_engines)
    {
      static gboolean no_module_warning = FALSE;
      if (!no_module_warning)
	{
	  gchar *filename = g_build_filename (pango_get_sysconf_subdirectory (),
					      "pango.modules",
					      NULL);
	  g_critical ("No modules found:\n"
		      "No builtin or dynamically loaded modules were found.\n"
		      "PangoFc will not work correctly.\n"
		      "This probably means there was an error in the creation of:\n"
		      "  '%s'\n"
		      "You should create this file by running:\n"
		      "  pango-querymodules > '%s'",
		     filename,
		     filename);
	  g_free (filename);

	  no_module_warning = TRUE;
	}
    }

  info->map = g_slice_new (PangoMap);
  info->map->entries = g_array_new (FALSE, TRUE, sizeof (PangoMapEntry));

  map_add_engine_list (info, dlloaded_engines, engine_type, render_type);
  map_add_engine_list (info, registered_engines, engine_type, render_type);
}

/**
 * pango_map_get_engine:
 * @map: a #PangoMap
 * @script: a #PangoScript
 *
 * Returns the best engine listed in the map for a given script
 *
 * Return value: the best engine, if one is listed for the script,
 *    or %NULL. The lookup may cause the engine to be loaded;
 *    once an engine is loaded, it won't be unloaded. If multiple
 *    engines are exact for the script, the choice of which is
 *    returned is arbitrary.
 **/
PangoEngine *
pango_map_get_engine (PangoMap   *map,
		      PangoScript script)
{
  PangoMapEntry *entry = NULL;
  PangoMapEntry *common_entry = NULL;

  if ((guint)script < map->entries->len)
    entry = &g_array_index (map->entries, PangoMapEntry, script);

  if (PANGO_SCRIPT_COMMON < map->entries->len)
    common_entry = &g_array_index (map->entries, PangoMapEntry, PANGO_SCRIPT_COMMON);

  if (entry && entry->exact)
    return pango_engine_pair_get_engine (entry->exact->data);
  else if (common_entry && common_entry->exact)
    return pango_engine_pair_get_engine (common_entry->exact->data);
  else if (entry && entry->fallback)
    return pango_engine_pair_get_engine (entry->fallback->data);
  else if (common_entry && common_entry->fallback)
    return pango_engine_pair_get_engine (common_entry->fallback->data);
  else
    return NULL;
}

static void
append_engines (GSList **engine_list,
		GSList  *pair_list)
{
  GSList *l;

  for (l = pair_list; l; l = l->next)
    {
      PangoEngine *engine = pango_engine_pair_get_engine (l->data);
      if (engine)
	*engine_list = g_slist_append (*engine_list, engine);
    }
}

/**
 * pango_map_get_engines:
 * @map: a #PangoMap
 * @script: a #PangoScript
 * @exact_engines: location to store list of engines that exactly
 *  handle this script.
 * @fallback_engines: location to store list of engines that approximately
 *  handle this script.
 *
 * Finds engines in the map that handle the given script. The returned
 * lists should be freed with g_slist_free, but the engines in the
 * lists are owned by GLib and will be kept around permanently, so
 * they should not be unref'ed.
 *
 * Since: 1.4
 **/
void
pango_map_get_engines (PangoMap     *map,
		       PangoScript   script,
		       GSList      **exact_engines,
		       GSList      **fallback_engines)
{
  PangoMapEntry *entry = NULL;
  PangoMapEntry *common_entry = NULL;

  if ((guint)script < map->entries->len)
    entry = &g_array_index (map->entries, PangoMapEntry, script);

  if (PANGO_SCRIPT_COMMON < map->entries->len)
    common_entry = &g_array_index (map->entries, PangoMapEntry, PANGO_SCRIPT_COMMON);

  if (exact_engines)
    {
      *exact_engines = NULL;
      if (entry && entry->exact)
	append_engines (exact_engines, entry->exact);
      else if (common_entry && common_entry->exact)
	append_engines (exact_engines, common_entry->exact);
    }

  if (fallback_engines)
    {
      *fallback_engines = NULL;
      if (entry && entry->fallback)
	append_engines (fallback_engines, entry->fallback);
      else if (common_entry && common_entry->fallback)
	append_engines (fallback_engines, common_entry->fallback);
    }
}

/**
 * pango_module_register:
 * @module: a #PangoIncludedModule
 *
 * Registers a statically linked module with Pango. The
 * #PangoIncludedModule structure that is passed in contains the
 * functions that would otherwise be loaded from a dynamically loaded
 * module.
 **/
void
pango_module_register (PangoIncludedModule *module)
{
  GSList *tmp_list = NULL;

  handle_included_module (module, &tmp_list);

  registered_engines = g_slist_concat (registered_engines,
				       g_slist_reverse (tmp_list));
}
