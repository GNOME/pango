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
#include <glib/gstdio.h>

int
main (int    argc,
      char **argv)
{
  PangoFontMap *fontmap;
  PangoFontFamily **families;
  int n_families;
  int i, j, k;

#if !GLIB_CHECK_VERSION (2, 35, 3)
  g_type_init();
#endif
  g_set_prgname ("pango-list");

  /* Use PangoCairo to get default fontmap so it works on every platform. */
  fontmap = pango_cairo_font_map_get_default ();

  pango_font_map_list_families (fontmap, &families, &n_families);
  for (i = 0; i < n_families; i++)
    {
      PangoFontFace **faces;
      int n_faces;

      const char *family_name = pango_font_family_get_name (families[i]);

      pango_font_family_list_faces (families[i], &faces, &n_faces);
      for (j = 0; j < n_faces; j++)
	{
	  int *sizes;
	  int n_sizes;

	  const char *face_name = pango_font_face_get_face_name (faces[j]);
	  gboolean is_synth = pango_font_face_is_synthesized (faces[j]);
	  const char *synth_str = is_synth ? "*" : "";
	  PangoFontDescription *desc = NULL;/*pango_font_face_describe (faces[j]);*/

	  pango_font_face_list_sizes (faces[j], &sizes, &n_sizes);
	  if (!n_sizes)
	    g_print ("%s%s, %s\n", synth_str, family_name, face_name);
	  else
	    for (k = 0; k < n_sizes; k++)
	      g_print ("%s%s, %s, %g\n", synth_str, family_name, face_name, pango_units_to_double (sizes[k]));

	  g_free (sizes);
	  pango_font_description_free (desc);
	}

      g_free (faces);
    }
  g_free (families);
  g_object_unref (fontmap);

  return 0;
}
