/* Pango
 * querymodules.c:
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

#include <glib.h>
#include <gmodule.h>
#include "pango-break.h"
#include "pango-context.h"
#include "pango-utils.h"
#include "pango-engine.h"

#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>

#ifdef G_OS_WIN32
#define SOEXT ".dll"
#else
#define SOEXT ".so"
#endif

void 
query_module (const char *dir, const char *name)
{
  void (*list) (PangoEngineInfo **engines, gint *n_engines);
  PangoEngine *(*load) (const gchar *id);
  void (*unload) (PangoEngine *engine);

  GModule *module;
  gchar *path;

  if (g_path_is_absolute (name))
    path = g_strdup (name);
  else
    path = g_build_filename (dir, name, NULL);
  
  module = g_module_open (path, 0);

  if (!module)
    fprintf(stderr, "Cannot load module %s: %s\n", path, g_module_error());
	  
  if (module &&
      g_module_symbol (module, "script_engine_list", (gpointer *) &list) &&
      g_module_symbol (module, "script_engine_load", (gpointer *) &load) &&
      g_module_symbol (module, "script_engine_unload", (gpointer *) &unload))
    {
      gint i,j;
      PangoEngineInfo *engines;
      gint n_engines;

      (*list) (&engines, &n_engines);

      for (i=0; i<n_engines; i++)
	{
	  const gchar *quote;
	  gchar *quoted_path;

	  if (strchr (path, ' ') != NULL)
	    {
	      quote = "\"";
	      quoted_path = g_strescape (path, NULL);
	    }
	  else
	    {
	      quote = "";
	      quoted_path = g_strdup (path);
	    }
	  
	  g_print ("%s%s%s %s %s %s ", quote, quoted_path, quote,
		   engines[i].id, engines[i].engine_type, engines[i].render_type);
	  g_free (quoted_path);

	  for (j=0; j < engines[i].n_ranges; j++)
	    {
	      if (j != 0)
		g_print (" ");
	      g_print ("%d-%d:%s",
		       engines[i].ranges[j].start,
		       engines[i].ranges[j].end,
		       engines[i].ranges[j].langs);
	    }
	  g_print ("\n");
	  }
    }
  else
    {
      fprintf (stderr, "%s does not export Pango module API\n", path);
    }

  g_free (path);
  if (module)
    g_module_close (module);
}		       

int main (int argc, char **argv)
{
  char *cwd;
  int i;
  char *path;

  printf ("# Pango Modules file\n"
	  "# Automatically generated file, do not edit\n"
	  "#\n");

  if (argc == 1)		/* No arguments given */
    {
      char **dirs;
      int i;
      
      path = pango_config_key_get ("Pango/ModulesPath");
      if (!path)
	path = g_build_filename (pango_get_lib_subdirectory (),
				 "modules",
				 NULL);

      printf ("# ModulesPath = %s\n#\n", path);

      dirs = pango_split_file_list (path);

      for (i=0; dirs[i]; i++)
	{
	  GDir *dir = g_dir_open (dirs[i], 0, NULL);
	  if (dir)
	    {
	      const char *dent;

	      while ((dent = g_dir_read_name (dir)))
		{
		  int len = strlen (dent);
		  if (len > 3 && strcmp (dent + len - strlen (SOEXT), SOEXT) == 0)
		    query_module (dirs[i], dent);
		}
	      
	      g_dir_close (dir);
	    }
	}
    }
  else
    {
      cwd = g_get_current_dir ();
      
      for (i=1; i<argc; i++)
	query_module (cwd, argv[i]);

      g_free (cwd);
    }
  
  return 0;
}
