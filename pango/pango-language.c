/* Pango
 * pango-language.c: Language handling routines
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

#include "pango-language.h"
#include "pango-impl-utils.h"

#ifdef HAVE_CORE_TEXT
#include <CoreFoundation/CoreFoundation.h>
#endif /* HAVE_CORE_TEXT */


/* We embed a private struct right *before* a where a PangoLanguage *
 * points to.
 */

typedef struct {
  gconstpointer lang_info;
  gconstpointer script_for_lang;

  int magic; /* Used for verification */
} PangoLanguagePrivate;

#define PANGO_LANGUAGE_PRIVATE_MAGIC 0x0BE4DAD0

static void
pango_language_private_init (PangoLanguagePrivate *priv)
{
  priv->magic = PANGO_LANGUAGE_PRIVATE_MAGIC;

  priv->lang_info = (gconstpointer) -1;
  priv->script_for_lang = (gconstpointer) -1;
}

static PangoLanguagePrivate * pango_language_get_private (PangoLanguage *language) G_GNUC_CONST;

static PangoLanguagePrivate *
pango_language_get_private (PangoLanguage *language)
{
  PangoLanguagePrivate *priv;

  if (!language)
    return NULL;

  priv = (PangoLanguagePrivate *)(void *)((char *)language - sizeof (PangoLanguagePrivate));

  if (G_UNLIKELY (priv->magic != PANGO_LANGUAGE_PRIVATE_MAGIC))
    {
      g_critical ("Invalid PangoLanguage.  Did you pass in a straight string instead of calling pango_language_from_string()?");
      return NULL;
    }

  return priv;
}



#define LANGUAGE_SEPARATORS ";:, \t"

static const char canon_map[256] = {
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,  '-',  0,   0,
  '0', '1', '2', '3', '4', '5', '6', '7',  '8', '9',  0,   0,   0,   0,   0,   0,
  '-', 'a', 'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
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

  while (canon_map[*p1] && canon_map[*p1] == canon_map[*p2])
    {
      p1++, p2++;
    }

  return (canon_map[*p1] == canon_map[*p2]);
}

static guint
lang_hash (gconstpointer key)
{
  const guchar *p = key;
  guint h = 0;
  while (canon_map[*p])
    {
      h = (h << 5) - h + canon_map[*p];
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
pango_language_free (PangoLanguage *language G_GNUC_UNUSED)
{
  return; /* nothing */
}

/**
 * PangoLanguage:
 *
 * The `PangoLanguage` structure is used to
 * represent a language.
 *
 * `PangoLanguage` pointers can be efficiently
 * copied and compared with each other.
 */
G_DEFINE_BOXED_TYPE (PangoLanguage, pango_language,
                     pango_language_copy,
                     pango_language_free);

/**
 * _pango_get_lc_ctype:
 *
 * Return the Unix-style locale string for the language currently in
 * effect. On Unix systems, this is the return value from
 * `setlocale (LC_CTYPE, NULL)`, and the user can affect this through
 * the environment variables LC_ALL, LC_CTYPE or LANG (checked
 * in that order). The locale strings typically is in the form lang_COUNTRY,
 * where lang is an ISO-639 language code, and COUNTRY is an ISO-3166 country
 * code. For instance, sv_FI for Swedish as written in Finland or pt_BR for
 * Portuguese as written in Brazil.
 *
 * On Windows, the C library doesn't use any such environment
 * variables, and setting them won't affect the behavior of functions
 * like ctime(). The user sets the locale through the Regional Options
 * in the Control Panel. The C library (in the setlocale() function)
 * does not use country and language codes, but country and language
 * names spelled out in English.
 * However, this function does check the above environment
 * variables, and does return a Unix-style locale string based on
 * either said environment variables or the thread's current locale.
 *
 * Return value: a dynamically allocated string, free with g_free().
 */
static gchar *
_pango_get_lc_ctype (void)
{
#ifdef G_OS_WIN32
  /* Somebody might try to set the locale for this process using the
   * LANG or LC_ environment variables. The Microsoft C library
   * doesn't know anything about them. You set the locale in the
   * Control Panel. Setting these env vars won't have any affect on
   * locale-dependent C library functions like ctime(). But just for
   * kicks, do obey LC_ALL, LC_CTYPE and LANG in Pango. (This also makes
   * it easier to test GTK and Pango in various default languages, you
   * don't have to clickety-click in the Control Panel, you can simply
   * start the program with LC_ALL=something on the command line.)
   */

  gchar *p;

  p = getenv ("LC_ALL");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LC_CTYPE");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LANG");
  if (p != NULL)
    return g_strdup (p);

  return g_win32_getlocale ();
#elif defined(HAVE_CORE_TEXT)
  CFArrayRef languages;
  CFStringRef language;
  gchar ret[16];
  gchar *p;

  /* Take the same approach as done for Windows above. First we check
   * if somebody tried to set the locale through environment variables.
   */
  p = getenv ("LC_ALL");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LC_CTYPE");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LANG");
  if (p != NULL)
    return g_strdup (p);

  /* If the environment variables are not set, determine the locale
   * through the platform-native API.
   */
  languages = CFLocaleCopyPreferredLanguages ();
  language = CFArrayGetValueAtIndex (languages, 0);

  if (!CFStringGetCString (language, ret, 16, kCFStringEncodingUTF8))
    {
      CFRelease (languages);
      return g_strdup (setlocale (LC_CTYPE, NULL));
    }

  CFRelease (languages);

  return g_strdup (ret);
#else
  {
    gchar *lc_ctype = setlocale (LC_CTYPE, NULL);

    if (lc_ctype)
      return g_strdup (lc_ctype);
    else
      return g_strdup ("C");
  }
#endif
}

