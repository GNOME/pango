/* Pango
 * test-no-fonts.c: Check that lack of fonts is survivable
 *
 * Copyright (C) 2022 Red Hat, Inc
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

#include <glib.h>
#include <string.h>
#include <locale.h>

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#include "config.h"
#include <pango/pangocairo.h>
#include <pango/pangocairo-fc.h>
#include <pango/pangofc-fontmap.h>
#include "test-common.h"


static PangoFontMap *
generate_font_map (const char *dir)
{
  FcConfig *config;
  PangoFontMap *map;
  char *path;
  gsize len;
  char *conf;

  map = g_object_new (PANGO_TYPE_CAIRO_FC_FONT_MAP, NULL);

  config = FcConfigCreate ();

  path = g_build_filename (dir, "fonts.conf", NULL);
  g_file_get_contents (path, &conf, &len, NULL);

  if (!FcConfigParseAndLoadFromMemory (config, (const FcChar8 *) conf, TRUE))
    g_error ("Failed to parse fontconfig configuration");

  g_free (conf);
  g_free (path);

  FcConfigAppFontAddDir (config, (const FcChar8 *) dir);
  pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (map), config);
  FcConfigDestroy (config);

  return map;
}

static void
test_nofonts (void)
{
  char *path;
  PangoFontMap *fontmap;
  PangoContext *context;
  const char *text;
  PangoLayout *layout;
  int missing;

  path = g_test_build_filename (G_TEST_DIST, "nofonts", NULL);
  fontmap = generate_font_map (path);
  g_free (path);

  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  text = "Schlaf, Kindlein schlaf! "
         "Der Vater hüt' die Schaf, "
         "die Mutter schúttelt's Bäumelein, "
         "da fällt herab ein Träumelein. "
         "Schlaf, Kindlein schlaf!";

  pango_layout_set_text (layout, text, -1);
  missing = pango_layout_get_unknown_glyphs_count (layout);
  g_assert_cmpint (missing, ==, g_utf8_strlen (text, -1));

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/nofonts", test_nofonts);

  return g_test_run ();
}
