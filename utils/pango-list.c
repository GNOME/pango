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
      const char *kind;

      const char *family_name = pango_font_family_get_name (families[i]);
      if (pango_font_family_is_monospace (families[i]))
        {
          if (pango_font_family_is_variable (families[i]))
            kind = "(monospace, variable)";
          else
            kind = "(monospace)";
        }
      else
        {
          if (pango_font_family_is_variable (families[i]))
            kind = "(variable)";
          else
            kind = "";
        }

      g_print ("%s %s\n", family_name, kind);

      pango_font_family_list_faces (families[i], &faces, &n_faces);
      for (j = 0; j < n_faces; j++)
	{
	  const char *face_name = pango_font_face_get_face_name (faces[j]);
	  gboolean is_synth = pango_font_face_is_synthesized (faces[j]);
	  const char *synth_str = is_synth ? "*" : "";
	  g_print ("	%s%s\n", synth_str, face_name);

	  if (0)
	  {
	    int *sizes;
	    int n_sizes;
	    pango_font_face_list_sizes (faces[j], &sizes, &n_sizes);
	    if (n_sizes)
	      {
		g_print ("		{");
		for (k = 0; k < n_sizes; k++)
		  {
		    if (k)
		      g_print (", ");
		    g_print ("%g", pango_units_to_double (sizes[k]));
		  }
		g_print ("}\n");
	      }
	    g_free (sizes);
	  }

	  if (1)
	  {
	    PangoFontDescription *desc = pango_font_face_describe (faces[j]);
	    char *desc_str = pango_font_description_to_string (desc);

	    g_print ("		\"%s\"\n", desc_str);

	    g_free (desc_str);
	    pango_font_description_free (desc);
	  }
	}

      g_free (faces);
    }
  g_free (families);
  g_object_unref (fontmap);

  return 0;
}
