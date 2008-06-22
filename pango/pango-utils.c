/* Pango
 * pango-utils.c: Utilities for internal functions and modules
 *
 * Copyright (C) 2000 Red Hat Software
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

#include "pango-font.h"
#include "pango-features.h"
#include "pango-impl-utils.h"

#include <glib/gstdio.h>

#ifndef HAVE_FLOCKFILE
#  define flockfile(f) (void)1
#  define funlockfile(f) (void)1
#  define getc_unlocked(f) getc(f)
#endif /* !HAVE_FLOCKFILE */

#ifdef G_OS_WIN32

#include <sys/types.h>

#define STRICT
#include <windows.h>

#endif

struct PangoAlias
{
  char *alias;
  int n_families;
  char **families;
  gboolean visible; /* Do we want/need this? */
};

static GHashTable *pango_aliases_ht = NULL;

/**
 * pango_version:
 *
 * This is similar to the macro %PANGO_VERSION except that
 * it returns the encoded version of Pango available at run-time,
 * as opposed to the version available at compile-time.
 *
 * A version number can be encoded into an integer using
 * PANGO_VERSION_ENCODE().
 *
 * Returns: The encoded version of Pango library
 *   available at run time.
 *
 * Since: 1.16
 **/
int
pango_version (void)
{
  return PANGO_VERSION;
}

/**
 * pango_version_string:
 *
 * This is similar to the macro %PANGO_VERSION_STRING except that
 * it returns the version of Pango available at run-time, as opposed to
 * the version available at compile-time.
 *
 * Returns: A string containing the version of Pango library
 *   available at run time.
 *   The returned string is owned by Pango and should not be modified
 *   or freed.
 *
 * Since: 1.16
 **/
const char *
pango_version_string (void)
{
  return PANGO_VERSION_STRING;
}

/**
 * pango_version_check:
 * @required_major: the required major version.
 * @required_minor: the required minor version.
 * @required_micro: the required major version.
 *
 * Checks that the Pango library in use is compatible with the
 * given version. Generally you would pass in the constants
 * %PANGO_VERSION_MAJOR, %PANGO_VERSION_MINOR, %PANGO_VERSION_MICRO
 * as the three arguments to this function; that produces
 * a check that the library in use at run-time is compatible with
 * the version of Pango the application or module was compiled against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * For compile-time version checking use PANGO_VERSION_CHECK().
 *
 * Return value: %NULL if the Pango library is compatible with the
 *   given version, or a string describing the version mismatch.
 *   The returned string is owned by Pango and should not be modified
 *   or freed.
 *
 * Since: 1.16
 **/
const gchar*
pango_version_check (int required_major,
		     int required_minor,
		     int required_micro)
{
  gint pango_effective_micro = 100 * PANGO_VERSION_MINOR + PANGO_VERSION_MICRO;
  gint required_effective_micro = 100 * required_minor + required_micro;

  if (required_major < PANGO_VERSION_MAJOR)
    return "Pango version too new (major mismatch)";
  if (required_effective_micro < pango_effective_micro - PANGO_BINARY_AGE)
    return "Pango version too new (micro mismatch)";
  if (required_effective_micro > pango_effective_micro)
    return "Pango version too old (micro mismatch)";
  return NULL;
}

/**
 * pango_trim_string:
 * @str: a string
 *
 * Trims leading and trailing whitespace from a string.
 *
 * Return value: A newly-allocated string that must be freed with g_free()
 **/
char *
pango_trim_string (const char *str)
{
  int len;

  g_return_val_if_fail (str != NULL, NULL);

  while (*str && g_ascii_isspace (*str))
    str++;

  len = strlen (str);
  while (len > 0 && g_ascii_isspace (str[len-1]))
    len--;

  return g_strndup (str, len);
}

/**
 * pango_split_file_list:
 * @str: a %G_SEARCHPATH_SEPARATOR separated list of filenames
 *
 * Splits a %G_SEARCHPATH_SEPARATOR-separated list of files, stripping
 * white space and substituting ~/ with $HOME/.
 *
 * Return value: a list of strings to be freed with g_strfreev()
 **/
char **
pango_split_file_list (const char *str)
{
  int i = 0;
  int j;
  char **files;

  files = g_strsplit (str, G_SEARCHPATH_SEPARATOR_S, -1);

  while (files[i])
    {
      char *file = pango_trim_string (files[i]);

      /* If the resulting file is empty, skip it */
      if (file[0] == '\0')
	{
	  g_free(file);
	  g_free (files[i]);

	  for (j = i + 1; files[j]; j++)
	    files[j - 1] = files[j];

	  files[j - 1] = NULL;

	  continue;
	}
#ifndef G_OS_WIN32
      /* '~' is a quite normal and common character in file names on
       * Windows, especially in the 8.3 versions of long file names, which
       * still occur now and then. Also, few Windows user are aware of the
       * Unix shell convention that '~' stands for the home directory,
       * even if they happen to have a home directory.
       */
      if (file[0] == '~' && file[1] == G_DIR_SEPARATOR)
	{
	  char *tmp = g_strconcat (g_get_home_dir(), file + 1, NULL);
	  g_free (file);
	  file = tmp;
	}
      else if (file[0] == '~' && file[1] == '\0')
	{
	  g_free (file);
	  file = g_strdup (g_get_home_dir());
	}
#endif
      g_free (files[i]);
      files[i] = file;

      i++;
    }

  return files;
}