/**
 * pango_language_get_default:
 *
 * Returns the `PangoLanguage` for the current locale of the process.
 *
 * On Unix systems, this is the return value is derived from
 * `setlocale (LC_CTYPE, NULL)`, and the user can
 * affect this through the environment variables LC_ALL, LC_CTYPE or
 * LANG (checked in that order). The locale string typically is in
 * the form lang_COUNTRY, where lang is an ISO-639 language code, and
 * COUNTRY is an ISO-3166 country code. For instance, sv_FI for
 * Swedish as written in Finland or pt_BR for Portuguese as written in
 * Brazil.
 *
 * On Windows, the C library does not use any such environment
 * variables, and setting them won't affect the behavior of functions
 * like ctime(). The user sets the locale through the Regional Options
 * in the Control Panel. The C library (in the setlocale() function)
 * does not use country and language codes, but country and language
 * names spelled out in English.
 * However, this function does check the above environment
 * variables, and does return a Unix-style locale string based on
 * either said environment variables or the thread's current locale.
 *
 * Your application should call `setlocale(LC_ALL, "")` for the user
 * settings to take effect. GTK does this in its initialization
 * functions automatically (by calling gtk_set_locale()).
 * See the setlocale() manpage for more details.
 *
 * Note that the default language can change over the life of an application.
 *
 * Also note that this function will not do the right thing if you
 * use per-thread locales with uselocale(). In that case, you should
 * just call pango_language_from_string() yourself.
 *
 * Return value: (transfer none): the default language as a `PangoLanguage`
 **/
PangoLanguage *
pango_language_get_default (void)
{
  static PangoLanguage *result = NULL; /* MT-safe */

  if (g_once_init_enter (&result))
    {
      gchar *lc_ctype;
      PangoLanguage *lang;

      lc_ctype = _pango_get_lc_ctype ();
      lang = pango_language_from_string (lc_ctype);
      g_free (lc_ctype);

      g_once_init_leave (&result, lang);
    }

  return result;
}

/**
 * pango_language_from_string:
 * @language: (nullable): a string representing a language tag
 *
 * Convert a language tag to a `PangoLanguage`.
 *
 * The language tag must be in a RFC-3066 format. `PangoLanguage` pointers
 * can be efficiently copied (copy the pointer) and compared with other
 * language tags (compare the pointer.)
 *
 * This function first canonicalizes the string by converting it to
 * lowercase, mapping '_' to '-', and stripping all characters other
 * than letters and '-'.
 *
 * Use [func@Pango.Language.get_default] if you want to get the
 * `PangoLanguage` for the current locale of the process.
 *
 * Return value: (transfer none) (nullable): a `PangoLanguage`
 */
