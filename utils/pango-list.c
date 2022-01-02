/* pango-list.c: List all fonts
 *
 * Copyright (C) 2018 Google
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
 *
 * Google Author(s): Behdad Esfahbod
 */

#include "config.h"
#include <pango/pangocairo.h>
#include <pango/pangofc-hbfontmap.h>
#include <pango/pangofc-font.h>
#include <pango/pango-hbface-private.h>
#include <hb-ot.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <locale.h>


#if HB_VERSION_ATLEAST (3, 3, 0)
static void
get_axes_and_values (hb_font_t             *font,
                     unsigned int           n_axes,
                     hb_ot_var_axis_info_t *axes,
                     float                 *coords)
{
  const float *dcoords;
  unsigned int length = n_axes;

  hb_ot_var_get_axis_infos (hb_font_get_face (font), 0, &length, axes);

  dcoords = hb_font_get_var_coords_design (font, &length);
  if (dcoords)
    memcpy (coords, dcoords, sizeof (float) * length);
  else
    {
      for (int i = 0; i < n_axes; i++)
        {
          hb_ot_var_axis_info_t *axis = &axes[i];
          coords[axis->axis_index] = axis->default_value;
        }
    }
}

#else

/* FIXME: This doesn't work if the font has an avar table */
static float
denorm_coord (hb_ot_var_axis_info_t *axis, int coord)
{
  float r = coord / 16384.0;

  if (coord < 0)
    return axis->default_value + r * (axis->default_value - axis->min_value);
  else
    return axis->default_value + r * (axis->max_value - axis->default_value);
}

static void
get_axes_and_values (hb_font_t             *font,
                     unsigned int           n_axes,
                     hb_ot_var_axis_info_t *axes,
                     float                 *coords)
{
  const int *ncoords;
  unsigned int length = n_axes;

  hb_ot_var_get_axis_infos (hb_font_get_face (font), 0, &length, axes);

  ncoords = hb_font_get_var_coords_normalized (font, &length);

  for (int i = 0; i < n_axes; i++)
    {
      hb_ot_var_axis_info_t *axis = &axes[i];
      int idx = axis->axis_index;
      if (ncoords)
        coords[idx] = denorm_coord (axis, ncoords[idx]);
      else
        coords[idx] = axis->default_value;
    }
}
#endif