/**
 * pango_read_line:
 * @stream: a stdio stream
 * @str: #GString buffer into which to write the result
 *
 * Reads an entire line from a file into a buffer. Lines may
 * be delimited with '\n', '\r', '\n\r', or '\r\n'. The delimiter
 * is not written into the buffer. Text after a '#' character is treated as
 * a comment and skipped. '\' can be used to escape a # character.
 * '\' proceeding a line delimiter combines adjacent lines. A '\' proceeding
 * any other character is ignored and written into the output buffer
 * unmodified.
 *
 * Return value: 0 if the stream was already at an %EOF character, otherwise
 *               the number of lines read (this is useful for maintaining
 *               a line number counter which doesn't combine lines with '\')
 **/
gint
pango_read_line (FILE *stream, GString *str)
{
  gboolean quoted = FALSE;
  gboolean comment = FALSE;
  int n_read = 0;
  int lines = 1;

  flockfile (stream);

  g_string_truncate (str, 0);

  while (1)
    {
      int c;

      c = getc_unlocked (stream);

      if (c == EOF)
	{
	  if (quoted)
	    g_string_append_c (str, '\\');

	  goto done;
	}
      else
	n_read++;

      if (quoted)
	{
	  quoted = FALSE;

	  switch (c)
	    {
	    case '#':
	      g_string_append_c (str, '#');
	      break;
	    case '\r':
	    case '\n':
	      {
		int next_c = getc_unlocked (stream);

		if (!(next_c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);

		lines++;

		break;
	      }
	    default:
	      g_string_append_c (str, '\\');
	      g_string_append_c (str, c);
	    }
	}
      else
	{
	  switch (c)
	    {
	    case '#':
	      comment = TRUE;
	      break;
	    case '\\':
	      if (!comment)
		quoted = TRUE;
	      break;
	    case '\n':
	      {
		int next_c = getc_unlocked (stream);

		if (!(c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);

		goto done;
	      }
	    default:
	      if (!comment)
		g_string_append_c (str, c);
	    }
	}
    }

 done:

  funlockfile (stream);

  return (n_read > 0) ? lines : 0;
}

/**
 * pango_skip_space:
 * @pos: in/out string position
 *
 * Skips 0 or more characters of white space.
 *
 * Return value: %FALSE if skipping the white space leaves
 * the position at a '\0' character.
 **/
gboolean
pango_skip_space (const char **pos)
{
  const char *p = *pos;

  while (g_ascii_isspace (*p))
    p++;

  *pos = p;

  return !(*p == '\0');
}

/**
 * pango_scan_word:
 * @pos: in/out string position
 * @out: a #GString into which to write the result
 *
 * Scans a word into a #GString buffer. A word consists
 * of [A-Za-z_] followed by zero or more [A-Za-z_0-9]
 * Leading white space is skipped.
 *
 * Return value: %FALSE if a parse error occurred.
 **/
gboolean
pango_scan_word (const char **pos, GString *out)
{
  const char *p = *pos;

  while (g_ascii_isspace (*p))
    p++;

  if (!((*p >= 'A' && *p <= 'Z') ||
	(*p >= 'a' && *p <= 'z') ||
	*p == '_'))
    return FALSE;

  g_string_truncate (out, 0);
  g_string_append_c (out, *p);
  p++;

  while ((*p >= 'A' && *p <= 'Z') ||
	 (*p >= 'a' && *p <= 'z') ||
	 (*p >= '0' && *p <= '9') ||
	 *p == '_')
    {
      g_string_append_c (out, *p);
      p++;
    }

  *pos = p;

  return TRUE;
}

/**
 * pango_scan_string:
 * @pos: in/out string position
 * @out: a #GString into which to write the result
 *
 * Scans a string into a #GString buffer. The string may either
 * be a sequence of non-white-space characters, or a quoted
 * string with '"'. Instead a quoted string, '\"' represents
 * a literal quote. Leading white space outside of quotes is skipped.
 *
 * Return value: %FALSE if a parse error occurred.
 **/
gboolean
pango_scan_string (const char **pos, GString *out)
{
  const char *p = *pos;

  while (g_ascii_isspace (*p))
    p++;

  if (!*p)
    return FALSE;
  else if (*p == '"')
    {
      gboolean quoted = FALSE;
      g_string_truncate (out, 0);

      p++;

      while (TRUE)
	{
	  if (quoted)
	    {
	      int c = *p;

	      switch (c)
		{
		case '\0':
		  return FALSE;
		case 'n':
		  c = '\n';
		  break;
		case 't':
		  c = '\t';
		  break;
		default:
		  break;
		}

	      quoted = FALSE;
	      g_string_append_c (out, c);
	    }
	  else
	    {
	      switch (*p)
		{
		case '\0':
		  return FALSE;
		case '\\':
		  quoted = TRUE;
		  break;
		case '"':
		  p++;
		  goto done;
		default:
		  g_string_append_c (out, *p);
		  break;
		}
	    }
	  p++;
	}
    done:
      ;
    }
  else
    {
      g_string_truncate (out, 0);

      while (*p && !g_ascii_isspace (*p))
	{
	  g_string_append_c (out, *p);
	  p++;
	}
    }

  *pos = p;

  return TRUE;
}

/**
 * pango_scan_int:
 * @pos: in/out string position
 * @out: an int into which to write the result
 *
 * Scans an integer.
 * Leading white space is skipped.
 *
 * Return value: %FALSE if a parse error occurred.
 **/
gboolean
pango_scan_int (const char **pos, int *out)
{
  char *end;
  long temp;

  errno = 0;
  temp = strtol (*pos, &end, 10);
  if (errno == ERANGE)
    {
      errno = 0;
      return FALSE;
    }

  *out = (int)temp;
  if ((long)(*out) != temp)
    {
      return FALSE;
    }

  *pos = end;

  return TRUE;
}

static GHashTable *config_hash = NULL;

static void
read_config_file (const char *filename, gboolean enoent_error)
{
  GKeyFile *key_file = g_key_file_new();
  GError *key_file_error = NULL;
  gchar **groups;
  gsize groups_count = 0;
  guint group_index;

  if (!g_key_file_load_from_file(key_file,filename, 0, &key_file_error))
    {
      if (key_file_error)
	{
	  if (key_file_error->domain != G_FILE_ERROR || key_file_error->code != G_FILE_ERROR_NOENT || enoent_error)
	    {
	      g_warning ("error opening config file '%s': %s\n",
			  filename, key_file_error->message);
	    }
	  g_error_free(key_file_error);
	}
      g_key_file_free(key_file);
      return;
    }

  groups = g_key_file_get_groups (key_file, &groups_count);
  for (group_index = 0; group_index < groups_count; group_index++)
    {
      gsize keys_count = 0;
      const gchar *group = groups[group_index];
      GError *keys_error = NULL;
      gchar **keys;

      keys = g_key_file_get_keys(key_file, group, &keys_count, &keys_error);

      if (keys)
	{
	  guint key_index;

	  for (key_index = 0; key_index < keys_count; key_index++)
	    {
	      const gchar *key = keys[key_index];
	      GError *key_error = NULL;
	      gchar *value =  g_key_file_get_value(key_file, group, key, &key_error);
	      if (value != NULL)
		{
		  g_hash_table_insert (config_hash,
				       g_strdup_printf ("%s/%s", group, key),
				       value);
		}
	      if (key_error)
		{
		  g_warning ("error getting key '%s/%s' in config file '%s'\n",
			     group, key, filename);
		  g_error_free(key_error);
		}
	    }
	  g_strfreev(keys);
	}

      if (keys_error)
	{
	  g_warning ("error getting keys in group '%s' of config file '%s'\n",
		     filename, group);
	  g_error_free(keys_error);
	}
    }
  g_strfreev(groups);
  g_key_file_free(key_file);
}

static void
read_config (void)
{
  if (!config_hash)
    {
      char *filename;
      const char *home;
      const char *envvar;

      config_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					   (GDestroyNotify)g_free,
					   (GDestroyNotify)g_free);
      filename = g_build_filename (pango_get_sysconf_subdirectory (),
				   "pangorc",
				   NULL);
      read_config_file (filename, FALSE);
      g_free (filename);

      home = g_get_home_dir ();
      if (home && *home)
	{
	  filename = g_build_filename (home, ".pangorc", NULL);
	  read_config_file (filename, FALSE);
	  g_free (filename);
	}

      envvar = g_getenv ("PANGO_RC_FILE");
      if (envvar)
	read_config_file (envvar, TRUE);
    }
}

/**
 * pango_config_key_get:
 * @key: Key to look up, in the form "SECTION/KEY".
 *
 * Looks up a key in the Pango config database
 * (pseudo-win.ini style, read from $sysconfdir/pango/pangorc,
 *  ~/.pangorc, and getenv (PANGO_RC_FILE).)
 *
 * Return value: the value, if found, otherwise %NULL. The value is a
 * newly-allocated string and must be freed with g_free().
 **/
char *
pango_config_key_get (const char *key)
{
  g_return_val_if_fail (key != NULL, NULL);

  read_config ();

  return g_strdup (g_hash_table_lookup (config_hash, key));
}

#ifdef G_OS_WIN32

/* DllMain function needed to tuck away the DLL name */

G_WIN32_DLLMAIN_FOR_DLL_NAME(static, dll_name)
#endif

/**
 * pango_get_sysconf_subdirectory:
 *
 * On Unix, returns the name of the "pango" subdirectory of SYSCONFDIR
 * (which is set at compile time). On Win32, returns a subdirectory of
 * the Pango installation directory (which is deduced at run time from
 * the DLL's location, or stored in the Registry).
 *
 * Return value: the Pango sysconf directory. The returned string should
 * not be freed.
 */
G_CONST_RETURN char *
pango_get_sysconf_subdirectory (void)
{
#ifdef G_OS_WIN32
  static gchar *result = NULL;

  if (result == NULL)
    result = g_win32_get_package_installation_subdirectory
      (PACKAGE " " VERSION, dll_name, "etc\\pango");

  return result;
#else
  return SYSCONFDIR "/pango";
#endif
}

/**
 * pango_get_lib_subdirectory:
 *
 * On Unix, returns the name of the "pango" subdirectory of LIBDIR
 * (which is set at compile time). On Win32, returns the Pango
 * installation directory (which is deduced at run time from the DLL's
 * location, or stored in the Registry). The returned string should
 * not be freed.
 *
 * Return value: the Pango lib directory. The returned string should
 * not be freed.
 */
G_CONST_RETURN char *
pango_get_lib_subdirectory (void)
{
#ifdef G_OS_WIN32
  static gchar *result = NULL;

  if (result == NULL)
    result = g_win32_get_package_installation_subdirectory
      (PACKAGE " " VERSION, dll_name, "lib\\pango");

  return result;
#else
  return LIBDIR "/pango";
#endif
}


/**
 * pango_parse_enum:
 * @type: enum type to parse, eg. %PANGO_TYPE_ELLIPSIZE_MODE.
 * @str: string to parse.  May be %NULL.
 * @value: integer to store the result in, or %NULL.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 * @possible_values: place to store list of possible values on failure, or %NULL.
 *
 * Parses an enum type and stored the result in @value.
 *
 * If @str does not match the nick name of any of the possible values for the
 * enum, %FALSE is returned, a warning is issued if @warn is %TRUE, and a
 * string representing the list of possible values is stored in
 * @possible_values.  The list is slash-separated, eg.
 * "none/start/middle/end".  If failed and @possible_values is not %NULL,
 * returned string should be freed using g_free().
 *
 * Return value: %TRUE if @str was successfully parsed.
 *
 * Since: 1.16
 **/
gboolean
pango_parse_enum (GType       type,
		  const char *str,
		  int        *value,
		  gboolean    warn,
		  char      **possible_values)
{
  GEnumClass *class = NULL;
  gboolean ret = TRUE;
  GEnumValue *v = NULL;

  class = g_type_class_ref (type);

  if (G_LIKELY (str))
    v = g_enum_get_value_by_nick (class, str);

  if (v)
    {
      if (G_LIKELY (value))
	*value = v->value;
    }
  else
    {
      ret = FALSE;
      if (warn || possible_values)
	{
	  int i;
	  GString *s = g_string_new (NULL);

	  for (i = 0, v = g_enum_get_value (class, i); v;
	       i++  , v = g_enum_get_value (class, i))
	    {
	      if (i)
		g_string_append_c (s, '/');
	      g_string_append (s, v->value_nick);
	    }

	  if (warn)
	    g_warning ("%s must be one of %s",
		       G_ENUM_CLASS_TYPE_NAME(class),
		       s->str);

	  if (possible_values)
	    *possible_values = s->str;

	  g_string_free (s, possible_values ? FALSE : TRUE);
	}
    }

  g_type_class_unref (class);

  return ret;
}

/**
 * pango_parse_style:
 * @str: a string to parse.
 * @style: a #PangoStyle to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font style. The allowed values are "normal",
 * "italic" and "oblique", case variations being
 * ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
pango_parse_style (const char *str,
		   PangoStyle *style,
		   gboolean    warn)
{
  if (*str == '\0')
    return FALSE;

  switch (str[0])
    {
    case 'n':
    case 'N':
      if (g_ascii_strcasecmp (str, "normal") == 0)
	{
	  *style = PANGO_STYLE_NORMAL;
	  return TRUE;
	}
      break;
    case 'i':
    case 'I':
      if (g_ascii_strcasecmp (str, "italic") == 0)
	{
	  *style = PANGO_STYLE_ITALIC;
	  return TRUE;
	}
      break;
    case 'o':
    case 'O':
      if (g_ascii_strcasecmp (str, "oblique") == 0)
	{
	  *style = PANGO_STYLE_OBLIQUE;
	  return TRUE;
	}
      break;
    }
  if (warn)
    g_warning ("style must be normal, italic, or oblique");

  return FALSE;
}

/**
 * pango_parse_variant:
 * @str: a string to parse.
 * @variant: a #PangoVariant to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font variant. The allowed values are "normal"
 * and "smallcaps" or "small_caps", case variations being
 * ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
pango_parse_variant (const char   *str,
		     PangoVariant *variant,
		     gboolean	   warn)
{
  if (*str == '\0')
    return FALSE;

  switch (str[0])
    {
    case 'n':
    case 'N':
      if (g_ascii_strcasecmp (str, "normal") == 0)
	{
	  *variant = PANGO_VARIANT_NORMAL;
	  return TRUE;
	}
      break;
    case 's':
    case 'S':
      if (g_ascii_strcasecmp (str, "small_caps") == 0 ||
	  g_ascii_strcasecmp (str, "smallcaps") == 0)
	{
	  *variant = PANGO_VARIANT_SMALL_CAPS;
	  return TRUE;
	}
      break;
    }

  if (warn)
    g_warning ("variant must be normal or small_caps");
  return FALSE;
}

/**
 * pango_parse_weight:
 * @str: a string to parse.
 * @weight: a #PangoWeight to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font weight. The allowed values are "heavy",
 * "ultrabold", "bold", "normal", "light", "ultraleight"
 * and integers. Case variations are ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
pango_parse_weight (const char  *str,
		    PangoWeight *weight,
		    gboolean     warn)
{
  if (*str == '\0')
    return FALSE;

  switch (str[0])
    {
    case 'b':
    case 'B':
      if (g_ascii_strcasecmp (str, "bold") == 0)
	{
	  *weight = PANGO_WEIGHT_BOLD;
	  return TRUE;
	}
      break;
    case 'h':
    case 'H':
      if (g_ascii_strcasecmp (str, "heavy") == 0)
	{
	  *weight = PANGO_WEIGHT_HEAVY;
	  return TRUE;
	}
      break;
    case 'l':
    case 'L':
      if (g_ascii_strcasecmp (str, "light") == 0)
	{
	  *weight = PANGO_WEIGHT_LIGHT;
	  return TRUE;
	}
      break;
    case 'n':
    case 'N':
      if (g_ascii_strcasecmp (str, "normal") == 0)
	{
	  *weight = PANGO_WEIGHT_NORMAL;
	  return TRUE;
	}
      break;
    case 'u':
    case 'U':
      if (g_ascii_strcasecmp (str, "ultralight") == 0)
	{
	  *weight = PANGO_WEIGHT_ULTRALIGHT;
	  return TRUE;
	}
      else if (g_ascii_strcasecmp (str, "ultrabold") == 0)
	{
	  *weight = PANGO_WEIGHT_ULTRABOLD;
	  return TRUE;
	}
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	char *end;

	*weight = strtol (str, &end, 10);
	if (*end != '\0')
	  {
	    if (warn)
	      g_warning ("failed parsing numerical weight '%s'", str);
	    return FALSE;
	  }
	return TRUE;
      }
    }

  if (warn)
    g_warning ("weight must be ultralight, light, normal, bold, ultrabold, heavy, or an integer");
  return FALSE;
}

/**
 * pango_parse_stretch:
 * @str: a string to parse.
 * @stretch: a #PangoStretch to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font stretch. The allowed values are
 * "ultra_condensed", "extra_condensed", "condensed",
 * "semi_condensed", "normal", "semi_expanded", "expanded",
 * "extra_expanded" and "ultra_expanded". Case variations are
 * ignored and the '_' characters may be omitted.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
pango_parse_stretch (const char   *str,
		     PangoStretch *stretch,
		     gboolean	   warn)
{
  if (*str == '\0')
    return FALSE;

  switch (str[0])
    {
    case 'c':
    case 'C':
      if (g_ascii_strcasecmp (str, "condensed") == 0)
	{
	  *stretch = PANGO_STRETCH_CONDENSED;
	  return TRUE;
	}
      break;
    case 'e':
    case 'E':
      if (g_ascii_strcasecmp (str, "extra_condensed") == 0 ||
	  g_ascii_strcasecmp (str, "extracondensed") == 0)
	{
	  *stretch = PANGO_STRETCH_EXTRA_CONDENSED;
	  return TRUE;
	}
     if (g_ascii_strcasecmp (str, "extra_expanded") == 0 ||
	 g_ascii_strcasecmp (str, "extraexpanded") == 0)
	{
	  *stretch = PANGO_STRETCH_EXTRA_EXPANDED;
	  return TRUE;
	}
      if (g_ascii_strcasecmp (str, "expanded") == 0)
	{
	  *stretch = PANGO_STRETCH_EXPANDED;
	  return TRUE;
	}
      break;
    case 'n':
    case 'N':
      if (g_ascii_strcasecmp (str, "normal") == 0)
	{
	  *stretch = PANGO_STRETCH_NORMAL;
	  return TRUE;
	}
      break;
    case 's':
    case 'S':
      if (g_ascii_strcasecmp (str, "semi_condensed") == 0 ||
	  g_ascii_strcasecmp (str, "semicondensed") == 0)
	{
	  *stretch = PANGO_STRETCH_SEMI_CONDENSED;
	  return TRUE;
	}
      if (g_ascii_strcasecmp (str, "semi_expanded") == 0 ||
	  g_ascii_strcasecmp (str, "semiexpanded") == 0)
	{
	  *stretch = PANGO_STRETCH_SEMI_EXPANDED;
	  return TRUE;
	}
      break;
    case 'u':
    case 'U':
      if (g_ascii_strcasecmp (str, "ultra_condensed") == 0 ||
	  g_ascii_strcasecmp (str, "ultracondensed") == 0)
	{
	  *stretch = PANGO_STRETCH_ULTRA_CONDENSED;
	  return TRUE;
	}
      if (g_ascii_strcasecmp (str, "ultra_expanded") == 0 ||
	  g_ascii_strcasecmp (str, "ultraexpanded") == 0)
	{
	  *stretch = PANGO_STRETCH_ULTRA_EXPANDED;
	  return TRUE;
	}
      break;
    }

  if (warn)
    g_warning ("stretch must be ultra_condensed, extra_condensed, condensed, semi_condensed, normal, semi_expanded, expanded, extra_expanded, or ultra_expanded");
  return FALSE;
}


static guint
alias_hash (struct PangoAlias *alias)
{
  return g_str_hash (alias->alias);
}

static gboolean
alias_equal (struct PangoAlias *alias1,
	     struct PangoAlias *alias2)
{
  return g_str_equal (alias1->alias,
		      alias2->alias);
}


static void
alias_free (struct PangoAlias *alias)
{
  int i;
  g_free (alias->alias);

  for (i = 0; i < alias->n_families; i++)
    g_free (alias->families[i]);

  g_free (alias->families);

  g_slice_free (struct PangoAlias, alias);
}

static void
handle_alias_line (GString  *line_buffer,
		   char    **errstring)
{
  GString *tmp_buffer1;
  GString *tmp_buffer2;
  const char *pos;
  struct PangoAlias alias_key;
  struct PangoAlias *alias;
  gboolean append = FALSE;
  char **new_families;
  int n_new;
  int i;

  tmp_buffer1 = g_string_new (NULL);
  tmp_buffer2 = g_string_new (NULL);


  pos = line_buffer->str;
  if (!pango_skip_space (&pos))
    return;

  if (!pango_scan_string (&pos, tmp_buffer1) ||
      !pango_skip_space (&pos))
    {
      *errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
      goto error;
    }

  if (*pos == '+')
    {
      append = TRUE;
      pos++;
    }

  if (*(pos++) != '=')
    {
      *errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
      goto error;
    }

  if (!pango_scan_string (&pos, tmp_buffer2))
    {
      *errstring = g_strdup ("Error parsing value string");
      goto error;
    }
  if (pango_skip_space (&pos))
    {
      *errstring = g_strdup ("Junk after value string");
      goto error;
    }

  alias_key.alias = g_ascii_strdown (tmp_buffer1->str, -1);

  /* Remove any existing values */
  alias = g_hash_table_lookup (pango_aliases_ht, &alias_key);

  if (!alias)
    {
      alias = g_slice_new0 (struct PangoAlias);
      alias->alias = alias_key.alias;
      
      g_hash_table_insert (pango_aliases_ht,
			   alias, alias);
    }
  else
    g_free (alias_key.alias);

  new_families = g_strsplit (tmp_buffer2->str, ",", -1);

  n_new = 0;
  while (new_families[n_new])
    n_new++;

  if (alias->families && append)
    {
      alias->families = g_realloc (alias->families,
				   sizeof (char *) *(n_new + alias->n_families));
      for (i = 0; i < n_new; i++)
	alias->families[alias->n_families + i] = new_families[i];
      g_free (new_families);
      alias->n_families += n_new;
    }
  else
    {
      for (i = 0; i < alias->n_families; i++)
	g_free (alias->families[i]);
      g_free (alias->families);
      
      alias->families = new_families;
      alias->n_families = n_new;
    }

 error:
  
  g_string_free (tmp_buffer1, TRUE);
  g_string_free (tmp_buffer2, TRUE);
}

