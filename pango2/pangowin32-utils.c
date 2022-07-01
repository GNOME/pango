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

void
pango2_win32_font_map_add_aliases_and_fallbacks (Pango2FontMap *map)
{
  /* Add generic aliases */
  struct {
    const char *alias_name;
    const char *family_name;
  } aliases[] = {
    { "Monospace",  "Consolas" },
    { "Sans-serif", "Arial" },
    { "Sans",       "Arial" },
    { "Serif",      "Times New Roman" },
    { "System-ui",  "Segoe UI" },
    { "Emoji",      "Segoe UI Emoji" }
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
