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

#include <glib.h>
#include <gmodule.h>
#include "pango.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void 
query_module (GModule *module, gchar *name)
{
  void (*list) (PangoEngineInfo **engines, gint *n_engines);
  PangoEngine *(*load) (const gchar *id);
  void (*unload) (PangoEngine *engine);
  
  if (g_module_symbol (module, "script_engine_list", (gpointer)&list) &&
      g_module_symbol (module, "script_engine_load", (gpointer)&load) &&
      g_module_symbol (module, "script_engine_unload", (gpointer)&unload))
    {
      gint i,j;
      PangoEngineInfo *engines;
      gint n_engines;

      (*list) (&engines, &n_engines);

      for (i=0; i<n_engines; i++)
	{
	  g_print ("%s %s %s %s ", name, engines[i].id, engines[i].engine_type, engines[i].render_type);
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
      fprintf (stderr, "%s does not export Pango module API\n", name);
    }
}		       

int main (int argc, char **argv)
{
  char cwd[PATH_MAX];
  int i;

  getcwd (cwd, PATH_MAX);

  for (i=1; i<argc; i++)
    {
      GModule *module;
      gchar *tmp;
      
      if (argv[i][0] == '/')
	tmp = g_strdup (argv[i]);
      else
	tmp = g_strconcat (cwd, "/", argv[i], NULL);

      module = g_module_open (tmp, 0);
      if (module)
	{
	  query_module (module, tmp);
	  g_module_close (module);
	}
      else
	{
	  fprintf(stderr, "Cannot load module %s: %s\n",
		  tmp, g_module_error());
	}
      
      g_free (tmp);
    }
  
  return 0;
}