#ifdef G_OS_WIN32

static const char *builtin_aliases[] = {
  "courier = \"courier new\"",
  "\"segoe ui\" = \"segoe ui,meiryo,malgun gothic,microsoft jhenghei,microsoft yahei,gisha,leelawadee,arial unicode ms,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "tahoma = \"tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  /* It sucks to use the same GulimChe, MS Gothic, Sylfaen, Kartika,
   * Latha, Mangal and Raavi fonts for all three of sans, serif and
   * mono, but it isn't like there would be much choice. For most
   * non-Latin scripts that Windows includes any font at all for, it
   * has ony one. One solution is to install the free DejaVu fonts
   * that are popular on Linux. They are listed here first.
   */
  "sans = \"dejavu sans,tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "sans-serif = \"dejavu sans,tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "serif = \"dejavu serif,georgia,angsana new,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "mono = \"dejavu sans mono,courier new,lucida console,courier monothai,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "monospace = \"dejavu sans mono,courier new,lucida console,courier monothai,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\""
};

static void
read_builtin_aliases (void)
{

  GString *line_buffer;
  char *errstring = NULL;
  int line;

  line_buffer = g_string_new (NULL);

  for (line = 0; line < G_N_ELEMENTS (builtin_aliases) && errstring == NULL; line++)
    {
      g_string_assign (line_buffer, builtin_aliases[line]);
      handle_alias_line (line_buffer, &errstring);
    }

  if (errstring)
    {
      g_error ("error in built-in aliases:%d: %s\n", line, errstring);
      g_free (errstring);
    }

  g_string_free (line_buffer, TRUE);
}

