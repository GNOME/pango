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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

#include "pango-font.h"
#include "pango-utils.h"

#ifdef HAVE_FRIBIDI
#include <fribidi/fribidi.h>
#endif

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

GHashTable *pango_aliases_ht = NULL;

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
 * @str: a comma separated list of filenames
 * 
 * Splits a G_SEARCHPATH_SEPARATOR-separated list of files, stripping
 * white space and subsituting ~/ with $HOME/.
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
 * Return value: %FALSE if a parse error occured. 
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
 * Return value: %FALSE if a parse error occured.
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
 * Scans an integer. An integer consists
 * of up to 31 decimal digits. 
 * Leading white space is skipped.
 * 
 * Return value: %FALSE if a parse error occured.
 **/
gboolean
pango_scan_int (const char **pos, int *out)
{
  int i = 0;
  char buf[32];
  const char *p = *pos;

  while (g_ascii_isspace (*p))
    p++;
  
  if (*p < '0' || *p > '9')
    return FALSE;

  while ((*p >= '0') && (*p <= '9') && i < sizeof(buf))
    {
      buf[i] = *p;
      i++;
      p++;
    }

  if (i == sizeof(buf))
    return FALSE;
  else
    buf[i] = '\0';

  *out = atoi (buf);
  *pos = p;

  return TRUE;
}

static GHashTable *config_hash = NULL;

static void
read_config_file (const char *filename, gboolean enoent_error)
{
  FILE *file;
    
  GString *line_buffer;
  GString *tmp_buffer1;
  GString *tmp_buffer2;
  char *errstring = NULL;
  const char *pos;
  char *section = NULL;
  int line = 0;

  file = fopen (filename, "r");
  if (!file)
    {
      if (errno != ENOENT || enoent_error)
	g_printerr ("Pango:%s: Error opening config file: %s\n",
		    filename, g_strerror (errno));
      return;
    }
  
  line_buffer = g_string_new (NULL);
  tmp_buffer1 = g_string_new (NULL);
  tmp_buffer2 = g_string_new (NULL);

  while (pango_read_line (file, line_buffer))
    {
      line++;

      pos = line_buffer->str;
      if (!pango_skip_space (&pos))
	continue;

      if (*pos == '[')	/* Section */
	{
	  pos++;
	  if (!pango_skip_space (&pos) ||
	      !pango_scan_word (&pos, tmp_buffer1) ||
	      !pango_skip_space (&pos) ||
	      *(pos++) != ']' ||
	      pango_skip_space (&pos))
	    {
	      errstring = g_strdup ("Error parsing [SECTION] declaration");
	      goto error;
	    }

	  section = g_strdup (tmp_buffer1->str);
	}
      else			/* Key */
	{
	  gboolean empty = FALSE;
	  gboolean append = FALSE;
	  char *k, *v;

	  if (!section)
	    {
	      errstring = g_strdup ("A [SECTION] declaration must occur first");
	      goto error;
	    }

	  if (!pango_scan_word (&pos, tmp_buffer1) ||
	      !pango_skip_space (&pos))
	    {
	      errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
	      goto error;
	    }
	  if (*pos == '+')
	    {
	      append = TRUE;
	      pos++;
	    }

	  if (*(pos++) != '=')
	    {
	      errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
	      goto error;
	    }
	    
	  if (!pango_skip_space (&pos))
	    {
	      empty = TRUE;
	    }
	  else
	    {
	      if (!pango_scan_string (&pos, tmp_buffer2))
		{
		  errstring = g_strdup ("Error parsing value string");
		  goto error;
		}
	      if (pango_skip_space (&pos))
		{
		  errstring = g_strdup ("Junk after value string");
		  goto error;
		}
	    }

	  g_string_prepend_c (tmp_buffer1, '/');
	  g_string_prepend (tmp_buffer1, section);

	  /* Remove any existing values */
	  if (g_hash_table_lookup_extended (config_hash, tmp_buffer1->str,
					    (gpointer *)&k, (gpointer *)&v))
	    {
	      g_free (k);
	      if (append)
		{
		  g_string_prepend (tmp_buffer2, v);
		  g_free (v);
		}
	    }
	      
	  if (!empty)
	    {
	      g_hash_table_insert (config_hash,
				   g_strdup (tmp_buffer1->str),
				   g_strdup (tmp_buffer2->str));
	    }
	}
    }
      
  if (ferror (file))
    errstring = g_strdup (g_strerror(errno));
  
 error:

  if (errstring)
    {
      g_printerr ("Pango:%s:%d: %s\n", filename, line, errstring);
      g_free (errstring);
    }
      
  g_free (section);
  g_string_free (line_buffer, TRUE);
  g_string_free (tmp_buffer1, TRUE);
  g_string_free (tmp_buffer2, TRUE);

  fclose (file);
}

