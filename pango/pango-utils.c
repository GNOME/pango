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

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <config.h>

#include "pango-font.h"
#include "pango-utils.h"

#ifndef HAVE_FLOCKFILE
#  define flockfile(f) (void)1
#  define funlockfile(f) (void)1
#  define getc_unlocked(f) getc(f)
#endif /* !HAVE_FLOCKFILE */

#ifdef G_OS_WIN32

#define STRICT
#include <windows.h>

#endif

/**
 * pango_trim_string:
 * @str: a string
 * 
 * Trim leading and trailing whitespace from a string.
 * 
 * Return value: A newly allocated string that must be freed with g_free()
 **/
char *
pango_trim_string (const char *str)
{
  int len;

  g_return_val_if_fail (str != NULL, NULL);
  
  while (*str && isspace (*str))
    str++;

  len = strlen (str);
  while (len > 0 && isspace (str[len-1]))
    len--;

  return g_strndup (str, len);
}

/**
 * pango_split_file_list:
 * @str: a comma separated list of filenames
 * 
 * Split a G_SEARCHPATH_SEPARATOR-separated list of files, stripping
 * white space and subsituting ~/ with $HOME/
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
       * still occur and then. Also, few Windows user are aware of the
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
 * Read an entire line from a file into a buffer. Lines may
 * be delimited with '\n', '\r', '\n\r', or '\r\n'. The delimiter
 * is not written into the buffer. Text after a '#' character is treated as
 * a comment and skipped. '\' can be used to escape a # character.
 * '\' proceding a line delimiter combines adjacent lines. A '\' proceding
 * any other character is ignored and written into the output buffer
 * unmodified.
 * 
 * Return value: %FALSE if the stream was already at an EOF character.
 **/
gboolean
pango_read_line (FILE *stream, GString *str)
{
  gboolean quoted = FALSE;
  gboolean comment = FALSE;
  int n_read = 0;
  
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

		if (!(c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);
		
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

  return n_read > 0;
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
  
  while (isspace (*p))
    p++;

  *pos = p;

  return !(*p == '\0');
}

/**
 * pango_scan_word:
 * @pos: in/out string position
 * @out: a #GString into which to write the result
 * 
 * Scan a word into a #GString buffer. A word consists
 * of [A-Za-z_] followed by zero or more [A-Za-z_0-9]
 * Leading white space is skipped.
 * 
 * Return value: %FALSE if a parse error occured. 
 **/
gboolean
pango_scan_word (const char **pos, GString *out)
{
  const char *p = *pos;

  while (isspace (*p))
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
 * Scan a string into a #GString buffer. The string may either
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
  
  while (isspace (*p))
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
    }
  else
    {
      g_string_truncate (out, 0);

      while (*p && !isspace (*p))
	{
	  g_string_append_c (out, *p);
	  p++;
	}
    }

  *pos = p;

  return TRUE;
}

gboolean
pango_scan_int (const char **pos, int *out)
{
  int i = 0;
  char buf[32];
  const char *p = *pos;

  while (isspace (*p))
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

  return TRUE;
}

static GHashTable *config_hash = NULL;

static void
read_config_file (const char *filename, gboolean enoent_error)
{
  FILE *file;
    
  GString *line_buffer = g_string_new (NULL);
  GString *tmp_buffer1 = g_string_new (NULL);
  GString *tmp_buffer2 = g_string_new (NULL);
  char *errstring = NULL;
  const char *pos;
  char *section = NULL;
  int line = 0;

  file = fopen (filename, "r");
  if (!file)
    {
      if (errno != ENOENT || enoent_error)
	fprintf (stderr, "Pango:%s: Error opening config file: %s\n",
		 filename, g_strerror (errno));
      return;
    }
  
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
    errstring = g_strdup ("g_strerror(errno)");
  
 error:

  if (errstring)
    {
      fprintf (stderr, "Pango:%s:%d: %s\n", filename, line, errstring);
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
      char *home;
      
      config_hash = g_hash_table_new (g_str_hash, g_str_equal);
      filename = g_strconcat (pango_get_sysconf_subdirectory (),
			      G_DIR_SEPARATOR_S "pangorc",
			      NULL);
      read_config_file (filename, FALSE);
      g_free (filename);

      home = g_get_home_dir ();
      if (home && *home)
	{
	  filename = g_strconcat (home,
				  G_DIR_SEPARATOR_S ".pangorc",
				  NULL);
	  read_config_file (filename, FALSE);
	  g_free (filename);
	}

      filename = g_getenv ("PANGO_RC_FILE");
      if (filename)
	read_config_file (filename, TRUE);
    }
}