#endif

static void
read_alias_file (const char *filename)
{
  FILE *file;

  GString *line_buffer;
  char *errstring = NULL;
  int line = 0;

  file = g_fopen (filename, "r");
  if (!file)
    return;

  line_buffer = g_string_new (NULL);

  while (pango_read_line (file, line_buffer) &&
	 errstring == NULL)
    {
      line++;
      handle_alias_line (line_buffer, &errstring);
    }

  if (errstring == NULL && ferror (file))
    errstring = g_strdup (g_strerror(errno));

  if (errstring)
    {
      g_warning ("error reading alias file: %s:%d: %s\n", filename, line, errstring);
      g_free (errstring);
    }

  g_string_free (line_buffer, TRUE);

  fclose (file);
}

static void
pango_load_aliases (void)
{
  char *filename;
  const char *home;

  pango_aliases_ht = g_hash_table_new_full ((GHashFunc)alias_hash,
					    (GEqualFunc)alias_equal,
					    (GDestroyNotify)alias_free,
					    NULL);

#ifdef G_OS_WIN32
  read_builtin_aliases ();
#endif

  filename = g_strconcat (pango_get_sysconf_subdirectory (),
			  G_DIR_SEPARATOR_S "pango.aliases",
			  NULL);
  read_alias_file (filename);
  g_free (filename);

  home = g_get_home_dir ();
  if (home && *home)
    {
      filename = g_strconcat (home,
			      G_DIR_SEPARATOR_S ".pango.aliases",
			      NULL);
      read_alias_file (filename);
      g_free (filename);
    }
}


