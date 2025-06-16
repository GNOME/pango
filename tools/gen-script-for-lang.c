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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pango/pango-enum-types.h>
#include <pango/pango-script.h>
#include <pango/pango-types.h>

#include <fontconfig/fontconfig.h>

#define MAX_SCRIPTS 3

typedef struct {
  GUnicodeScript script;
  int freq;
} ScriptInfo;

typedef struct {
  PangoLanguage *lang;
  ScriptInfo scripts[MAX_SCRIPTS];
} LangInfo;

static const char *get_script_name (GUnicodeScript script)
{
  static GEnumClass *class = NULL;
  GEnumValue *value;
  if (!class)
    class = g_type_class_ref (G_TYPE_UNICODE_SCRIPT);
  
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
  GUnicodeScript script = g_unichar_get_script (ch);
  if (script != G_UNICODE_SCRIPT_COMMON &&
      script != G_UNICODE_SCRIPT_INHERITED)
    {
      int j;

      if (script == G_UNICODE_SCRIPT_UNKNOWN)
        {
           g_message ("Script unknown for U+%04X", ch);
           return;
        }

      for (j = 0; j < MAX_SCRIPTS; j++)
        {
          if (info->scripts[j].script == script)
            break;
          if (info->scripts[j].script == G_UNICODE_SCRIPT_COMMON)
            {
              info->scripts[j].script = script;
              break;
            }
        }

      if (j == MAX_SCRIPTS)
        fail ("More than %d scripts found for %s.  Increase MAX_SCRIPTS.\n", MAX_SCRIPTS, pango_language_to_string (info->lang));

      info->scripts[j].freq++;
    }
}
     
static void
scripts_for_lang (LangInfo   *info)
{
  const FcCharSet *charset;
  FcChar32  ucs4, pos;
  FcChar32  map[FC_CHARSET_MAP_SIZE];
  int i;

  charset = FcLangGetCharSet ((const FcChar8 *) info->lang);
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
    {
      info.scripts[j].script = G_UNICODE_SCRIPT_COMMON;
      info.scripts[j].freq = 0;
    }
  
  scripts_for_lang (&info);

  g_array_append_val (script_array, info);
}

static int
compare_script (gconstpointer a,
                gconstpointer b,
                gpointer      data)
{
  const ScriptInfo *info_a = a;
  const ScriptInfo *info_b = b;
  G_GNUC_UNUSED LangInfo *lang_info = data;

  /* first compare frequencies, higher first */
  if (info_a->freq > info_b->freq)
    return -1;
  if (info_a->freq < info_b->freq)
    return +1;

  /* next compare script indices, higher first (it's more specific) */
  if (info_a->script > info_b->script)
    return -1;
  if (info_a->script < info_b->script)
    return +1;

  /* for stability, next compare pointers themselves, smaller first */
  if (info_a < info_b)
    return -1;
  if (info_a > info_b)
    return +1;

  return 0;
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
  int max_script_len = 0;

  FcStrSet *langs_set;
  FcStrList *langs;
  FcChar8* lang;

  char date_buf[200];
  const char *date_str = "unknown";
  time_t t;
  struct tm *tmp;
  int fc_version;

  script_array = g_array_new (FALSE, FALSE, sizeof (LangInfo));
  

  langs_set = FcGetLangs ();
  langs = FcStrListCreate (langs_set);
  FcStrSetDestroy (langs_set);

  while ((lang = FcStrListNext (langs)))
    do_lang (script_array, lang);

  FcStrListDone (langs);


  g_array_sort (script_array, compare_lang);

  for (i = 0; i < script_array->len; i++)
    {
      LangInfo *info = &g_array_index (script_array, LangInfo, i);

      max_lang_len = MAX (max_lang_len,
                          (int)strlen (pango_language_to_string (info->lang)));

      g_sort_array (info->scripts,
                    G_N_ELEMENTS (info->scripts),
                    sizeof (info->scripts[0]),
                    compare_script,
                    info);

      for (j = 0; j < MAX_SCRIPTS; j++)
        if (!info->scripts[j].freq)
          break;
      
      max_script_len = MAX (max_script_len, j);
    }

  if ((t = time(NULL), tmp = localtime (&t)) && strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tmp))
    date_str = date_buf;

  fc_version = FcGetVersion ();

  g_print ("/* pango-script-lang-table.h:\n"
           " * \n"
           " * Generated by %s\n"
           " * Date: %s\n"
           " * Source: fontconfig-%d.%d.%d\n"
           " * \n"
           " * Do not edit.\n"
           " */\n",
           __FILE__,
           date_str,
           fc_version / 10000, (fc_version / 100) % 100, fc_version % 100);

  g_print ("typedef struct _PangoScriptForLang {\n"
           "  const char lang[%d];\n"
           "  GUnicodeScript scripts[%d];\n"
           "} PangoScriptForLang;\n"
           "\n"
           "static const PangoScriptForLang pango_script_for_lang[] = {\n",
           max_lang_len + 1,
           max_script_len);
  
  for (i = 0; i < script_array->len; i++)
    {
      LangInfo *info = &g_array_index (script_array, LangInfo, i);
      
      g_print ("  { \"%s\", %*s{ ",
               pango_language_to_string (info->lang),
               (int)(max_lang_len - strlen (pango_language_to_string (info->lang))), "");
      for (j = 0; j < MAX_SCRIPTS; j++)
        {
          if (!info->scripts[j].freq)
            {
              /* Avoid generating code with empty arrays */
              if (j == 0)
                g_print ("0");

              break;
            }

          if (j != 0)
            g_print (", ");
          g_print ("%s/*%d*/",
                   get_script_name (info->scripts[j].script),
                   info->scripts[j].freq);
        }
      g_print (" } }");
      if (i + 1 != script_array->len)
        g_print (",");
      g_print ("\n");
    }

  g_print ("};\n");

  return 0;
}
