/* Pango
 * pangowin32-utils.c: Miscellaneous Windows-related utils
 *
 * Copyright (C) 2022 the GTK team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "pango-impl-utils.h"
#include "pangowin32-utils-private.h"

#define MAX_ALIAS_LENGTH 11
#define MAX_FAMILY_LENGTH 20

void
pango2_win32_font_map_add_aliases_and_fallbacks (Pango2FontMap *map)
{
  /* list referred from Pango 1.x, from pangowin32-fontmap.c */
  const char sans_families[][MAX_FAMILY_LENGTH] =
    {"dejavu", "sans", "arial", "tahoma",
     "arial unicode ms", "lucida sans unicode",
     "browallia new" };
  const char serif_families[][MAX_FAMILY_LENGTH] =
    {"dejavu serif", "times new roman", "georgia", "angsana new"};
  const char mono_families[][MAX_FAMILY_LENGTH] =
    {"dejavu sans mono", "consolas", "courier new",
     "lucida console", "courier monothai"};

  /* TODO: Add when how to populate fallback fontmap is clearer */
  const char fallback_families[][MAX_FAMILY_LENGTH] =
    {"simsun", "simhei", "mingliu",
     "gulimche", "ms gothic", "sylfaen",
     "kartika", "latha", "mangal", "raavi"};

  const char sans_aliases [][MAX_ALIAS_LENGTH] = {"Sans-serif", "Sans"};
  const char mono_aliases [][MAX_ALIAS_LENGTH] = {"Mono", "Monospace"};

  /* populate families for Sans and Sans-serif aliases */
  for (gsize i = 0; i < G_N_ELEMENTS (sans_families); i++)
    {
      Pango2FontFamily *family = pango2_font_map_get_family (map, sans_families[i]);

      if (family)
        {
          for (gsize j = 0; j < G_N_ELEMENTS (sans_aliases); j++)
            {
              Pango2GenericFamily *alias_family;

              alias_family = pango2_generic_family_new (sans_aliases[j]);
              pango2_generic_family_add_family (alias_family, family);
              pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (alias_family));
            }
        }
    }

  /* populate families for Serif aliases */
  for (gsize i = 0; i < G_N_ELEMENTS (serif_families); i++)
    {
      Pango2FontFamily *family = pango2_font_map_get_family (map, serif_families[i]);

      if (family)
        {
          Pango2GenericFamily *alias_family;

          alias_family = pango2_generic_family_new ("Serif");
          pango2_generic_family_add_family (alias_family, family);
          pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (alias_family));
        }
    }

  /* populate families for Mono and Monospace aliases */
  for (gsize i = 0; i < G_N_ELEMENTS (mono_families); i++)
    {
      Pango2FontFamily *family = pango2_font_map_get_family (map, mono_families[i]);

      if (family)
        {
          for (gsize j = 0; j < G_N_ELEMENTS (mono_aliases); j++)
            {
              Pango2GenericFamily *alias_family;

              alias_family = pango2_generic_family_new (mono_aliases[j]);
              pango2_generic_family_add_family (alias_family, family);
              pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (alias_family));
            }
        }
    }

  /* TODO: populate fallback fontmap */
#if 0
  for (gsize i = 0; i < G_N_ELEMENTS (fallback_families); i++)
    {
      Pango2FontFamily *family = pango2_font_map_get_family (map->fallback, fallback_families[i]);

      if (family)
        {
          Pango2GenericFamily *alias_family;

          alias_family = pango2_generic_family_new (fallback_families[i]);
          pango2_generic_family_add_family (alias_family, family);
          pango2_font_map_add_family (map->fallback, PANGO2_FONT_FAMILY (alias_family));
        }
    }
#endif

  /* Add other generic aliases */
  struct {
    const char *alias_name;
    const char *family_name;
  } aliases[] = {
    { "System-ui",  "Yu Gothic UI" },
    { "System-ui",  "Segoe UI" },
    { "System-ui",  "Meiryo" },
    { "Emoji",      "Segoe UI Emoji" },
    { "Emoji",      "Segoe UI Symbol" },
    { "Emoji",      "Segoe UI" },
    { "Cursive",    "Comic Sans MS" },
    { "Fantasy",    "Gabriola" },
    { "Fantasy",    "Impact" },
  };

#if 0
  if (IsWindows11OrLater ())
    aliases[0].family_name = "Cascadia Mono";
#endif

  for (gsize i = 0; i < G_N_ELEMENTS (aliases); i++)
    {
      Pango2FontFamily *family = pango2_font_map_get_family (map, aliases[i].family_name);

      if (family)
        {
          Pango2GenericFamily *alias_family;

          alias_family = pango2_generic_family_new (aliases[i].alias_name);
          pango2_generic_family_add_family (alias_family, family);
          pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (alias_family));
        }
    }
}

#undef MAX_ALIAS_LENGTH
#undef MAX_FAMILY_LENGTH