/**
 * pango_lookup_aliases:
 * @fontname: an ascii string
 * @families: will be set to an array of font family names.
 *    this array is owned by pango and should not be freed.
 * @n_families: will be set to the length of the @families array.
 *
 * Look up all user defined aliases for the alias @fontname.
 * The resulting font family names will be stored in @families,
 * and the number of families in @n_families.
 **/
void
pango_lookup_aliases (const char   *fontname,
		      char       ***families,
		      int          *n_families)
{
  struct PangoAlias alias_key;
  struct PangoAlias *alias;

  if (pango_aliases_ht == NULL)
    pango_load_aliases ();

  alias_key.alias = g_ascii_strdown (fontname, -1);
  alias = g_hash_table_lookup (pango_aliases_ht, &alias_key);
  g_free (alias_key.alias);

  if (alias)
    {
      *families = alias->families;
      *n_families = alias->n_families;
    }
  else
    {
      *families = NULL;
      *n_families = 0;
    }
}

/**
 * pango_find_base_dir:
 * @text:   the text to process
 * @length: length of @text in bytes (may be -1 if @text is nul-terminated)
 *
 * Searches a string the first character that has a strong
 * direction, according to the Unicode bidirectional algorithm.
 *
 * Return value: The direction corresponding to the first strong character.
 * If no such character is found, then %PANGO_DIRECTION_NEUTRAL is returned.
 *
 * Since: 1.4
 */