PangoLanguage *
pango_language_from_string (const char *language)
{
  G_LOCK_DEFINE_STATIC (lang_from_string);
  static GHashTable *hash = NULL; /* MT-safe */
  PangoLanguagePrivate *priv;
  char *result;
  int len;
  char *p;

  if (language == NULL)
    return NULL;

  G_LOCK (lang_from_string);

  if (G_UNLIKELY (!hash))
    hash = g_hash_table_new (lang_hash, lang_equal);
  else
    {
      result = g_hash_table_lookup (hash, language);
      if (result)
        goto out;
    }

  len = strlen (language);
  priv = g_malloc0 (sizeof (PangoLanguagePrivate) + len + 1);
  g_assert (priv);

  result = (char *)priv;
  result += sizeof (PangoLanguagePrivate);

  pango_language_private_init (priv);

  p = result;
  while ((*(p++) = canon_map[*(guchar *)language++]))
    ;

  g_hash_table_insert (hash, result, result);

out:
  G_UNLOCK (lang_from_string);

  return (PangoLanguage *)result;
}

/**
 * pango_language_to_string:
 * @language: a language tag.
 *
 * Gets the RFC-3066 format string representing the given language tag.
 *
 * Returns (transfer none): a string representing the language tag
 */
const char *
(pango_language_to_string) (PangoLanguage *language)
{
  return pango_language_to_string (language);
}

/**
 * pango_language_matches:
 * @language: (nullable): a language tag (see [func@Pango.Language.from_string]),
 *   %NULL is allowed and matches nothing but '*'
 * @range_list: a list of language ranges, separated by ';', ':',
 *   ',', or space characters.
 *   Each element must either be '*', or a RFC 3066 language range
 *   canonicalized as by [func@Pango.Language.from_string]
 *
 * Checks if a language tag matches one of the elements in a list of
 * language ranges.
 *
 * A language tag is considered to match a range in the list if the
 * range is '*', the range is exactly the tag, or the range is a prefix
 * of the tag, and the character after it in the tag is '-'.
 *
 * Return value: %TRUE if a match was found
 */
gboolean
pango_language_matches (PangoLanguage *language,
			const char    *range_list)
{
  const char *lang_str = pango_language_to_string (language);
  const char *p = range_list;
  gboolean done = FALSE;

  while (!done)
    {
      const char *end = strpbrk (p, LANGUAGE_SEPARATORS);
      if (!end)
	{
	  end = p + strlen (p);
	  done = TRUE;
	}

      if (strncmp (p, "*", 1) == 0 ||
	  (lang_str && strncmp (lang_str, p, end - p) == 0 &&
	   (lang_str[end - p] == '\0' || lang_str[end - p] == '-')))
	return TRUE;

      if (!done)
	p = end + 1;
    }

  return FALSE;
}

static int
lang_compare_first_component (gconstpointer pa,
			      gconstpointer pb)
{
  const char *a = pa, *b = pb;
  unsigned int da, db;
  const char *p;

  p = strstr (a, "-");
  da = p ? (unsigned int) (p - a) : strlen (a);

  p = strstr (b, "-");
  db = p ? (unsigned int) (p - b) : strlen (b);
   
  return strncmp (a, b, MAX (da, db));
}

/* Finds the best record for @language in an array of records.
 * Each record should start with the string representation of the language
 * code for the record (embedded, not a pointer), and the records must be
 * sorted on language code.
 */
