/* viewer-render.c: Common code for rendering in viewers
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
#ifndef VIEWER_RENDER_H
#define VIEWER_RENDER_H

#include <pango/pango-layout.h>

#include "viewer.h"

typedef enum {
  HINT_DEFAULT,
  HINT_NONE,
  HINT_AUTO,
  HINT_SLIGHT,
  HINT_MEDIUM,
  HINT_FULL
} HintMode;

typedef enum {
  SUBPIXEL_DEFAULT,
  SUBPIXEL_RGB,
  SUBPIXEL_BGR,
  SUBPIXEL_VRGB,
  SUBPIXEL_VBGR
} SubpixelOrder;

typedef enum {
  ANTIALIAS_DEFAULT,
  ANTIALIAS_NONE,
  ANTIALIAS_GRAY,
  ANTIALIAS_SUBPIXEL
} Antialias;

typedef enum {
  HINT_METRICS_DEFAULT,
  HINT_METRICS_OFF,
  HINT_METRICS_ON
} HintMetrics;

typedef void (*RenderCallback) (PangoLayout *layout,
				int          x,
				int          y,
				gpointer     cb_context,
				gpointer     cb_data);
typedef void (*TransformCallback) (PangoContext *context,
				   PangoMatrix  *transform,
				   gpointer      cb_context,
				   gpointer      cb_data);

void fail (const char *format, ...) G_GNUC_PRINTF (1, 2) G_GNUC_NORETURN;

void   parse_options      (int               argc,
			   char             *argv[]);
void   do_output          (PangoContext     *context,
			   RenderCallback    render_cb,
			   TransformCallback transform_cb,
			   gpointer          cb_context,
			   gpointer          cb_data,
			   int              *width,
			   int              *height);
void   finalize           (void);
gchar *get_options_string (void);

extern const char *prog_name;

/* handled by viewer-render.c */
extern const char *opt_font;
extern gboolean opt_header;
extern int opt_margin;
extern int opt_markup;
extern gboolean opt_rtl;
extern double opt_rotate;
extern gboolean opt_auto_dir;
extern const char *opt_text;
extern gboolean opt_waterfall;
extern int opt_width;
extern int opt_indent;
extern PangoEllipsizeMode opt_ellipsize;

/* handled by viewer-main.c */
extern gboolean opt_display;
extern const char *opt_output;
extern int opt_runs;
extern const PangoViewer *opt_viewer;

/* handled by backend-specific code */
extern int opt_dpi;
extern HintMode opt_hinting;
extern SubpixelOrder opt_subpixel_order;
extern Antialias opt_antialias;
extern HintMetrics opt_hint_metrics;
extern gboolean opt_subpixel_positions;
extern PangoColor opt_fg_color;
extern guint16 opt_fg_alpha;
extern gboolean opt_bg_set;
extern PangoColor opt_bg_color;
extern guint16 opt_bg_alpha;

#endif /* VIEWER_RENDER_H */
