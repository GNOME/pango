/* Pango
 * gen-script-for-lang.c: Utility program to generate pango-script-lang-table.h
 *
 * Copyright (C) 2003 Red Hat, Inc.
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

#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pango/pango-enum-types.h>
#include <pango/pango-script.h>
#include <pango/pango-types.h>

#include <fontconfig/fontconfig.h>

#define MAX_SCRIPTS 3

typedef struct {
  PangoLanguage *lang;
  PangoScript scripts[MAX_SCRIPTS];
} LangInfo;

static const char *get_script_name (PangoScript script)
{
  static GEnumClass *class = NULL;
  GEnumValue *value;
  if (!class)
    class = g_type_class_ref (PANGO_TYPE_SCRIPT);
  
  value = g_enum_get_value (class, script);
  g_assert (value);

  return value->value_name;
}

static void fail (const char *format, ...) G_GNUC_PRINTF (1, 2) G_GNUC_NORETURN;
static void fail (const char *format, ...)
{
  va_list vap;
  
  va_start (vap, format);
  vfprintf (stderr, format, vap);
  va_end (vap);
  
  exit (1);
}

static void
script_for_char (gunichar   ch,
		 LangInfo  *info)
{
  PangoScript script = pango_script_for_unichar (ch);
  if (script != PANGO_SCRIPT_COMMON &&
      script != PANGO_SCRIPT_INHERITED)
    {
      int j;

      if (script == PANGO_SCRIPT_UNKNOWN)
	{
	   g_message ("Script unknown for U+%04X", ch);
	   return;
	}

      for (j = 0; j < MAX_SCRIPTS; j++)
	{
	  if (info->scripts[j] == script)
	    break;
	  if (info->scripts[j] == PANGO_SCRIPT_INVALID_CODE)
	    {
	      info->scripts[j] = script;
	      break;
	    }
	}

      if (j == MAX_SCRIPTS)
	fail ("More than %d scripts found for %s\n", MAX_SCRIPTS, pango_language_to_string (info->lang));
    }
}
     
static void
scripts_for_lang (LangInfo   *info)
{
  const FcCharSet *charset;
  FcChar32  ucs4, pos;
  FcChar32  map[FC_CHARSET_MAP_SIZE];
  int i;

  charset = FcCharSetForLang ((const FcChar8 *) info->lang);
  if (!charset)
    return;

  for (ucs4 = FcCharSetFirstPage (charset, map, &pos);
       ucs4 != FC_CHARSET_DONE;
       ucs4 = FcCharSetNextPage (charset, map, &pos))
    {

      for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
	{
	  FcChar32  bits = map[i];
	  FcChar32  base = ucs4 + i * 32;
	  int b = 0;
	  bits = map[i];
	  while (bits)
	    {
	      if (bits & 1)
		script_for_char (base + b, info);

	      bits >>= 1;
	      b++;
	    }
	}
    }
}

static void
do_lang (GArray        *script_array,
	 const FcChar8 *lang)
{
  LangInfo info;
  int j;

  info.lang = pango_language_from_string ((const char *)lang);

  for (j = 0; j < MAX_SCRIPTS; j++)
    info.scripts[j] = PANGO_SCRIPT_INVALID_CODE;
  
  scripts_for_lang (&info);

  g_array_append_val (script_array, info);
}

static int
compare_lang (gconstpointer a,
	      gconstpointer b)
{
  const LangInfo *info_a = a;
  const LangInfo *info_b = b;

  return strcmp (pango_language_to_string (info_a->lang),
		 pango_language_to_string (info_b->lang));
}

int main (void)
{
  GArray *script_array;
  unsigned int i;
  int j;
  int max_lang_len = 0;
  FcStrList *langs;
  FcChar8* lang;

  g_type_init ();

  script_array = g_array_new (FALSE, FALSE, sizeof (LangInfo));
  

  langs = FcGetLangs ();

  while ((lang = FcStrListNext (langs)))
    do_lang (script_array, lang);

  FcStrListDone (langs);


  g_array_sort (script_array, compare_lang);

  for (i = 0; i < script_array->len; i++)
    {
      LangInfo *info = &g_array_index (script_array, LangInfo, i);
      
      max_lang_len = MAX (max_lang_len,
			  1 + (int)strlen (pango_language_to_string (info->lang)));
    }
      
  g_print ("typedef struct {\n"
	   "  const char lang[%d];\n"
	   "  PangoScript scripts[%d];\n"
	   "} PangoScriptForLang;\n"
	   "\n"
	   "static const PangoScriptForLang pango_script_for_lang[] = {\n",
	   max_lang_len,
	   MAX_SCRIPTS);
  
  for (i = 0; i < script_array->len; i++)
    {
      LangInfo *info = &g_array_index (script_array, LangInfo, i);
      
      g_print ("  { \"%s\", { ", pango_language_to_string (info->lang));
      for (j = 0; j < MAX_SCRIPTS; j++)
	{
	  if (j != 0)
	    g_print (", ");
	  g_print ("%s", get_script_name (info->scripts[j]));
	}
      g_print (" } },\n");
    }

  g_print ("};\n");
  
  
  return 0;
}