static gconstpointer
find_best_lang_match (PangoLanguage *language,
		      gconstpointer  records,
		      guint          num_records,
		      guint          record_size)
{
  const char *lang_str;
  const char *record, *start, *end;

  if (language == NULL)
    return NULL;

  lang_str = pango_language_to_string (language);

  record = bsearch (lang_str,
		    records, num_records, record_size,
		    lang_compare_first_component);
  if (!record)
    return NULL;

  start = (const char *) records;
  end   = start + num_records * record_size;

  /* find the best match among all those that have the same first-component */

  /* go to the final one matching in the first component */
  while (record < end - record_size &&
	 lang_compare_first_component (lang_str, record + record_size) == 0)
    record += record_size;

  /* go back, find which one matches completely */
  while (start <= record &&
	 lang_compare_first_component (lang_str, record) == 0)
    {
      if (pango_language_matches (language, record))
        return record;

      record -= record_size;
    }

  return NULL;
}

static gconstpointer
find_best_lang_match_cached (PangoLanguage *language,
			     gconstpointer *cache,
			     gconstpointer  records,
			     guint          num_records,
			     guint          record_size)
{
  gconstpointer result;

  if (G_LIKELY (cache && *cache != (gconstpointer) -1))
    return *cache;

  result = find_best_lang_match (language,
				 records,
				 num_records,
				 record_size);

  if (cache)
    *cache = result;

  return result;
}

#define FIND_BEST_LANG_MATCH_CACHED(language, cache_key, records) \
	find_best_lang_match_cached ((language), \
				     pango_language_get_private (language) ? \
				       &(pango_language_get_private (language)->cache_key) : NULL, \
				     records, \
				     G_N_ELEMENTS (records), \
				     sizeof (*records));

typedef struct {
  char lang[6];
  guint16 offset;
} LangInfo;

/* Pure black magic, based on appendix of dsohowto.pdf */
#define POOLSTRFIELD(line) POOLSTRFIELD1(line)
#define POOLSTRFIELD1(line) str##line
struct _LangPoolStruct {
  char str0[1];
#define LANGUAGE(id, source, sample) char POOLSTRFIELD(__LINE__)[sizeof(sample)];
#include "pango-language-sample-table.h"
#undef LANGUAGE
};

static const union _LangPool {
  struct _LangPoolStruct lang_pool_struct;
  const char str[1];
} lang_pool = { {
    "",
#define LANGUAGE(id, source, sample) sample,
#include "pango-language-sample-table.h"
#undef LANGUAGE
} };
static const LangInfo lang_texts[] = {
#define LANGUAGE(id, source, sample) {G_STRINGIFY(id),	G_STRUCT_OFFSET(struct _LangPoolStruct, POOLSTRFIELD(__LINE__))},
#include "pango-language-sample-table.h"
#undef LANGUAGE
  /* One extra entry with no final comma, to make it C89-happy */
 {"~~",	0}
};

/**
 * pango_language_get_sample_string:
 * @language: (nullable): a `PangoLanguage`
 *
 * Get a string that is representative of the characters needed to
 * render a particular language.
 *
 * The sample text may be a pangram, but is not necessarily. It is chosen
 * to be demonstrative of normal text in the language, as well as exposing
 * font feature requirements unique to the language. It is suitable for use
 * as sample text in a font selection dialog.
 *
 * If @language is %NULL, the default language as found by
 * [func@Pango.Language.get_default] is used.
 *
 * If Pango does not have a sample string for @language, the classic
 * "The quick brown fox..." is returned.  This can be detected by
 * comparing the returned pointer value to that returned for (non-existent)
 * language code "xx".  That is, compare to:
 *
 * ```
 * pango_language_get_sample_string (pango_language_from_string ("xx"))
 * ```
 *
 * Return value: (transfer none): the sample string
 */
const char *
pango_language_get_sample_string (PangoLanguage *language)
{
  const LangInfo *lang_info;

  if (!language)
    language = pango_language_get_default ();

  lang_info = FIND_BEST_LANG_MATCH_CACHED (language,
					   lang_info,
					   lang_texts);

  if (lang_info)
    return lang_pool.str + lang_info->offset;

  return "The quick brown fox jumps over the lazy dog.";
}




/*
 * From language to script
 */


#include "pango-script-lang-table.h"