static void
read_config ()
{
  if (!config_hash)
    {
      char *filename;
      const char *home;
      const char *envvar;
      
      config_hash = g_hash_table_new (g_str_hash, g_str_equal);
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
      if (g_ascii_strcasecmp (str, "italic") == 0)
	{
	  *style = PANGO_STYLE_ITALIC;
	  return TRUE;
	}
      break;
    case 'o':
      if (g_ascii_strcasecmp (str, "oblique") == 0)
	{
	  *style = PANGO_STYLE_OBLIQUE;
	  return TRUE;
	}
      break;
    }
  if (warn)
    g_warning ("Style must be normal, italic, or oblique");
  
  return FALSE;
}

/**
 * pango_parse_variant:
 * @str: a string to parse.
 * @style: a #PangoVariant to store the result in.
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
    g_warning ("Variant must be normal or small_caps");
  return FALSE;
}

/**
 * pango_parse_weight:
 * @str: a string to parse.
 * @style: a #PangoWeight to store the result in.
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

	*weight = strtol (str, &end, 0);
	if (*end != '\0')
	  {
	    if (warn)
	      g_warning ("Cannot parse numerical weight '%s'", str);
	    return FALSE;
	  }
	return TRUE;
      }
    }
  
  if (warn)
    g_warning ("Weight must be ultralight, light, normal, bold, ultrabold, heavy, or an integer");
  return FALSE;
}

/**
 * pango_parse_stretch:
 * @str: a string to parse.
 * @style: a #PangoStretch to store the result in.
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
    g_warning ("Stretch must be ultra_condensed, extra_condensed, condensed, semi_condensed, normal, semi_expanded, expanded, extra_expanded, or ultra_expanded");
  return FALSE;
}

static const char canon_map[256] = {
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,  '-',  0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0, 
   0,  'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z',  0,   0,   0,   0,  '-',
   0,  'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',  'x', 'y', 'z',  0,   0,   0,   0,   0
};
   
static gboolean
lang_equal (gconstpointer v1,
	    gconstpointer v2)
{
  const guchar *p1 = v1;
  const guchar *p2 = v2;

  while (*p2)
    {
      guchar value = canon_map[*p2];
      if (value && value != *p1++)
	return FALSE;
      p2++;
    }

  return (*p1 == '\0');
}

static guint
lang_hash (gconstpointer key)
{
  const guchar *p = key;
  guint h = 0;
  while (*p)
    {
      guchar value = canon_map[*p];
      if (value)
	h = (h << 5) - h + value;
      p++;
    }

  return h;
}

static PangoLanguage *
pango_language_copy (PangoLanguage *language)
{
  return language; /* language tags are const */
}
static void
pango_language_free (PangoLanguage *language)
{
  return; /* nothing */
}

GType
pango_language_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static ("PangoLanguage",
                                             (GBoxedCopyFunc)pango_language_copy,
                                             (GBoxedFreeFunc)pango_language_free);
  return our_type;
}

/**
 * pango_language_from_string:
 * @language: a string representing a language tag
 * 
 * Take a RFC-3066 format language tag as a string and convert it to a
 * #PangoLanguage pointer that can be efficiently copied (copy the
 * pointer) and compared with other language tags (compare the
 * pointer.)
 *
 * This function first canonicalizes the string by converting it to
 * lowercase, mapping '_' to '-', and stripping all characters other
 * than letters and '-'.
 * 
 * Return value: an opaque pointer to a #PangoLanguage structure.
 *               this will be valid forever after.
 **/
