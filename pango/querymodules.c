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
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
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
#ifndef PATH_MAX
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#endif /* PATH_MAX */
#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#endif
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

  if (name[0] == G_DIR_SEPARATOR)
    path = g_strdup (name);
  else
    path = g_strconcat (dir, G_DIR_SEPARATOR_S, name, NULL);
  
  module = g_module_open (path, 0);

  if (!module)
    fprintf(stderr, "Cannot load module %s: %s\n", path, g_module_error());
	  
  if (module &&
      g_module_symbol (module, "script_engine_list", (gpointer)&list) &&
      g_module_symbol (module, "script_engine_load", (gpointer)&load) &&
      g_module_symbol (module, "script_engine_unload", (gpointer)&unload))
    {
      gint i,j;
      PangoEngineInfo *engines;
      gint n_engines;

      (*list) (&engines, &n_engines);

      for (i=0; i<n_engines; i++)
	{
	  g_print ("%s %s %s %s ", path, engines[i].id, engines[i].engine_type, engines[i].render_type);
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
  char cwd[PATH_MAX];
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
	path = g_strconcat (pango_get_lib_subdirectory (),
			    G_DIR_SEPARATOR_S "modules",
			    NULL);

      printf ("# ModulesPath = %s\n#\n", path);

      dirs = pango_split_file_list (path);

      for (i=0; dirs[i]; i++)
	{
	  DIR *dir = opendir (dirs[i]);
	  if (dir)
	    {
	      struct dirent *dent;

	      while ((dent = readdir (dir)))
		{
		  int len = strlen (dent->d_name);
		  if (len > 3 && strcmp (dent->d_name + len - strlen (SOEXT), SOEXT) == 0)
		    query_module (dirs[i], dent->d_name);
		}
	      
	      closedir (dir);
	    }
	}
    }
  else
    {
      getcwd (cwd, PATH_MAX);
      
      for (i=1; i<argc; i++)
	query_module (cwd, argv[i]);
    }
  
  return 0;
}