/**
 * pango_language_get_scripts:
 * @language: (nullable): a `PangoLanguage`
 * @num_scripts: (out caller-allocates) (optional): location to
 *   return number of scripts
 *
 * Determines the scripts used to to write @language.
 *
 * If nothing is known about the language tag @language,
 * or if @language is %NULL, then %NULL is returned.
 * The list of scripts returned starts with the script that the
 * language uses most and continues to the one it uses least.
 *
 * The value @num_script points at will be set to the number
 * of scripts in the returned array (or zero if %NULL is returned).
 *
 * Most languages use only one script for writing, but there are
 * some that use two (Latin and Cyrillic for example), and a few
 * use three (Japanese for example). Applications should not make
 * any assumptions on the maximum number of scripts returned
 * though, except that it is positive if the return value is not
 * %NULL, and it is a small number.
 *
 * The [method@Pango.Language.includes_script] function uses this
 * function internally.
 *
 * Return value: (transfer none) (array length=num_scripts) (nullable):
 *   An array of `GUnicodeScript` values, with the number of entries in
 *   the array stored in @num_scripts, or %NULL if Pango does not have
 *   any information about this particular language tag (also the case
 *   if @language is %NULL).
 */
const GUnicodeScript *
pango_language_get_scripts (PangoLanguage *language,
			    int           *num_scripts)
{
  const PangoScriptForLang *script_for_lang;
  unsigned int j;

  script_for_lang = FIND_BEST_LANG_MATCH_CACHED (language,
						 script_for_lang,
						 pango_script_for_lang);

  if (!script_for_lang || script_for_lang->scripts[0] == 0)
    {
      if (num_scripts)
	*num_scripts = 0;

      return NULL;
    }

  if (num_scripts)
    {
      for (j = 0; j < G_N_ELEMENTS (script_for_lang->scripts); j++)
	if (script_for_lang->scripts[j] == 0)
	  break;

      g_assert (j > 0);

      *num_scripts = j;
    }

  return (const GUnicodeScript *) script_for_lang->scripts;
}

/**
 * pango_language_includes_script:
 * @language: (nullable): a `PangoLanguage`
 * @script: a `GUnicodeScript`
 *
 * Determines if @script is one of the scripts used to
 * write @language.
 *
 * The returned value is conservative; if nothing is known about
 * the language tag @language, %TRUE will be returned, since, as
 * far as Pango knows, @script might be used to write @language.
 *
 * This routine is used in Pango's itemization process when
 * determining if a supplied language tag is relevant to
 * a particular section of text. It probably is not useful
 * for applications in most circumstances.
 *
 * This function uses [method@Pango.Language.get_scripts] internally.
 *
 * Return value: %TRUE if @script is one of the scripts used
 *   to write @language or if nothing is known about @language
 *   (including the case that @language is %NULL), %FALSE otherwise.
 */
gboolean
pango_language_includes_script (PangoLanguage *language,
				GUnicodeScript    script)
{
  const GUnicodeScript *scripts;
  int num_scripts, j;

/* copied from the one in pango-script.c */
#define REAL_SCRIPT(script) \
  ((script) > G_UNICODE_SCRIPT_INHERITED && (script) != G_UNICODE_SCRIPT_UNKNOWN)

  if (!REAL_SCRIPT (script))
    return TRUE;

#undef REAL_SCRIPT

  scripts = pango_language_get_scripts (language, &num_scripts);
  if (!scripts)
    return TRUE;

  for (j = 0; j < num_scripts; j++)
    if (scripts[j] == script)
      return TRUE;

  return FALSE;
}




/*
 * From script to language
 */


