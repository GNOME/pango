#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pango/pango-enum-types.h>
#include <pango/pango-script.h>
#include <pango/pango-types.h>
#include <pango/pango-utils.h>

PangoScript script_for_file (const char *base_dir,
			     const char *file_part);

const char *get_script_name (PangoScript script)
{
  static GEnumClass *class = NULL;
  GEnumValue *value;
  if (!class)
    class = g_type_class_ref (PANGO_TYPE_SCRIPT);
  
  value = g_enum_get_value (class, script);
  g_assert (value);

  return value->value_name;
}

int fail (char *format, ...)
{
  va_list vap;
  
  va_start (vap, format);
  vfprintf (stderr, format, vap);
  va_end (vap);
  
  exit (1);
}

gboolean scan_hex (const char **str, gunichar *result)
{
  const char *end;

  *result = strtol (*str, (char **)&end, 16);
  if (end == *str)
    return FALSE;

  *str = end;
  return TRUE;
}

void warn_mismatch (const char *file_part,
		    PangoScript script1,
		    PangoScript script2)
{
  g_printerr ("%s has characters for both %s and %s\n",
	      file_part, get_script_name (script1), get_script_name (script2));
}

PangoScript script_for_line (const char *base_dir,
			     const char *file_part,
			     const char *str)
{
  gunichar start_char;
  gunichar end_char;
  gunichar ch;
  PangoScript result = PANGO_SCRIPT_COMMON;
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

      result = script_for_file (base_dir, file_part->str);
      g_string_free (file_part, TRUE);

      return result;
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
  if (*p != '\0')
    goto err;

  for (ch = start_char; ch <= end_char; ch++)
    {
      PangoScript script = pango_script_for_unichar (ch);
      if (script != PANGO_SCRIPT_COMMON &&
	  script != PANGO_SCRIPT_INHERITED)
	{
	  if (result != PANGO_SCRIPT_COMMON && script != result)
	    {
	      warn_mismatch (file_part, script, result);
	      return PANGO_SCRIPT_INVALID_CODE;
	    }
	  result = script;
	}
    }

  return result;
  
 err:
  fail ("While processing '%s', cannot parse line: '%s'\n", file_part, str);
  return PANGO_SCRIPT_INVALID_CODE; /* Not reached */
}
     
PangoScript script_for_file (const char *base_dir,
			     const char *file_part)
{
  GError *error = NULL;
  char *filename = g_build_filename (base_dir, file_part, NULL);
  GIOChannel *channel = g_io_channel_new_file (filename, "r", &error);
  GIOStatus status = G_IO_STATUS_NORMAL;
  PangoScript result = PANGO_SCRIPT_COMMON;

  if (!channel)
    fail ("Error opening '%s': %s\n", filename, error);

  /* The files have ISO-8859-1 copyright signs in them */
  if (!g_io_channel_set_encoding (channel, "ISO-8859-1", &error))
      fail ("Cannot set encoding when reading '%s': %s\n", filename, error);
  
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
	  if (str[0] != '\0') 	/* Empty */
	    {
	      PangoScript script = script_for_line (base_dir, file_part, str);
	      if (script != PANGO_SCRIPT_COMMON &&
		  script != PANGO_SCRIPT_INHERITED)
		{
		  if (result != PANGO_SCRIPT_COMMON && script != result)
		    {
		      warn_mismatch (file_part, script, result);
		      result = PANGO_SCRIPT_INVALID_CODE;
		      status = G_IO_STATUS_EOF;
		    }
		  else
		    result = script;
		}
	    }
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
    fail ("Error closing '%s': %s\n", channel, error);

  g_free (filename);

  return result;
}

int main (int argc, char **argv)
{
  GDir *dir;
  GError *error = NULL;

  g_type_init ();

  if (argc != 2)
    fail ("Usage: gen-script-for-lang DIR > script-for-lang.h\n");

  dir = g_dir_open (argv[1], 0, &error);
  if (!dir)
    fail ("%s\n", error->message);

  while (TRUE)
    {
      const char *name = g_dir_read_name (dir);
      if (!name)
	break;

      if (g_str_has_suffix (name, ".orth"))
	{
	  char *langpart = g_strndup (name,
				      strlen (name) - strlen (".orth"));
	  PangoLanguage *lang = pango_language_from_string (langpart);
	  PangoScript script;

	  script = script_for_file (argv[1], name);
	  g_print ("%s - %s\n",
		   pango_language_to_string (lang),
		   get_script_name (script));
	  g_free (langpart);
	}
    }

  g_dir_close (dir);
  
  return 0;
}