PangoDirection
pango_find_base_dir (const gchar *text,
		     gint         length)
{
  PangoDirection dir = PANGO_DIRECTION_NEUTRAL;
  const gchar *p;

  g_return_val_if_fail (text != NULL || length == 0, PANGO_DIRECTION_NEUTRAL);

  p = text;
  while ((length < 0 || p < text + length) && *p)
    {
      gunichar wc = g_utf8_get_char (p);

      dir = pango_unichar_direction (wc);

      if (dir != PANGO_DIRECTION_NEUTRAL)
	break;

      p = g_utf8_next_char (p);
    }

  return dir;
}

/**
 * pango_is_zero_width:
 * @ch: a Unicode character
 *
 * Checks @ch to see if it is a character that should not be
 * normally rendered on the screen.  This includes all Unicode characters
 * with "ZERO WIDTH" in their name, as well as <firstterm>bidi</firstterm> formatting characters, and
 * a few other ones.  This is totally different from g_unichar_iszerowidth()
 * and is at best misnamed.
 *
 * Return value: %TRUE if @ch is a zero-width character, %FALSE otherwise
 *
 * Since: 1.10
 */
gboolean
pango_is_zero_width (gunichar ch)
{
/* Zero Width characters:
 *
 *  00AD  SOFT HYPHEN
 *  034F  COMBINING GRAPHEME JOINER
 *
 *  200B  ZERO WIDTH SPACE
 *  200C  ZERO WIDTH NON-JOINER
 *  200D  ZERO WIDTH JOINER
 *  200E  LEFT-TO-RIGHT MARK
 *  200F  RIGHT-TO-LEFT MARK
 *
 *  2028  LINE SEPARATOR
 *
 *  202A  LEFT-TO-RIGHT EMBEDDING
 *  202B  RIGHT-TO-LEFT EMBEDDING
 *  202C  POP DIRECTIONAL FORMATTING
 *  202D  LEFT-TO-RIGHT OVERRIDE
 *  202E  RIGHT-TO-LEFT OVERRIDE
 *
 *  2060  WORD JOINER
 *  2061  FUNCTION APPLICATION
 *  2062  INVISIBLE TIMES
 *  2063  INVISIBLE SEPARATOR
 *
 *  FEFF  ZERO WIDTH NO-BREAK SPACE
 */
  return ((ch & ~(gunichar)0x007F) == 0x2000 && (
		(ch >= 0x200B && ch <= 0x200F) ||
		(ch >= 0x202A && ch <= 0x202E) ||
		(ch >= 0x2060 && ch <= 0x2063) ||
		(ch == 0x2028)
	 )) || G_UNLIKELY (ch == 0x00AD
			|| ch == 0x034F
			|| ch == 0xFEFF);
}