static PangoLanguage **
parse_default_languages (void)
{
  char *p, *p_copy;
  gboolean done = FALSE;
  GPtrArray *langs;

  p = getenv ("PANGO_LANGUAGE");

  if (p == NULL)
    p = getenv ("LANGUAGE");

  if (p == NULL)
    return NULL;

  p_copy = p = g_strdup (p);

  langs = g_ptr_array_new ();

  while (!done)
    {
      char *end = strpbrk (p, LANGUAGE_SEPARATORS);
      if (!end)
	{
	  end = p + strlen (p);
	  done = TRUE;
	}
      else
        *end = '\0';

      /* skip empty languages, and skip the language 'C' */
      if (p != end && !(p + 1 == end && *p == 'C'))
        {
	  PangoLanguage *l = pango_language_from_string (p);
	  
	  g_ptr_array_add (langs, l);
	}

      if (!done)
	p = end + 1;
    }

  g_ptr_array_add (langs, NULL);

  g_free (p_copy);

  return (PangoLanguage **) g_ptr_array_free (langs, FALSE);
}

G_LOCK_DEFINE_STATIC (languages);
static gboolean initialized = FALSE; /* MT-safe */
static PangoLanguage * const * languages = NULL; /* MT-safe */
static GHashTable *hash = NULL; /* MT-safe */

static PangoLanguage *
_pango_script_get_default_language (GUnicodeScript script)
{
  PangoLanguage *result, * const * p;

  G_LOCK (languages);

  if (G_UNLIKELY (!initialized))
    {
      languages = parse_default_languages ();

      if (languages)
	hash = g_hash_table_new (NULL, NULL);

      initialized = TRUE;
    }

  if (!languages)
    {
      result = NULL;
      goto out;
    }

  if (g_hash_table_lookup_extended (hash, GINT_TO_POINTER (script), NULL, (gpointer *) (gpointer) &result))
    goto out;

  for (p = languages; *p; p++)
    if (pango_language_includes_script (*p, script))
      break;
  result = *p;

  g_hash_table_insert (hash, GINT_TO_POINTER (script), result);

out:
  G_UNLOCK (languages);

  return result;
}

/**
 * pango_language_get_preferred:
 *
 * Returns the list of languages that the user prefers.
 *
 * The list is specified by the `PANGO_LANGUAGE` or `LANGUAGE`
 * environment variables, in order of preference. Note that this
 * list does not necessarily include the language returned by
 * [func@Pango.Language.get_default].
 *
 * When choosing language-specific resources, such as the sample
 * text returned by [method@Pango.Language.get_sample_string],
 * you should first try the default language, followed by the
 * languages returned by this function.
 *
 * Returns: (transfer none) (nullable): a %NULL-terminated array
 *   of `PangoLanguage`*
 */
PangoLanguage **
pango_language_get_preferred (void)
{
  /* We call this just for its side-effect of initializing languages */
  _pango_script_get_default_language (G_UNICODE_SCRIPT_COMMON);

  return (PangoLanguage **) languages;
}

/**
 * pango_script_get_sample_language:
 * @script: a `GUnicodeScript`
 *
 * Finds a language tag that is reasonably representative of @script.
 *
 * The language will usually be the most widely spoken or used language
 * written in that script: for instance, the sample language for
 * %G_UNICODE_SCRIPT_CYRILLIC is ru (Russian), the sample language for
 * %G_UNICODE_SCRIPT_ARABIC is ar.
 *
 * For some scripts, no sample language will be returned because
 * there is no language that is sufficiently representative. The
 * best example of this is %G_UNICODE_SCRIPT_HAN, where various different
 * variants of written Chinese, Japanese, and Korean all use
 * significantly different sets of Han characters and forms
 * of shared characters. No sample language can be provided
 * for many historical scripts as well.
 *
 * As of 1.18, this function checks the environment variables
 * `PANGO_LANGUAGE` and `LANGUAGE` (checked in that order) first.
 * If one of them is set, it is parsed as a list of language tags
 * separated by colons or other separators. This function
 * will return the first language in the parsed list that Pango
 * believes may use @script for writing. This last predicate
 * is tested using [method@Pango.Language.includes_script]. This can
 * be used to control Pango's font selection for non-primary
 * languages. For example, a `PANGO_LANGUAGE` enviroment variable
 * set to "en:fa" makes Pango choose fonts suitable for Persian (fa)
 * instead of Arabic (ar) when a segment of Arabic text is found
 * in an otherwise non-Arabic text. The same trick can be used to
 * choose a default language for %G_UNICODE_SCRIPT_HAN when setting
 * context language is not feasible.
 *
 * Return value: (nullable): a `PangoLanguage` that is representative
 *   of the script
 */
