/* Pango
 * pangoft2topgm.c: Example program to view a UTF-8 encoding file
 *                  using Pango to render result.
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2001 Sun Microsystems
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "renderdemo.h"

#include <pango/pangoft2.h>

static void
ft2_render (PangoLayout *layout,
	    int          x,
	    int          y,
	    gpointer     data)
{
  pango_ft2_render_layout (data, layout, x, y);
}

int
main(int argc, char *argv[])
{
  PangoContext *context;
  FILE *outfile;
  PangoFontMap *fontmap;
  GError *error = NULL;
  gboolean do_convert = FALSE;
  int exit_status = 0;
  char *tmpfile_name;

  g_type_init();

  parse_options (argc, argv);

  if (opt_output)
    {
      if (!(g_str_has_suffix (opt_output, ".pgm") ||
	    g_str_has_suffix (opt_output, ".PGM")))
	do_convert = TRUE;
    }

  if (opt_output && !do_convert)
    {
      outfile = fopen (opt_output, "wb");

      if (!outfile)
	fail ("Cannot open output file %s: %s\n",
	      opt_output, g_strerror (errno));
    }
  else				/* --display */
    {
      /* This may need to be G_OS_UNIX guarded for fdopen */
      int fd = g_file_open_tmp ("pangoft2pgmXXXXXX", &tmpfile_name, &error);
      if (fd == 1)
	fail ("Cannot open temporary file: %s\n", error->message);
      outfile = fdopen (fd, "wb");
      if (!outfile)
	fail ("Cannot open temporary file: %s\n", g_strerror (errno));
    }

  fontmap = pango_ft2_font_map_new ();

  pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap), opt_dpi, opt_dpi);
  pango_ft2_font_map_set_default_substitute (PANGO_FT2_FONT_MAP (fontmap), fc_substitute_func, NULL, NULL);
  context = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP (fontmap));

  g_object_unref (fontmap);

  /* Write contents as pgm file */
  {
      FT_Bitmap bitmap;
      guchar *buf;
      int row;
      int width, height;

      do_output (context, NULL, NULL, NULL, &width, &height);
      
      bitmap.width = width;
      bitmap.pitch = (bitmap.width + 3) & ~3;
      bitmap.rows = height;
      buf = bitmap.buffer = g_malloc (bitmap.pitch * bitmap.rows);
      bitmap.num_grays = 256;
      bitmap.pixel_mode = ft_pixel_mode_grays;
      memset (buf, 0x00, bitmap.pitch * bitmap.rows);

      do_output (context, ft2_render, NULL, &bitmap, &width, &height);

      /* Invert bitmap to get black text on white background */
      {
	int pix_idx;
	for (pix_idx=0; pix_idx<bitmap.pitch * bitmap.rows; pix_idx++)
	  {
	    buf[pix_idx] = 255-buf[pix_idx];
	  }
      }
      
      /* Write it as pgm to output */
      fprintf(outfile,
	      "P5\n"
	      "%d %d\n"
	      "255\n", bitmap.width, bitmap.rows);
      for (row = 0; row < bitmap.rows; row++)
	fwrite(bitmap.buffer + row * bitmap.pitch,
	       1, bitmap.width,
	       outfile);
      g_free (buf);
      if (fclose(outfile) == EOF)
	fail ("Error writing output file: %s\n", g_strerror (errno));

      /* Convert to a different format, if necessary */
      if (do_convert)
	{
	  gchar *command = g_strdup_printf ("convert %s %s",
					    tmpfile_name,
					    opt_output);
	  if (!g_spawn_command_line_sync (command, NULL, NULL, &exit_status, &error))
	    fail ("When running ImageMagick 'convert' command: %s\n",
		  error->message);

	  g_free (command);
	  
	  if (tmpfile_name)
	    {
	      remove (tmpfile_name);
	      g_free (tmpfile_name);
	      tmpfile_name = NULL;
	    }

	  if (exit_status)
	    goto done;
	}

      if (opt_display)
	{
	  gchar *title = get_options_string ();
	  gchar *title_quoted = g_shell_quote (title);
	  
	  gchar *command = g_strdup_printf ("display -title %s %s",
					    title_quoted,
					    opt_output ? opt_output: tmpfile_name);
	  if (!g_spawn_command_line_sync (command, NULL, NULL, &exit_status, &error))
	    fail ("When running ImageMagick 'display' command: %s\n",
		  error->message);

	  g_free (command);
	  g_free (title);
	  g_free (title_quoted);
	  
	  if (tmpfile_name)
	    {
	      remove (tmpfile_name);
	      g_free (tmpfile_name);
	      tmpfile_name = NULL;
	    }

	  if (exit_status)
	    goto done;
	}
    }

done:
  g_object_unref (context);
  finalize ();

  return exit_status ? 1 : 0;
}