int
main (int    argc,
      char **argv)
{
  gboolean opt_verbose = FALSE;
  gboolean opt_metrics = FALSE;
  gboolean opt_variations = FALSE;
  gboolean opt_version = FALSE;
  GOptionEntry entries[] = {
    { "verbose", 0, 0, G_OPTION_ARG_NONE, &opt_verbose, "Print verbose information", NULL },
    { "metrics", 0, 0, G_OPTION_ARG_NONE, &opt_metrics, "Print font metrics", NULL },
    { "variations", 0, 0, G_OPTION_ARG_NONE, &opt_variations, "Print font variations", NULL },
    { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version" },
    { NULL, }
  };
  GOptionContext *context;
  PangoContext *ctx;
  PangoFontMap *fontmap;
  int i, j;
  int width;
  GError *error = NULL;
  hb_ot_var_axis_info_t axes[100];
  float coords[100];
  unsigned int n_axes;

  g_set_prgname ("pango-list");
  setlocale (LC_ALL, "");

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      if (error != NULL)
        g_printerr ("%s\n", error->message);
      else
        g_printerr ("Option parse error\n");
      exit (1);
    }

  g_option_context_free (context);

  if (opt_version)
    {
      g_print ("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);
      exit (0);
    }

  fontmap = pango_cairo_font_map_get_default ();
  ctx = pango_font_map_create_context (fontmap);

  if (opt_verbose)
    g_print ("Using %s\n\n", G_OBJECT_TYPE_NAME (fontmap));

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (fontmap)); i++)
    {
      PangoFontFamily *family = g_list_model_get_item (G_LIST_MODEL (fontmap), i);
      const char *kind;
      const char *family_name = pango_font_family_get_name (family);

      g_object_unref (family);

      if (pango_font_family_is_monospace (family))
        {
          if (pango_font_family_is_variable (family))
            kind = "(monospace, variable)";
          else
            kind = "(monospace)";
        }
      else
        {
          if (pango_font_family_is_variable (family))
            kind = "(variable)";
          else
            kind = "";
        }

      g_print ("%s %s\n", family_name, kind);

      width = 0;
      for (j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (family)); j++)
	{
          PangoFontFace *face = g_list_model_get_item (G_LIST_MODEL (family), j);
	  const char *face_name = pango_font_face_get_face_name (face);
	  gboolean is_synth = pango_font_face_is_synthesized (face);
	  const char *synth_str = is_synth ? "*" : "";
          gboolean is_variable = pango_font_face_is_variable (face);
	  const char *variable_str = is_variable ? "@" : "";
          width = MAX (width, strlen (synth_str) + strlen (variable_str) + strlen (face_name));
          g_object_unref (face);
        }

      for (j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (family)); j++)
        {
          PangoFontFace *face = g_list_model_get_item (G_LIST_MODEL (family), j);
          const char *face_name = pango_font_face_get_face_name (face);
          gboolean is_synth = pango_font_face_is_synthesized (face);
          const char *synth_str = is_synth ? "*" : "";
          gboolean is_variable = pango_font_face_is_variable (face);
          const char *variable_str = is_variable ? "@" : "";
          PangoFontDescription *desc;
          char *desc_str;

          desc = pango_font_face_describe (face);
          desc_str = pango_font_description_to_string (desc);

          g_print ("  %s%s%s: %*s%s\n", synth_str, variable_str, face_name,
                   width - (int)strlen (face_name) - (int)strlen (synth_str) - (int)strlen (variable_str), "", desc_str);

          pango_font_description_set_absolute_size (desc, 10 * PANGO_SCALE);

          if (opt_metrics)
            {
              PangoFontMetrics *metrics;

              metrics = pango_context_get_metrics (ctx, desc, pango_language_from_string ("en-us"));
              g_print ("    (a %d d %d h %d cw %d dw %d u %d %d s %d %d)\n",
                       pango_font_metrics_get_ascent (metrics),
                       pango_font_metrics_get_descent (metrics),
                       pango_font_metrics_get_height (metrics),
                       pango_font_metrics_get_approximate_char_width (metrics),
                       pango_font_metrics_get_approximate_digit_width (metrics),
                       pango_font_metrics_get_underline_position (metrics),
                       pango_font_metrics_get_underline_thickness (metrics),
                       pango_font_metrics_get_strikethrough_position (metrics),
                       pango_font_metrics_get_strikethrough_thickness (metrics));
              pango_font_metrics_unref (metrics);
            }

          if (opt_variations &&
              pango_font_family_is_variable (family))
            {
              PangoFont *font;
              hb_font_t *hb_font;
              int instance_id = -1;

              /* set variations here, to make the fontmap prefer the variable family
               * over a non-variable one.
               */
              pango_font_description_set_variations (desc, "@");

              font = pango_context_load_font (ctx, desc);
              hb_font = pango_font_get_hb_font (font);
              if (PANGO_IS_HB_FONT (font))
                {
                  PangoHbFace *hbface = (PangoHbFace *)face;
                  instance_id = hbface->instance_id;
                }
              else if (PANGO_IS_FC_FONT (font))
                {
                  int index;
                  FcPattern *pattern = pango_fc_font_get_pattern (PANGO_FC_FONT (font));
                  FcPatternGetInteger (pattern, FC_INDEX, 0, (int *)&index);
                  instance_id = (index >> 16) - 1;
                }

              if (pango_font_face_is_variable (face))
                {
                  if (instance_id >= 0)
                    g_print ("    Instance %d\n", instance_id);
                  else if (instance_id == -1)
                    g_print ("    Default Instance\n");
                }

              n_axes = hb_ot_var_get_axis_count (hb_font_get_face (hb_font));
              get_axes_and_values (hb_font, n_axes, axes, coords);

              for (int i = 0; i < n_axes; i++)
                {
                  char name[20];
                  unsigned int namelen = 20;

                  hb_ot_name_get_utf8 (hb_font_get_face (hb_font), axes[i].name_id, HB_LANGUAGE_INVALID, &namelen, name);

                  g_print ("    %s: %g (%g - %g, %g)\n",
                           name,
                           coords[axes[i].axis_index],
                           axes[i].min_value,
                           axes[i].max_value,
                           axes[i].default_value);
                }

              g_object_unref (font);
            }

          if (opt_verbose)
            {
              if (PANGO_IS_HB_FACE (face))
                {
                  PangoHbFace *hbface = (PangoHbFace *)face;

                  if (hbface->file)
                    g_print ("    %d %s\n", hbface->index, hbface->file);
                }
              else
                {
                  PangoFont *font;

                  font = pango_context_load_font (ctx, desc);
                  if (PANGO_IS_FC_FONT (font))
                    {
                      FcPattern *pattern;
                      const char *file;
                      int index;

                      pattern = pango_fc_font_get_pattern (PANGO_FC_FONT (font));
                      FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **)&file);
                      FcPatternGetInteger (pattern, FC_INDEX, 0, &index);
                      g_print ("    %d %s\n", index, file);
                    }
                  g_object_unref (font);
                }
            }

          g_free (desc_str);
          pango_font_description_free (desc);
          g_object_unref (face);
        }
    }
  g_object_unref (ctx);
  g_object_unref (fontmap);

  return 0;
}
