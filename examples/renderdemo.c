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

#define BUFSIZE 1024
#define MALLOCSIZE 1024

#include <pango/pango.h>
#include <pango/pangoft2.h>

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *prog_name = NULL;

PangoContext *context;

static char *init_family = "sans";
static int init_scale = 24;
static int init_margin = 10;
static  PangoDirection init_dir = PANGO_DIRECTION_LTR;
static  gboolean init_waterfall = FALSE;

static void
fail (const char *format, ...)
{
  const char *msg;
  
  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  g_printerr ("%s\n", msg);
  
  exit (1);
}

static PangoLayout *
make_layout(PangoContext *context,
	    const char   *text,
	    int           scale)
{
  static PangoFontDescription *font_description;
  PangoDirection base_dir;
  PangoLayout *layout;

  layout = pango_layout_new (context);
  pango_layout_set_text (layout, text, -1);

  font_description = pango_font_description_new ();
  pango_font_description_set_family (font_description, init_family);
  pango_font_description_set_size (font_description, scale * PANGO_SCALE);

  base_dir = pango_context_get_base_dir (context);
  pango_layout_set_alignment (layout,
			      base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
  
  pango_layout_set_font_description (layout, font_description);
  pango_font_description_free (font_description);

  return layout;
}

static void
do_output (PangoContext *context,
	   const char   *text,
	   FT_Bitmap    *bitmap,
	   int          *width,
	   int          *height)
{
  PangoLayout *layout;
  PangoRectangle logical_rect;
  int x = init_margin;
  int y = init_margin;
  int scale, start_scale, end_scale, increment;
  
  *width = 0;
  *height = 0;

  if (init_waterfall)
    {
      start_scale = 8;
      end_scale = 48;
      increment = 4;
    }
  else
    {
      start_scale = end_scale = init_scale;
      increment = 1;
    }

  for (scale = start_scale; scale <= end_scale; scale += increment)
    {
      layout = make_layout (context, text, scale);
      pango_layout_get_extents (layout, NULL, &logical_rect);
      
      *width = MAX (*width, PANGO_PIXELS (logical_rect.width));
      *height += PANGO_PIXELS (logical_rect.height);
      
      if (bitmap)
	pango_ft2_render_layout (bitmap, layout, x, y);
      
      y += PANGO_PIXELS (logical_rect.height);

      g_object_unref (layout);
    }
  
  *width += 2 * init_margin;
  *height += 2 * init_margin;
}

int main(int argc, char *argv[])
{
  FILE *outfile;
  int dpi_x = 100, dpi_y = 100;
  char *text;
  size_t len;
  char *p;
  int argp;
  char *prog_name = g_path_get_basename (argv[0]);
  GError *error = NULL;
  
  g_type_init();

  if (g_file_test ("./pangorc", G_FILE_TEST_EXISTS))
    putenv ("PANGO_RC_FILE=./pangorc");
  
  /* Parse command line */
  argp=1;
  while(argp < argc && argv[argp][0] == '-')
    {
      char *opt = argv[argp++];
      if (strcmp(opt, "--help") == 0)
	{
	  printf("%s - An example viewer for the pango ft2 extension\n"
		 "\n"
		 "Syntax:\n"
		 "    %s [--family f] [--scale s] file\n"
		 "\n"
		 "Options:\n"
		 "    --family f   Set the family. Default is '%s'.\n"
		 "    --margin m   Set the margin on the output in pixels. Default is %d.\n"
		 "    --scale s    Set the scale. Default is %d.\n"
		 "    --rtl        Set base dir to RTL. Default is LTR.\n"
		 "    --waterfall  Create a waterfall display."
		 "    --width      Width of drawing window. Default is 500.\n",
		 prog_name, prog_name, init_family, init_margin, init_scale);
	  exit(0);
	}
      if (strcmp(opt, "--family") == 0)
	{
	  init_family = argv[argp++];
	  continue;
	}
      if (strcmp(opt, "--margin") == 0)
	{
	  init_margin = atoi(argv[argp++]);
	  continue;
	}
      if (strcmp(opt, "--waterfall") == 0)
	{
	  init_waterfall = TRUE;
	  continue;
	}
      if (strcmp(opt, "--scale") == 0)
	{
	  init_scale = atoi(argv[argp++]);
	  continue;
	}
      if (strcmp(opt, "--rtl") == 0)
	{
	  init_dir = PANGO_DIRECTION_RTL;
	  continue;
	}
      fail ("Unknown option %s!\n", opt);
    }

  if (argp + 1 != argc && argp + 2 != argc)
    fail ("Usage: %s [options] FILE [OUTFILE]\n", prog_name);

  /* Get the text in the supplied file
   */
  if (!g_file_get_contents (argv[argp++], &text, &len, &error))
    fail ("%s\n", error->message);
  if (!g_utf8_validate (text, len, NULL))
    fail ("Text is not valid UTF-8");

  /* Strip trailing whitespace
   */
  p = text + len;
  while (p > text)
    {
      gunichar ch;
      p = g_utf8_prev_char (p);
      ch = g_utf8_get_char (p);
      if (!g_unichar_isspace (ch))
	break;
      else
	*p = '\0';
    }

  if (argp < argc)
    outfile = fopen (argv[argp++], "wb");
  else
    outfile = stdout;
  
  if (!outfile)
    fail ("Cannot open output file %s: s\n", outfile, g_strerror (errno));

  context = pango_ft2_get_context (dpi_x, dpi_y);

  pango_context_set_language (context, pango_language_from_string ("en_US"));
  pango_context_set_base_dir (context, init_dir);

  /* Write contents as pgm file */
  {
      FT_Bitmap bitmap;
      guchar *buf;
      int row;
      int width, height;

      do_output (context, text, NULL, &width, &height);
      
      bitmap.width = width;
      bitmap.pitch = (bitmap.width + 3) & ~3;
      bitmap.rows = height;
      buf = bitmap.buffer = g_malloc (bitmap.pitch * bitmap.rows);
      bitmap.num_grays = 256;
      bitmap.pixel_mode = ft_pixel_mode_grays;
      memset (buf, 0x00, bitmap.pitch * bitmap.rows);
	  
      do_output (context, text, &bitmap, &width, &height);
      
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
    }

  return 0;
}
