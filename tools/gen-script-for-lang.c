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

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pango/pango-enum-types.h>
#include <pango/pango-script.h>
#include <pango/pango-types.h>
#include <pango/pango-utils.h>

#define MAX_SCRIPTS 3

typedef struct {
  PangoLanguage *lang;
  PangoScript scripts[MAX_SCRIPTS];
} LangInfo;

static void scripts_for_file (const char *base_dir,
			      const char *file_part,
			      LangInfo   *info);

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

static gboolean scan_hex (const char **str, gunichar *result)
{
  const char *end;

  *result = strtol (*str, (char **)&end, 16);
  if (end == *str)
    return FALSE;

  *str = end;
  return TRUE;
}

static void
scripts_for_line (const char  *base_dir,
		  const char  *file_part,
		  const char  *str,
		  LangInfo    *info)
{
  gunichar start_char;
  gunichar end_char;
  gunichar ch;
  const char *p = str;

  if (g_str_has_prefix (str, "include"))
    {
      GString *file_part = g_string_new (NULL);
      
      str += strlen ("include");
      if (!pango_skip_space (&str))
	goto err;

      if (!pango_scan_string (&str, file_part) ||
	  pango_skip_space (&str))
	goto err;

      scripts_for_file (base_dir, file_part->str, info);
      g_string_free (file_part, TRUE);

      return;
    }
  
  /* Format is HEX_DIGITS or HEX_DIGITS-HEX_DIGITS */
  if (!scan_hex (&p, &start_char))
    goto err;
  
  end_char = start_char;

  pango_skip_space (&p);
  if (*p == '-')
    {
      p++;
      if (!scan_hex (&p, &end_char))
	goto err;

      pango_skip_space (&p);
    }

  /* The rest of the line is ignored */
  /*
  if (*p != '\0')
    goto err;
   */

  for (ch = start_char; ch <= end_char; ch++)
    {
      PangoScript script = pango_script_for_unichar (ch);
      if (script != PANGO_SCRIPT_COMMON &&
	  script != PANGO_SCRIPT_INHERITED)
	{
	  int j;

	  if (script == PANGO_SCRIPT_UNKNOWN)
	    {
	       g_message ("Script unknown for U+%04X", ch);
	       continue;
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
	    fail ("More than %d scripts found for %s\n", MAX_SCRIPTS, file_part);
	}
    }

  return;
  
 err:
  fail ("While processing '%s', cannot parse line: '%s'\n", file_part, str);
  return; /* Not reached */
}
     
static void
scripts_for_file (const char *base_dir,
		  const char *file_part,
		  LangInfo   *info)
{
  GError *error = NULL;
  char *filename = g_build_filename (base_dir, file_part, NULL);
  GIOChannel *channel = g_io_channel_new_file (filename, "r", &error);
  GIOStatus status = G_IO_STATUS_NORMAL;

  if (!channel)
    fail ("Error opening '%s': %s\n", filename, error->message);

  /* The files have ISO-8859-1 copyright signs in them */
  if (!g_io_channel_set_encoding (channel, "ISO-8859-1", &error))
      fail ("Cannot set encoding when reading '%s': %s\n", filename, error->message);
  
  while (status == G_IO_STATUS_NORMAL)
    {
      char *str;
      size_t term;
      char *comment;
      
      status = g_io_channel_read_line  (channel, &str, NULL, &term, &error);
      switch (status)
	{
	case G_IO_STATUS_NORMAL:
	  str[term] = '\0';
	  comment = strchr (str, '#');
	  if (comment)
	    *comment = '\0';
	  g_strstrip (str);
	  if (str[0] != '\0')	/* Empty */
	    scripts_for_line (base_dir, file_part, str, info);
	  g_free (str);
	  break;
	case G_IO_STATUS_EOF:
	  break;
	case G_IO_STATUS_ERROR:
	  fail ("Error reading '%s': %s\n", filename, error->message);
	  break;
	case G_IO_STATUS_AGAIN:
	  g_assert_not_reached ();
	  break;
	}
    }

  if (!g_io_channel_shutdown (channel, FALSE, &error))
    fail ("Error closing '%s': %s\n", filename, error->message);

  g_free (filename);
}

static void
do_file (GArray      *script_array,
	 const char  *base_dir,
	 const char  *file_part)
{
  char *langpart;
  LangInfo info;
  int j;

  langpart = g_strndup (file_part, strlen (file_part) - strlen (".orth"));
  info.lang = pango_language_from_string (langpart);
  g_free (langpart);

  for (j = 0; j < MAX_SCRIPTS; j++)
    info.scripts[j] = PANGO_SCRIPT_INVALID_CODE;
  
  scripts_for_file (base_dir, file_part, &info);

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

int main (int argc, char **argv)
{
  GDir *dir;
  GError *error = NULL;
  GArray *script_array;
  unsigned int i;
  int j;
  int max_lang_len = 0;

  g_type_init ();

  if (argc != 2)
    fail ("Usage: gen-script-for-lang DIR > script-for-lang.h\n");

  dir = g_dir_open (argv[1], 0, &error);
  if (!dir)
    fail ("%s\n", error->message);

  script_array = g_array_new (FALSE, FALSE, sizeof (LangInfo));
  
  while (TRUE)
    {
      const char *name = g_dir_read_name (dir);
      if (!name)
	break;

      if (g_str_has_suffix (name, ".orth"))
	do_file (script_array, argv[1], name);
    }

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
  
  
  g_dir_close (dir);
  
  return 0;
}