/**
 * pango_config_key_get:
 * @key: Key to look up, in the form "SECTION/KEY".
 * 
 * Look up a key in the pango config database
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

char *
pango_get_sysconf_subdirectory (void)
{
#ifdef G_OS_WIN32

  /* On Windows we don't hardcode any paths (SYSCONFDIR) in the DLL,
   * but rely on an installation program to store the installation
   * directory in the registry. If no installation program has been
   * used, punt and assume the Pango directory is %WINDIR%\Pango.
   */

  static gboolean been_here = FALSE;
  static gchar pango_sysconf_dir[200];
  gchar win_dir[100];
  HKEY reg_key = NULL;
  DWORD type;
  DWORD nbytes = sizeof (pango_sysconf_dir);

  if (been_here)
    return pango_sysconf_dir;

  been_here = TRUE;

  if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Software\\GNU\\Pango", 0,
		    KEY_QUERY_VALUE, &reg_key) != ERROR_SUCCESS
      || RegQueryValueEx (reg_key, "InstallationDirectory", 0,
			  &type, pango_sysconf_dir, &nbytes) != ERROR_SUCCESS
      || type != REG_SZ)
    {
      /* Uh oh. Use %WinDir%\Pango */
      GetWindowsDirectory (win_dir, sizeof (win_dir));
      sprintf (pango_sysconf_dir, "%s\\pango", win_dir);
    }

  if (reg_key != NULL)
    RegCloseKey (reg_key);

  return pango_sysconf_dir;

#else
  return SYSCONFDIR "/pango";
#endif
}

char *
pango_get_lib_subdirectory (void)
{
#ifdef G_OS_WIN32
  return pango_get_sysconf_subdirectory ();
#else
  return LIBDIR "/pango";
#endif
}

gboolean
pango_parse_style (GString              *str,
		   PangoFontDescription *desc)
{
  if (str->len == 0)
    return FALSE;

  switch (str->str[0])
    {
    case 'n':
    case 'N':
      if (strncasecmp (str->str, "normal", str->len) == 0)
	{
	  desc->style = PANGO_STYLE_NORMAL;
	  return TRUE;
	}
      break;
    case 'i':
      if (strncasecmp (str->str, "italic", str->len) == 0)
	{
	  desc->style = PANGO_STYLE_ITALIC;
	  return TRUE;
	}
      break;
    case 'o':
      if (strncasecmp (str->str, "oblique", str->len) == 0)
	{
	  desc->style = PANGO_STYLE_OBLIQUE;
	  return TRUE;
	}
      break;
    }
  g_warning ("Style must be normal, italic, or oblique");
  
  return FALSE;
}

gboolean
pango_parse_variant (GString              *str,
		     PangoFontDescription *desc)
{
  if (str->len == 0)
    return FALSE;

  switch (str->str[0])
    {
    case 'n':
    case 'N':
      if (strncasecmp (str->str, "normal", str->len) == 0)
	{
	  desc->variant = PANGO_VARIANT_NORMAL;
	  return TRUE;
	}
      break;
    case 's':
    case 'S':
      if (strncasecmp (str->str, "small_caps", str->len) == 0)
	{
	  desc->variant = PANGO_VARIANT_SMALL_CAPS;
	  return TRUE;
	}
      break;
    }
  
  g_warning ("Variant must be normal, or small_caps");
  return FALSE;
}