PangoLanguage *
pango_script_get_sample_language (GUnicodeScript script)
{
  /* Note that in the following, we want
   * pango_language_includes_script() for the sample language
   * to include the script, so alternate orthographies
   * (Shavian for English, Osmanya for Somali, etc), typically
   * have no sample language
   */
  static const char sample_languages[][4] = {
    "",    /* G_UNICODE_SCRIPT_COMMON */
    "",    /* G_UNICODE_SCRIPT_INHERITED */
    "ar",  /* G_UNICODE_SCRIPT_ARABIC */
    "hy",  /* G_UNICODE_SCRIPT_ARMENIAN */
    "bn",  /* G_UNICODE_SCRIPT_BENGALI */
    /* Used primarily in Taiwan, but not part of the standard
     * zh-tw orthography  */
    "",    /* G_UNICODE_SCRIPT_BOPOMOFO */
    "chr", /* G_UNICODE_SCRIPT_CHEROKEE */
    "cop", /* G_UNICODE_SCRIPT_COPTIC */
    "ru",  /* G_UNICODE_SCRIPT_CYRILLIC */
    /* Deseret was used to write English */
    "",    /* G_UNICODE_SCRIPT_DESERET */
    "hi",  /* G_UNICODE_SCRIPT_DEVANAGARI */
    "am",  /* G_UNICODE_SCRIPT_ETHIOPIC */
    "ka",  /* G_UNICODE_SCRIPT_GEORGIAN */
    "",    /* G_UNICODE_SCRIPT_GOTHIC */
    "el",  /* G_UNICODE_SCRIPT_GREEK */
    "gu",  /* G_UNICODE_SCRIPT_GUJARATI */
    "pa",  /* G_UNICODE_SCRIPT_GURMUKHI */
    "",    /* G_UNICODE_SCRIPT_HAN */
    "ko",  /* G_UNICODE_SCRIPT_HANGUL */
    "he",  /* G_UNICODE_SCRIPT_HEBREW */
    "ja",  /* G_UNICODE_SCRIPT_HIRAGANA */
    "kn",  /* G_UNICODE_SCRIPT_KANNADA */
    "ja",  /* G_UNICODE_SCRIPT_KATAKANA */
    "km",  /* G_UNICODE_SCRIPT_KHMER */
    "lo",  /* G_UNICODE_SCRIPT_LAO */
    "en",  /* G_UNICODE_SCRIPT_LATIN */
    "ml",  /* G_UNICODE_SCRIPT_MALAYALAM */
    "mn",  /* G_UNICODE_SCRIPT_MONGOLIAN */
    "my",  /* G_UNICODE_SCRIPT_MYANMAR */
    /* Ogham was used to write old Irish */
    "",    /* G_UNICODE_SCRIPT_OGHAM */
    "",    /* G_UNICODE_SCRIPT_OLD_ITALIC */
    "or",  /* G_UNICODE_SCRIPT_ORIYA */
    "",    /* G_UNICODE_SCRIPT_RUNIC */
    "si",  /* G_UNICODE_SCRIPT_SINHALA */
    "syr", /* G_UNICODE_SCRIPT_SYRIAC */
    "ta",  /* G_UNICODE_SCRIPT_TAMIL */
    "te",  /* G_UNICODE_SCRIPT_TELUGU */
    "dv",  /* G_UNICODE_SCRIPT_THAANA */
    "th",  /* G_UNICODE_SCRIPT_THAI */
    "bo",  /* G_UNICODE_SCRIPT_TIBETAN */
    "iu",  /* G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL */
    "",    /* G_UNICODE_SCRIPT_YI */
    "tl",  /* G_UNICODE_SCRIPT_TAGALOG */
    /* Phillipino languages/scripts */
    "hnn", /* G_UNICODE_SCRIPT_HANUNOO */
    "bku", /* G_UNICODE_SCRIPT_BUHID */
    "tbw", /* G_UNICODE_SCRIPT_TAGBANWA */

    "",    /* G_UNICODE_SCRIPT_BRAILLE */
    "",    /* G_UNICODE_SCRIPT_CYPRIOT */
    "",    /* G_UNICODE_SCRIPT_LIMBU */
    /* Used for Somali (so) in the past */
    "",    /* G_UNICODE_SCRIPT_OSMANYA */
    /* The Shavian alphabet was designed for English */
    "",    /* G_UNICODE_SCRIPT_SHAVIAN */
    "",    /* G_UNICODE_SCRIPT_LINEAR_B */
    "",    /* G_UNICODE_SCRIPT_TAI_LE */
    "uga", /* G_UNICODE_SCRIPT_UGARITIC */

    "",    /* G_UNICODE_SCRIPT_NEW_TAI_LUE */
    "bug", /* G_UNICODE_SCRIPT_BUGINESE */
    /* The original script for Old Church Slavonic (chu), later
     * written with Cyrillic */
    "",    /* G_UNICODE_SCRIPT_GLAGOLITIC */
    /* Used for for Berber (ber), but Arabic script is more common */
    "",    /* G_UNICODE_SCRIPT_TIFINAGH */
    "syl", /* G_UNICODE_SCRIPT_SYLOTI_NAGRI */
    "peo", /* G_UNICODE_SCRIPT_OLD_PERSIAN */
    "",    /* G_UNICODE_SCRIPT_KHAROSHTHI */

    "",    /* G_UNICODE_SCRIPT_UNKNOWN */
    "",    /* G_UNICODE_SCRIPT_BALINESE */
    "",    /* G_UNICODE_SCRIPT_CUNEIFORM */
    "",    /* G_UNICODE_SCRIPT_PHOENICIAN */
    "",    /* G_UNICODE_SCRIPT_PHAGS_PA */
    "nqo", /* G_UNICODE_SCRIPT_NKO */

    /* Unicode-5.1 additions */
    "",    /* G_UNICODE_SCRIPT_KAYAH_LI */
    "",    /* G_UNICODE_SCRIPT_LEPCHA */
    "",    /* G_UNICODE_SCRIPT_REJANG */
    "",    /* G_UNICODE_SCRIPT_SUNDANESE */
    "",    /* G_UNICODE_SCRIPT_SAURASHTRA */
    "",    /* G_UNICODE_SCRIPT_CHAM */
    "",    /* G_UNICODE_SCRIPT_OL_CHIKI */
    "",    /* G_UNICODE_SCRIPT_VAI */
    "",    /* G_UNICODE_SCRIPT_CARIAN */
    "",    /* G_UNICODE_SCRIPT_LYCIAN */
    "",    /* G_UNICODE_SCRIPT_LYDIAN */

    /* Unicode-6.0 additions */
    "",    /* G_UNICODE_SCRIPT_BATAK */
    "",    /* G_UNICODE_SCRIPT_BRAHMI */
    "",    /* G_UNICODE_SCRIPT_MANDAIC */

    /* Unicode-6.1 additions */
    "",    /* G_UNICODE_SCRIPT_CHAKMA */
    "",    /* G_UNICODE_SCRIPT_MEROITIC_CURSIVE */
    "",    /* G_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS */
    "",    /* G_UNICODE_SCRIPT_MIAO */
    "",    /* G_UNICODE_SCRIPT_SHARADA */
    "",    /* G_UNICODE_SCRIPT_SORA_SOMPENG */
    "",    /* G_UNICODE_SCRIPT_TAKRI */
  };
  const char *sample_language;
  PangoLanguage *result;

  g_return_val_if_fail (script >= 0, NULL);

  if ((guint)script >= G_N_ELEMENTS (sample_languages))
    return NULL;

  result = _pango_script_get_default_language (script);
  if (result)
    return result;

  sample_language = sample_languages[script];

  if (!sample_language[0])
    return NULL;
  else
    return pango_language_from_string (sample_language);
}