/**
 * pango_quantize_line_geometry:
 * @thickness: pointer to the thickness of a line, in Pango units
 * @position: corresponding position
 *
 * Quantizes the thickness and position of a line, typically an
 * underline or strikethrough, to whole device pixels, that is integer
 * multiples of %PANGO_SCALE. The purpose of this function is to avoid
 * such lines looking blurry.
 *
 * Care is taken to make sure @thickness is at least one pixel when this
 * function returns, but returned @position may become zero as a result
 * of rounding.
 *
 * Since: 1.12
 */
void
pango_quantize_line_geometry (int *thickness,
			      int *position)
{
  int thickness_pixels = (*thickness + PANGO_SCALE / 2) / PANGO_SCALE;
  if (thickness_pixels == 0)
    thickness_pixels = 1;

  if (thickness_pixels & 1)
    {
      int new_center = ((*position - *thickness / 2) & ~(PANGO_SCALE - 1)) + PANGO_SCALE / 2;
      *position = new_center + (PANGO_SCALE * thickness_pixels) / 2;
    }
  else
    {
      int new_center = ((*position - *thickness / 2 + PANGO_SCALE / 2) & ~(PANGO_SCALE - 1));
      *position = new_center + (PANGO_SCALE * thickness_pixels) / 2;
    }

  *thickness = thickness_pixels * PANGO_SCALE;
}