PangoLanguage *
pango_language_from_string (const char *language)
{
  static GHashTable *hash = NULL;
  char *result;
  int len;
  char *p;

  if (!hash)
    hash = g_hash_table_new (lang_hash, lang_equal);

  result = g_hash_table_lookup (hash, language);
  if (result)
    return (PangoLanguage *)result;

  len = strlen (language);
  result = g_malloc (len + 1);

  p = result;
  while (*language)
    {
      char value = canon_map[*(guchar *)language++];
      if (value)
	*(p++) = value;
    }
  *p++ = '\0';

  g_hash_table_insert (hash, result, result);

  return (PangoLanguage *)result;
}

/**
 * pango_language_matches:
 * @language: a language tag (see pango_language_from_string()),
 *            %NULL is allowed and matches nothing but '*'
 * @range_list: a list of language ranges, separated by ';' characters.
 *   each element must either be '*', or a RFC 3066 language range
 *   canonicalized as by pango_language_from_string().
 * 
 * Checks if a language tag matches one of the elements in a list of
 * language ranges. A language tag is considered to match a range
 * in the list if the range is '*', the range is exactly the tag,
 * or the range is a prefix of the tag, and the character after the
 * tag is '-'.
 *
 * Returns: %TRUE if a match was found.
 **/
gboolean
pango_language_matches (PangoLanguage *language,
			const char    *range_list)
{
  const char *lang_str = pango_language_to_string (language);
  const char *p = range_list;
  gboolean done = FALSE;

  while (!done)
    {
      const char *end = strchr (p, ';');
      if (!end)
	{
	  end = p + strlen (p);
	  done = TRUE;
	}

      if (strncmp (p, "*", 1) == 0 ||
	  (lang_str && strncmp (lang_str, p, end - p) == 0 &&
	   (lang_str[end - p] == '\0' || lang_str[end - p] == '-')))
	return TRUE;

      p = end;
    }

  return FALSE;
}

typedef struct {
  const char *lang;
  const char *str;
} LangInfo;

static int
lang_info_compare (const void *key, const void *val)
{
  const LangInfo *lang_info = val;

  return strncmp (key, lang_info->lang, 2);
}

/* The following array is supposed to contain enough text to tickle all necessary fonts for each
 * of the languages in the following. Yes, it's pretty lame. Not all of the languages
 * in the following have sufficient text to excercise all the accents for the language, and
 * there are obviously many more languages to include as well.
 */
static LangInfo lang_texts[] = {
  { "ar", "Arabic  السلام عليكم" },
  { "cs", "Czech (česky)  Dobrý den" },
  { "da", "Danish (Dansk)  Hej, Goddag" },
  { "el", "Greek (Ελληνικά) Γειά σας" },
  { "en", "English Hello" },
  { "eo", "Esperanto Saluton" },
  { "es", "Spanish (Español) ¡Hola!" },
  { "et", "Estonian  Tere, Tervist" },
  { "fi", "Finnish (Suomi)  Hei, Hyvää päivää" },
  { "fr", "French (Français)" },
  { "de", "German Grüß Gott" },
  { "iw", "Hebrew   שלום" },
  { "il", "Italiano  Ciao, Buon giorno" },
  { "ja", "Japanese (日本語) こんにちは, ｺﾝﾆﾁﾊ" },
  { "ko", "Korean (한글)   안녕하세요, 안녕하십니까" },
  { "mt", "Maltese   Ċaw, Saħħa" },
  { "nl", "Nederlands, Vlaams Hallo, Dag" },
  { "no", "Norwegian (Norsk) Hei, God dag" },
  { "pl", "Polish   Dzień dobry, Hej" },
  { "ru", "Russian (Русский)" },
  { "sk", "Slovak   Dobrý deň" },
  { "sv", "Swedish (Svenska) Hej på dej, Goddag" },
  { "tr", "Turkish (Türkçe) Merhaba" },
  { "zh", "Chinese (中文,普通话,汉语)" }
};

/**
 * pango_language_get_sample_string:
 * @language: a #PangoLanguage
 * 
 * Get a string that is representative of the characters needed to
 * render a particular language. This function is a bad hack for
 * internal use by renderers and Pango.
 * 
 * Return value: the sample string. This value is owned by Pango
 *   and must not be freed.
 **/
