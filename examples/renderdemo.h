/* renderdemo.c: Common code for rendering demos
 *
 * Copyright (C) 1999, 2004 Red Hat Software
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

#include <pango/pango-layout.h>

typedef void (*RenderCallback) (PangoLayout *layout,
				int          x,
				int          y,
				gpointer     data);

void fail (const char *format, ...) G_GNUC_PRINTF (1, 2);

void   parse_options      (int            argc,
			   char          *argv[]);
void   do_output          (PangoContext  *context,
			   RenderCallback render_cb,
			   gpointer       render_data,
			   int           *width,
			   int           *height);
gchar *get_options_string (void);

extern char *prog_name;

extern gboolean opt_display;
extern int opt_dpi;
extern char *opt_font;
extern gboolean opt_header;
extern char *opt_output;
extern int opt_margin;
extern int opt_markup;
extern gboolean opt_rtl;
extern int opt_rotate;
extern gboolean opt_auto_dir;
extern char *opt_text;
extern gboolean opt_waterfall;
extern int opt_width;
extern int opt_indent;