/**
 * pango_units_from_double:
 * @d: double floating-point value
 *
 * Converts a floating-point number to Pango units: multiplies
 * it by %PANGO_SCALE and rounds to nearest integer.
 *
 * Return value: the value in Pango units.
 *
 * Since: 1.16
 */
int
pango_units_from_double (double d)
{
  return (int)floor (d * PANGO_SCALE + 0.5);
}

/**
 * pango_units_to_double:
 * @i: value in Pango units
 *
 * Converts a number in Pango units to floating-point: divides
 * it by %PANGO_SCALE.
 *
 * Return value: the double value.
 *
 * Since: 1.16
 */
double
pango_units_to_double (int i)
{
  return (double)i / PANGO_SCALE;
}

/**
 * pango_extents_to_pixels:
 * @inclusive: rectangle to round to pixels inclusively, or %NULL.
 * @nearest: rectangle to round to nearest pixels, or %NULL.
 *
 * Converts extents from Pango units to device units, dividing by the
 * %PANGO_SCALE factor and performing rounding.
 *
 * The @inclusive rectangle is converted by flooring the x/y coordinates and extending
 * width/height, such that the final rectangle completely includes the original
 * rectangle.
 *
 * The @nearest rectangle is converted by rounding the coordinates
 * of the rectangle to the nearest device unit (pixel).
 *
 * The rule to which argument to use is: if you want the resulting device-space
 * rectangle to completely contain the original rectangle, pass it in as @inclusive.
 * If you want two touching-but-not-overlapping rectangles stay
 * touching-but-not-overlapping after rounding to device units, pass them in
 * as @nearest.
 *
 * Since: 1.16
 **/
void
pango_extents_to_pixels (PangoRectangle *inclusive,
			 PangoRectangle *nearest)
{
  if (inclusive)
    {
      int orig_x = inclusive->x;
      int orig_y = inclusive->y;

      inclusive->x = PANGO_PIXELS_FLOOR (inclusive->x);
      inclusive->y = PANGO_PIXELS_FLOOR (inclusive->y);

      inclusive->width  = PANGO_PIXELS_CEIL (orig_x + inclusive->width ) - inclusive->x;
      inclusive->height = PANGO_PIXELS_CEIL (orig_y + inclusive->height) - inclusive->y;
    }

  if (nearest)
    {
      int orig_x = nearest->x;
      int orig_y = nearest->y;

      nearest->x = PANGO_PIXELS (nearest->x);
      nearest->y = PANGO_PIXELS (nearest->y);

      nearest->width  = PANGO_PIXELS (orig_x + nearest->width ) - nearest->x;
      nearest->height = PANGO_PIXELS (orig_y + nearest->height) - nearest->y;
    }
}





/*********************************************************
 * Some internal functions for handling PANGO_ATTR_SHAPE *
 ********************************************************/

void
_pango_shape_shape (const char       *text,
		    gint              n_chars,
		    PangoRectangle   *shape_ink,
		    PangoRectangle   *shape_logical,
		    PangoGlyphString *glyphs)
{
  int i;
  const char *p;

  pango_glyph_string_set_size (glyphs, n_chars);

  for (i=0, p = text; i < n_chars; i++, p = g_utf8_next_char (p))
    {
      glyphs->glyphs[i].glyph = PANGO_GLYPH_EMPTY;
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = shape_logical->width;
      glyphs->glyphs[i].attr.is_cluster_start = 1;

      glyphs->log_clusters[i] = p - text;
    }
}

void
_pango_shape_get_extents (gint              n_chars,
			  PangoRectangle   *shape_ink,
			  PangoRectangle   *shape_logical,
			  PangoRectangle   *ink_rect,
			  PangoRectangle   *logical_rect)
{
  if (n_chars > 0)
    {
      if (ink_rect)
	{
	  ink_rect->x = MIN (shape_ink->x, shape_ink->x + shape_logical->width * (n_chars - 1));
	  ink_rect->width = MAX (shape_ink->width, shape_ink->width + shape_logical->width * (n_chars - 1));
	  ink_rect->y = shape_ink->y;
	  ink_rect->height = shape_ink->height;
	}
      if (logical_rect)
	{
	  logical_rect->x = MIN (shape_logical->x, shape_logical->x + shape_logical->width * (n_chars - 1));
	  logical_rect->width = MAX (shape_logical->width, shape_logical->width + shape_logical->width * (n_chars - 1));
	  logical_rect->y = shape_logical->y;
	  logical_rect->height = shape_logical->height;
	}
    }
  else
    {
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->y = 0;
	  ink_rect->width = 0;
	  ink_rect->height = 0;
	}

      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = 0;
	  logical_rect->width = 0;
	  logical_rect->height = 0;
	}
    }
}

