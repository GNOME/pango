/* viewer-main.c: Main routine for viewers
 *
 * Copyright (C) 1999,2004,2005 Red Hat, Inc.
 * Copyright (C) 2001 Sun Microsystems
 * Copyright (C) 2006 Behdad Esfahbod
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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <sys/wait.h>
#endif

#include "viewer.h"
#include "viewer-render.h"

int
main (int    argc,
      char **argv)
{
  const PangoViewer *view;
  gpointer instance;
  PangoContext *context;
  int run;
  int width, height;
  gpointer surface;

  g_type_init();
  setlocale (LC_ALL, "");
  parse_options (argc, argv);

  view = opt_viewer;

  g_assert (view->id);

  instance = view->create (view);
  context = view->get_context (instance);
  do_output (context, NULL, NULL, NULL, NULL, &width, &height);
  surface = view->create_surface (instance, width, height);
  for (run = 0; run < MAX(1,opt_runs); run++)
    view->render (instance, surface, context, width, height, NULL);

  if (opt_output)
    {
      if (!view->write)
	fail ("%s viewer backend does not support writing", view->name);
      else
	{
	  FILE *stream;
	  GPid pid = 0;

	  if (view->write_suffix && g_str_has_suffix (opt_output, view->write_suffix))
	    {
	      stream = g_fopen (opt_output, "wb");
	      if (!stream)
		fail ("Cannot open output file %s: %s\n",
		      opt_output, g_strerror (errno));
	    }
	  else
	    {
	      int fd;
	      const gchar *convert_argv[4] = {"convert", "-", "%s"};
	      GError *error;

	      convert_argv[2] = opt_output;

	      if (!g_spawn_async_with_pipes (NULL, (gchar **)convert_argv, NULL,
					     G_SPAWN_DO_NOT_REAP_CHILD |
					     G_SPAWN_SEARCH_PATH |
					     G_SPAWN_STDOUT_TO_DEV_NULL |
					     G_SPAWN_STDERR_TO_DEV_NULL,
					     NULL, NULL, &pid, &fd, NULL, NULL, &error))
		fail ("When running ImageMagick 'convert' command: %s\n", error->message);
	      stream = fdopen (fd, "wb");
	    }
	  view->write (instance, surface, stream, width, height);
	  fclose (stream);
#ifdef G_OS_UNIX
	  if (pid)
	    waitpid (pid, NULL, 0);
#endif
	}
    }

  if (opt_display)
    {
      char *title;
      title = get_options_string ();

      if (view->display)
	{
	  gpointer window = NULL;
	  gpointer state = NULL;

	  if (view->create_window)
	    window = view->create_window (instance, title, width, height);

	  while (1)
	    {
	      state = view->display (instance, surface, window, width, height, state);
	      if (!state)
		break;

	      view->render (instance, surface, context, width, height, state);
	    }

	  if (view->destroy_window)
	    view->destroy_window (instance, window);
	}
      else
	{
	  int fd;
	  FILE *stream;
	  const gchar *display_argv[5] = {"display", "-title", "%s", "-"};
	  GError *error = NULL;
	  GPid pid;

	  if (!view->write)
	    fail ("%s viewer backend does not support displaying or writing", view->name);
	  display_argv[2] = title;

	  if (!g_spawn_async_with_pipes (NULL, (gchar **)display_argv, NULL,
					 G_SPAWN_DO_NOT_REAP_CHILD |
					 G_SPAWN_SEARCH_PATH |
					 G_SPAWN_STDOUT_TO_DEV_NULL |
					 G_SPAWN_STDERR_TO_DEV_NULL,
					 NULL, NULL, &pid, &fd, NULL, NULL, &error))
	    fail ("When running ImageMagick 'display' command: %s\n", error->message);
	  stream = fdopen (fd, "wb");
	  view->write (instance, surface, stream, width, height);
	  fclose (stream);
#ifdef G_OS_UNIX
	  waitpid (pid, NULL, 0);
#endif
	  g_spawn_close_pid (pid);
	}

      g_free (title);
    }

  view->destroy_surface (instance, surface);
  g_object_unref (context);
  view->destroy (instance);
  finalize ();
  return 0;
}