gboolean
pango_parse_weight (GString              *str,
		    PangoFontDescription *desc)
{
  if (str->len == 0)
    return FALSE;

  switch (str->str[0])
    {
    case 'b':
    case 'B':
      if (strncasecmp (str->str, "bold", str->len) == 0)
	{
	  desc->weight = PANGO_WEIGHT_BOLD;
	  return TRUE;
	}
      break;
    case 'h':
    case 'H':
      if (strncasecmp (str->str, "heavy", str->len) == 0)
	{
	  desc->weight = PANGO_WEIGHT_HEAVY;
	  return TRUE;
	}
      break;
    case 'l':
    case 'L':
      if (strncasecmp (str->str, "light", str->len) == 0)
	{
	  desc->weight = PANGO_WEIGHT_LIGHT;
	  return TRUE;
	}
      break;
    case 'n':
    case 'N':
      if (strncasecmp (str->str, "normal", str->len) == 0)
	{
	  desc->weight = PANGO_WEIGHT_NORMAL;
	  return TRUE;
	}
      break;
    case 'u':
    case 'U':
      if (strncasecmp (str->str, "ultralight", str->len) == 0)
	{
	  desc->weight = PANGO_WEIGHT_ULTRALIGHT;
	  return TRUE;
	}
      else if (strncasecmp (str->str, "ultrabold", str->len) == 0)
	{
	  desc->weight = PANGO_WEIGHT_ULTRABOLD;
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
	char *numstr, *end;

	numstr = g_strndup (str->str, str->len);

	desc->weight = strtol (numstr, &end, 0);
	if (*end != '\0')
	  {
	    g_warning ("Cannot parse numerical weight '%s'", numstr);
	    g_free (numstr);
	    return FALSE;
	  }

	g_free (numstr);
	return TRUE;
      }
    }
  
  g_warning ("Weight must be ultralight, light, normal, bold, ultrabold, heavy, or an integer");
  return FALSE;
}

gboolean
pango_parse_stretch (GString               *str,
		     PangoFontDescription *desc)
{
  if (str->len == 0)
    return FALSE;

  switch (str->str[0])
    { 
    case 'c':
    case 'C':
      if (strncasecmp (str->str, "condensed", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_CONDENSED;
	  return TRUE;
	}
      break;
    case 'e':
    case 'E':
      if (strncasecmp (str->str, "extra_condensed", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_EXTRA_CONDENSED;
	  return TRUE;
	}
     if (strncasecmp (str->str, "extra_expanded", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_EXTRA_EXPANDED;
	  return TRUE;
	}
      if (strncasecmp (str->str, "expanded", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_EXPANDED;
	  return TRUE;
	}
      break;
    case 'n':
    case 'N':
      if (strncasecmp (str->str, "normal", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_NORMAL;
	  return TRUE;
	}
      break;
    case 's':
    case 'S':
      if (strncasecmp (str->str, "semi_condensed", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_SEMI_CONDENSED;
	  return TRUE;
	}
      if (strncasecmp (str->str, "semi_expanded", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_SEMI_EXPANDED;
	  return TRUE;
	}
      break;
    case 'u':
    case 'U':
      if (strncasecmp (str->str, "ultra_condensed", str->len) == 0)
	{
	  desc->stretch = PANGO_STRETCH_ULTRA_CONDENSED;
	  return TRUE;
	}
      if (strncasecmp (str->str, "ultra_expanded", str->len) == 0)
	{
	  desc->variant = PANGO_STRETCH_ULTRA_EXPANDED;
	  return TRUE;
	}
      break;
    }

  g_warning ("Stretch must be ultra_condensed, extra_condensed, condensed, semi_condensed, normal, semi_expanded, expanded, extra_expanded, or ultra_expanded");
  return FALSE;
}