G_CONST_RETURN char *
pango_language_get_sample_string (PangoLanguage *language)
{
  const char *result;
  
  if (language)
    {
      const char *lang_str = pango_language_to_string (language);
      
      LangInfo *lang_info = bsearch (lang_str, lang_texts,
				     G_N_ELEMENTS (lang_texts), sizeof (LangInfo),
				     lang_info_compare);

      if (lang_info)
	result = lang_info->str;
      else
	result = "French (Français)";     /* Assume iso-8859-1 */
    }
  else
    {
      /* Complete junk
       */

      result = "السلام عليكم česky Ελληνικά Français 日本語 한글 Русский 中文,普通话,汉语 Türkçe";
    }

  return result;
}

#ifdef HAVE_FRIBIDI

gboolean
pango_log2vis_get_embedding_levels (gunichar       *str,
				    int             len,
				    PangoDirection *pbase_dir,
				    guint8         *embedding_level_list)
{
  FriBidiCharType fribidi_base_dir;
  gboolean result;

  fribidi_base_dir = (*pbase_dir == PANGO_DIRECTION_LTR) ? FRIBIDI_TYPE_L : FRIBIDI_TYPE_R;

  result = fribidi_log2vis_get_embedding_levels (str, len, &fribidi_base_dir,
						 embedding_level_list);
  
  *pbase_dir = (fribidi_base_dir == FRIBIDI_TYPE_L) ?  PANGO_DIRECTION_LTR : PANGO_DIRECTION_RTL;

  return result;
}


gboolean 
pango_get_mirror_char (gunichar        ch,
		       gunichar       *mirrored_ch)
{
  return fribidi_get_mirror_char (ch, mirrored_ch); 
}

#endif /* HAVE_FRIBIDI */


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
  
  g_free (alias);
}

static void
read_alias_file (const char *filename)
{
  FILE *file;
    
  GString *line_buffer;
  GString *tmp_buffer1;
  GString *tmp_buffer2;
  char *errstring = NULL;
  const char *pos;
  int line = 0;
  struct PangoAlias alias_key;
  struct PangoAlias *alias;
  char **new_families;
  int n_new;
  int i;

  file = fopen (filename, "r");
  if (!file)
    return;

  line_buffer = g_string_new (NULL);
  tmp_buffer1 = g_string_new (NULL);
  tmp_buffer2 = g_string_new (NULL);

  while (pango_read_line (file, line_buffer))
    {
      gboolean empty = FALSE;
      gboolean append = FALSE;
      line++;

      pos = line_buffer->str;
      if (!pango_skip_space (&pos))
	continue;
      
      if (!pango_scan_word (&pos, tmp_buffer1) ||
	  !pango_skip_space (&pos))
	{
	  errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
	  goto error;
	}
      
      if (*pos == '+')
	{
	  append = TRUE;
	  pos++;
	}

      if (*(pos++) != '=')
	{
	  errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
	  goto error;
	}
      
      if (!pango_skip_space (&pos))
	{
	  empty = TRUE;
	}
      else
	{
	  if (!pango_scan_string (&pos, tmp_buffer2))
	    {
	      errstring = g_strdup ("Error parsing value string");
	      goto error;
	    }
	  if (pango_skip_space (&pos))
	    {
	      errstring = g_strdup ("Junk after value string");
	      goto error;
	    }
	}

      alias_key.alias = g_ascii_strdown (tmp_buffer1->str, -1);
      
      /* Remove any existing values */
      alias = g_hash_table_lookup (pango_aliases_ht, &alias_key);
      
      if (!alias)
	{
	  alias = g_new0 (struct PangoAlias, 1);
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
    }

  if (ferror (file))
    errstring = g_strdup (g_strerror(errno));
  
 error:

  if (errstring)
    {
      g_printerr ("Pango:%s:%d: %s\n", filename, line, errstring);
      g_free (errstring);
    }
      
  g_string_free (line_buffer, TRUE);
  g_string_free (tmp_buffer1, TRUE);
  g_string_free (tmp_buffer2, TRUE);

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
 *
 * Look up all user defined aliases for the alias #fontname.
 * The resulting font family names will be stored in #families,
 * and the number of families will be returned.
 * 
 * Return value: the number of font famillies stored in the #families argument.
 *   This value is owned by Pango and must not be freed.
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
