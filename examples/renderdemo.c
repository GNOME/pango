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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *prog_name = NULL;
typedef struct _Paragraph Paragraph;

/* Structure representing a paragraph
 */
struct _Paragraph {
  char *text;
  int length;
  int height;   /* Height, in pixels */
  PangoLayout *layout;
};

GList *paragraphs;

static PangoFontDescription *font_description;

PangoContext *context;

/* Read an entire file into a string
 */
static char *
read_file (char *name)
{
  GString *inbuf;
  FILE *file;
  char *text;
  char buffer[BUFSIZE];

  file = fopen (name, "r");
  if (!file)
    {
      fprintf (stderr, "%s: Cannot open %s\n", g_get_prgname (), name);
      return NULL;
    }

  inbuf = g_string_new (NULL);
  while (1)
    {
      char *bp = fgets (buffer, BUFSIZE-1, file);
      if (ferror (file))
	{
	  fprintf(stderr, "%s: Error reading %s\n", g_get_prgname (), name);
	  g_string_free (inbuf, TRUE);
	  return NULL;
	}
      else if (bp == NULL)
	break;

      g_string_append (inbuf, buffer);
    }

  fclose (file);

  text = inbuf->str;
  g_string_free (inbuf, FALSE);

  return text;
}

/* Take a UTF8 string and break it into paragraphs on \n characters
 */
static GList *
split_paragraphs (char *text)
{
  char *p = text;
  char *next;
  gunichar wc;
  GList *result = NULL;
  char *last_para = text;
  
  while (*p)
    {
      wc = g_utf8_get_char (p);
      next = g_utf8_next_char (p);
      if (wc == (gunichar)-1)
	{
	  fprintf (stderr, "%s: Invalid character in input\n", g_get_prgname ());
	  wc = 0;
	}
      if (!*p || !wc || wc == '\n')
	{
	  Paragraph *para = g_new (Paragraph, 1);
	  para->text = last_para;
	  para->length = p - last_para;
	  para->layout = pango_layout_new (context);
	  pango_layout_set_text (para->layout, para->text, para->length);
	  para->height = 0;

	  last_para = next;
	    
	  result = g_list_prepend (result, para);
	}
      if (!wc) /* incomplete character at end */
	break;
      p = next;
    }

  return g_list_reverse (result);
}

int main(int argc, char *argv[])
{
  FILE *outfile;
  int dpi_x = 100, dpi_y = 100;
  char *init_family = "sans";
  int init_scale = 24;
  PangoDirection init_dir = PANGO_DIRECTION_LTR;
  char *text;
  int y_start = 0, x_start = 0;
  int paint_width = 500;
  int argp;
  char *prog_name = g_path_get_basename (argv[0]);
  
  g_type_init();
  
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
		 "    --family f   Set the initial family. Default is '%s'.\n"
		 "    --scale s    Set the initial scale. Default is %d\n"
		 "    --rtl        Set base dir to RTL. Default is LTR.\n"
		 "    --width      Width of drawing window. Default is 500.\n",
		 prog_name, prog_name, init_family, init_scale);
	  exit(0);
	}
      if (strcmp(opt, "--family") == 0)
	{
	  init_family = argv[argp++];
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
      if (strcmp(opt, "--width") == 0)
	{
	  paint_width = atoi(argv[argp++]);
	  continue;
	}
      fprintf(stderr, "Unknown option %s!\n", opt);
      exit(1);
    }

  if (argp + 1 != argc && argp + 2 != argc)
    {
      fprintf (stderr, "Usage: %s [options] FILE [OUTFILE]\n", prog_name);
      exit(1);
    }

  /* Create the list of paragraphs from the supplied file
   */
  text = read_file (argv[argp++]);
  if (!text)
    exit(1);

  if (argp < argc)
    outfile = fopen (argv[argp++], "wb");
  else
    outfile = stdout;		/* Problematic if freetype outputs warnings
				 * to stdout...
				 */

  context = pango_ft2_get_context (dpi_x, dpi_y);

  paragraphs = split_paragraphs (text);

  pango_context_set_language (context, pango_language_from_string ("en_US"));
  pango_context_set_base_dir (context, init_dir);

  font_description = pango_font_description_new ();
  pango_font_description_set_family (font_description, g_strdup (init_family));
  pango_font_description_set_style (font_description, PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (font_description, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (font_description, PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (font_description, PANGO_STRETCH_NORMAL);
  pango_font_description_set_size (font_description, init_scale * PANGO_SCALE);

  pango_context_set_font_description (context, font_description);

  /* Write first paragraph as a pgm file */
  {
      Paragraph *para = paragraphs->data;
      int height = 0;
      PangoDirection base_dir = pango_context_get_base_dir (context);
      PangoRectangle logical_rect;

      pango_layout_set_alignment (para->layout,
				  base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
      pango_layout_set_width (para->layout, paint_width * PANGO_SCALE);

      pango_layout_get_extents (para->layout, NULL, &logical_rect);
      para->height = PANGO_PIXELS (logical_rect.height);
      

      if (height + para->height >= y_start)
	{
	  FT_Bitmap bitmap;
	  guchar *buf = g_malloc (paint_width * para->height);
      
	  memset (buf, 0x00, paint_width * para->height);
	  bitmap.rows = para->height;
	  bitmap.width = paint_width;
	  bitmap.pitch = bitmap.width;
	  bitmap.buffer = buf;
	  bitmap.num_grays = 256;
	  bitmap.pixel_mode = ft_pixel_mode_grays;
	  
	  pango_ft2_render_layout (&bitmap, para->layout,
				   x_start, 0);

	  /* Invert bitmap to get black text on white background */
	  {
	    int pix_idx;
	    for (pix_idx=0; pix_idx<paint_width * para->height; pix_idx++)
	      {
		buf[pix_idx] = 255-buf[pix_idx];
	      }
	  }

	  /* Write it as pgm to output */
	  fprintf(outfile,
		  "P5\n"
		  "%d %d\n"
		  "255\n", bitmap.width, bitmap.rows);
	  fwrite(bitmap.buffer, 1, bitmap.width * bitmap.rows, outfile);
	  g_free (buf);
	}
    }

  return 0;
}
