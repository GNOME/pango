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

#include <config.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

#include "pango-language.h"
#include "pango-impl-utils.h"

#define LANGUAGE_SEPARATORS ";:, \t"

static const char canon_map[256] = {
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,  '-',  0,   0,
  '0', '1', '2', '3', '4', '5', '6', '7',  '8', '9',  0,   0,   0,   0,   0,   0,
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
pango_language_free (PangoLanguage *language)
{
  return; /* nothing */
}

GType
pango_language_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type = g_boxed_type_register_static (I_("PangoLanguage"),
					     (GBoxedCopyFunc)pango_language_copy,
					     (GBoxedFreeFunc)pango_language_free);
  return our_type;
}

/**
 * _pango_get_lc_ctype:
 *
 * Return the Unix-style locale string for the language currently in
 * effect. On Unix systems, this is the return value from
 * <literal>setlocale(LC_CTYPE, NULL)</literal>, and the user can
 * affect this through the environment variables LC_ALL, LC_CTYPE or
 * LANG (checked in that order). The locale strings typically is in
 * the form lang_COUNTRY, where lang is an ISO-639 language code, and
 * COUNTRY is an ISO-3166 country code. For instance, sv_FI for
 * Swedish as written in Finland or pt_BR for Portuguese as written in
 * Brazil.
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
#else
  return g_strdup (setlocale (LC_CTYPE, NULL));
#endif
}

/**
 * pango_language_get_default:
 *
 * Returns the #PangoLanguage for the current locale of the process.
 * Note that this can change over the life of an application.
 *
 * On Unix systems, this is the return value is derived from
 * <literal>setlocale(LC_CTYPE, NULL)</literal>, and the user can
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
 * Your application should call <literal>setlocale(LC_ALL, "");</literal>
 * for the user settings to take effect.  Gtk+ does this in its initialization
 * functions automatically (by calling gtk_set_locale()).
 * See <literal>man setlocale</literal> for more details.
 *
 * Return value: the default language as a #PangoLanguage, must not be
 *               freed.
 *
 * Since: 1.16
 **/
PangoLanguage *
pango_language_get_default (void)
{
  static PangoLanguage *result = NULL;

  if (G_UNLIKELY (!result))
    {
      gchar *lang = _pango_get_lc_ctype ();
      result = pango_language_from_string (lang);
      g_free (lang);
    }

  return result;
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
 * Use pango_language_get_default() if you want to get the #PangoLanguage for
 * the current locale of the process.
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

  if (G_UNLIKELY (!hash))
    hash = g_hash_table_new (lang_hash, lang_equal);
  else
    {
      result = g_hash_table_lookup (hash, language);
      if (result)
	return (PangoLanguage *)result;
    }

  len = strlen (language);
  result = g_malloc (len + 1);

  p = result;
  while ((*(p++) = canon_map[*(guchar *)language++]))
    ;

  g_hash_table_insert (hash, result, result);

  return (PangoLanguage *)result;
}

/**
 * pango_language_matches:
 * @language: a language tag (see pango_language_from_string()),
 *            %NULL is allowed and matches nothing but '*'
 * @range_list: a list of language ranges, separated by ';', ':',
 *   ',', or space characters.
 *   Each element must either be '*', or a RFC 3066 language range
 *   canonicalized as by pango_language_from_string()
 *
 * Checks if a language tag matches one of the elements in a list of
 * language ranges. A language tag is considered to match a range
 * in the list if the range is '*', the range is exactly the tag,
 * or the range is a prefix of the tag, and the character after it
 * in the tag is '-'.
 *
 * Return value: %TRUE if a match was found.
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

typedef struct {
  const char lang[4];
  const char *str;
} LangInfo;

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

static int
lang_info_compare (gconstpointer key,
		   gconstpointer val)
{
  const LangInfo *lang_info = val;

  return lang_compare_first_component (key, lang_info->lang);
}

/* The following array is supposed to contain enough text to tickle all necessary fonts for each
 * of the languages in the following. Yes, it's pretty lame. Not all of the languages
 * in the following have sufficient text to exercise all the accents for the language, and
 * there are obviously many more languages to include as well.
 */
static const LangInfo lang_texts[] = {
  { "ar", "Arabic  \330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205" },
  { "cs", "Czech (\304\215esky)  Dobr\303\275 den" },
  { "da", "Danish (Dansk)  Hej, Goddag" },
  { "el", "Greek (\316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254) \316\223\316\265\316\271\316\254 \317\203\316\261\317\202" },
  { "en", "English Hello" },
  { "eo", "Esperanto Saluton" },
  { "es", "Spanish (Espa\303\261ol) \302\241Hola!" },
  { "et", "Estonian  Tere, Tervist" },
  { "fi", "Finnish (Suomi)  Hei, Hyv\303\244\303\244 p\303\244iv\303\244\303\244" },
  { "fr", "French (Fran\303\247ais)" },
  { "de", "German Gr\303\274\303\237 Gott" },
  { "he", "Hebrew   \327\251\327\234\327\225\327\235" },
  { "it", "Italiano  Ciao, Buon giorno" },
  { "ja", "Japanese (\346\227\245\346\234\254\350\252\236) \343\201\223\343\202\223\343\201\253\343\201\241\343\201\257, \357\275\272\357\276\235\357\276\206\357\276\201\357\276\212" },
  { "ko", "Korean (\355\225\234\352\270\200)   \354\225\210\353\205\225\355\225\230\354\204\270\354\232\224, \354\225\210\353\205\225\355\225\230\354\213\255\353\213\210\352\271\214" },
  { "mt", "Maltese   \304\212aw, Sa\304\247\304\247a" },
  { "nl", "Nederlands, Vlaams Hallo, Dag" },
  { "no", "Norwegian (Norsk) Hei, God dag" },
  { "pl", "Polish   Dzie\305\204 dobry, Hej" },
  { "ru", "Russian (\320\240\321\203\321\201\321\201\320\272\320\270\320\271)" },
  { "sk", "Slovak   Dobr\303\275 de\305\210" },
  { "sv", "Swedish (Svenska) Hej p\303\245 dej, Goddag" },
  { "tr", "Turkish (T\303\274rk\303\247e) Merhaba" },
  { "zh", "Chinese (\344\270\255\346\226\207,\346\231\256\351\200\232\350\257\235,\346\261\211\350\257\255)" }
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
	result = "French (Fran\303\247ais)";     /* Assume iso-8859-1 */
    }
  else
    {
      /* Complete junk
       */

      result = "\330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205 \304\215esky \316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254 Fran\303\247ais \346\227\245\346\234\254\350\252\236 \355\225\234\352\270\200 \320\240\321\203\321\201\321\201\320\272\320\270\320\271 \344\270\255\346\226\207,\346\231\256\351\200\232\350\257\235,\346\261\211\350\257\255 T\303\274rk\303\247e";
    }

  return result;
}

#include "pango-script-lang-table.h"

static int
script_for_lang_compare (gconstpointer key,
			 gconstpointer member)
{
  PangoLanguage *lang = (PangoLanguage *)key;
  const PangoScriptForLang *script_for_lang = member;

  return lang_compare_first_component (lang, script_for_lang->lang);
}

/**
 * pango_language_includes_script:
 * @language: a #PangoLanguage, or %NULL
 * @script: a #PangoScript
 *
 * Determines if @script is one of the scripts used to
 * write @language. The returned value is conservative;
 * if nothing is known about the language tag @language,
 * %TRUE will be returned, since, as far as Pango knows,
 * @script might be used to write @language.
 *
 * This routine is used in Pango's itemization process when
 * determining if a supplied language tag is relevant to
 * a particular section of text. It probably is not useful for
 * applications in most circumstances.
 *
 * Return value: %TRUE if @script is one of the scripts used
 * to write @language or if nothing is known about @language
 * (including the case that @language is %NULL),
 * %FALSE otherwise.
 
 * Since: 1.4
 **/
gboolean
pango_language_includes_script (PangoLanguage *language,
				PangoScript    script)
{
  PangoScriptForLang *script_for_lang;
  unsigned int j;
  const char *lang_str;

#define REAL_SCRIPT(script) \
  ((script) > PANGO_SCRIPT_INHERITED)

  if (language == NULL || !REAL_SCRIPT (script))
    return TRUE;

  lang_str = pango_language_to_string (language);

  /* This bsearch could be optimized to occur only once if
   * we store the pointer to the PangoScriptForLang in the
   * same block as the string value for the PangoLanguage.
   */
  script_for_lang = bsearch (lang_str,
			     pango_script_for_lang,
			     G_N_ELEMENTS (pango_script_for_lang),
			     sizeof (PangoScriptForLang),
			     script_for_lang_compare);
  if (!script_for_lang)
    return TRUE;
  else
    {
      gboolean found = FALSE;

      /* find the best matching language */
     
      /* go to the final one matching in the first component */
      while (script_for_lang + 1 < pango_script_for_lang + G_N_ELEMENTS (pango_script_for_lang) &&
	     script_for_lang_compare (lang_str, script_for_lang + 1) == 0)
        script_for_lang++;

      /* go back, find which one matches completely */
      while (script_for_lang >= pango_script_for_lang &&
	     script_for_lang_compare (lang_str, script_for_lang) == 0)
        {
	  if (pango_language_matches (language, script_for_lang->lang))
	    {
	      found = TRUE;
	      break;
	    }

          script_for_lang--;
	}

      if (!found)
        return TRUE;
    }

  for (j = 0; j < G_N_ELEMENTS (script_for_lang->scripts); j++)
    if (script_for_lang->scripts[j] == script)
      return TRUE;

  return FALSE;
}

static PangoLanguage **
parse_default_languages (void)
{
  char *p;
  gboolean done = FALSE;
  GArray *langs;

  p = getenv ("PANGO_LANGUAGE");

  if (p == NULL)
    p = getenv ("LANGUAGE");

  if (p == NULL)
    return NULL;

  p = g_strdup (p);

  langs = g_array_new (TRUE, FALSE, sizeof (PangoLanguage *));

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
	  
	  g_array_append_val (langs, l);
	}

      if (!done)
	p = end + 1;
    }

  return (PangoLanguage **) g_array_free (langs, FALSE);
}

static PangoLanguage *
_pango_script_get_default_language (PangoScript script)
{
  static gboolean initialized = FALSE;
  static PangoLanguage * const * languages = NULL;
  static GHashTable *hash = NULL;
  PangoLanguage *result, * const * p;

  if (G_UNLIKELY (!initialized))
    {
      languages = parse_default_languages ();

      if (languages)
	hash = g_hash_table_new (NULL, NULL);

      initialized = TRUE;
    }

  if (!languages)
    return NULL;

  if (g_hash_table_lookup_extended (hash, GINT_TO_POINTER (script), NULL, (gpointer *) (gpointer) &result))
    return result;

  for (p = languages; *p; p++)
    if (pango_language_includes_script (*p, script))
      break;
  result = *p;

  g_hash_table_insert (hash, GINT_TO_POINTER (script), result);

  return result;
}

/**
 * pango_script_get_sample_language:
 * @script: a #PangoScript
 *
 * Given a script, finds a language tag that is reasonably
 * representative of that script. This will usually be the
 * most widely spoken or used language written in that script:
 * for instance, the sample language for %PANGO_SCRIPT_CYRILLIC
 * is <literal>ru</literal> (Russian), the sample language
 * for %PANGO_SCRIPT_ARABIC is <literal>ar</literal>.
 *
 * For some
 * scripts, no sample language will be returned because there
 * is no language that is sufficiently representative. The best
 * example of this is %PANGO_SCRIPT_HAN, where various different
 * variants of written Chinese, Japanese, and Korean all use
 * significantly different sets of Han characters and forms
 * of shared characters. No sample language can be provided
 * for many historical scripts as well.
 *
 * As of 1.18, this function checks the environment variables
 * PANGO_LANGUAGE and LANGUAGE (checked in that order) first.
 * If one of them is set, it is parsed as a list of language tags
 * separated by colons or other separators.  This function
 * will return the first language in the parsed list that Pango
 * believes may use @script for writing.  This last predicate
 * is tested using pango_language_includes_script().  This can
 * be used to control Pango's font selection for non-primary
 * languages.  For example, a PANGO_LANGUAGE enviroment variable
 * set to "en:fa" makes Pango choose fonts suitable for Persian (fa) 
 * instead of Arabic (ar) when a segment of Arabic text is found
 * in an otherwise non-Arabic text.  The same trick can be used to
 * choose a default language for %PANGO_SCRIPT_HAN when setting
 * context language is not feasible.
 *
 * Return value: a #PangoLanguage that is representative
 * of the script, or %NULL if no such language exists.
 *
 * Since: 1.4
 **/
PangoLanguage *
pango_script_get_sample_language (PangoScript script)
{
  /* Note that in the following, we want
   * pango_language_includes_script() for the sample language
   * to include the script, so alternate orthographies
   * (Shavian for English, Osmanya for Somali, etc), typically
   * have no sample language
   */
  static const char sample_languages[][4] = {
    "",    /* PANGO_SCRIPT_COMMON */
    "",    /* PANGO_SCRIPT_INHERITED */
    "ar",  /* PANGO_SCRIPT_ARABIC */
    "hy",  /* PANGO_SCRIPT_ARMENIAN */
    "bn",  /* PANGO_SCRIPT_BENGALI */
    /* Used primarily in Taiwan, but not part of the standard
     * zh-tw orthography  */
    "",    /* PANGO_SCRIPT_BOPOMOFO */
    "chr", /* PANGO_SCRIPT_CHEROKEE */
    "cop", /* PANGO_SCRIPT_COPTIC */
    "ru",  /* PANGO_SCRIPT_CYRILLIC */
    /* Deseret was used to write English */
    "",    /* PANGO_SCRIPT_DESERET */
    "hi",  /* PANGO_SCRIPT_DEVANAGARI */
    "am",  /* PANGO_SCRIPT_ETHIOPIC */
    "ka",  /* PANGO_SCRIPT_GEORGIAN */
    "",    /* PANGO_SCRIPT_GOTHIC */
    "el",  /* PANGO_SCRIPT_GREEK */
    "gu",  /* PANGO_SCRIPT_GUJARATI */
    "pa",  /* PANGO_SCRIPT_GURMUKHI */
    "",    /* PANGO_SCRIPT_HAN */
    "ko",  /* PANGO_SCRIPT_HANGUL */
    "he",  /* PANGO_SCRIPT_HEBREW */
    "ja",  /* PANGO_SCRIPT_HIRAGANA */
    "kn",  /* PANGO_SCRIPT_KANNADA */
    "ja",  /* PANGO_SCRIPT_KATAKANA */
    "km",  /* PANGO_SCRIPT_KHMER */
    "lo",  /* PANGO_SCRIPT_LAO */
    "en",  /* PANGO_SCRIPT_LATIN */
    "ml",  /* PANGO_SCRIPT_MALAYALAM */
    "mn",  /* PANGO_SCRIPT_MONGOLIAN */
    "my",  /* PANGO_SCRIPT_MYANMAR */
    /* Ogham was used to write old Irish */
    "",    /* PANGO_SCRIPT_OGHAM */
    "",    /* PANGO_SCRIPT_OLD_ITALIC */
    "or",  /* PANGO_SCRIPT_ORIYA */
    "",    /* PANGO_SCRIPT_RUNIC */
    "si",  /* PANGO_SCRIPT_SINHALA */
    "syr", /* PANGO_SCRIPT_SYRIAC */
    "ta",  /* PANGO_SCRIPT_TAMIL */
    "te",  /* PANGO_SCRIPT_TELUGU */
    "dv",  /* PANGO_SCRIPT_THAANA */
    "th",  /* PANGO_SCRIPT_THAI */
    "bo",  /* PANGO_SCRIPT_TIBETAN */
    "iu",  /* PANGO_SCRIPT_CANADIAN_ABORIGINAL */
    "",    /* PANGO_SCRIPT_YI */
    "tl",  /* PANGO_SCRIPT_TAGALOG */
    /* There are no ISO-636 language codes for the following
     * Phillipino languages/scripts */
    "",    /* PANGO_SCRIPT_HANUNOO */
    "",    /* PANGO_SCRIPT_BUHID */
    "",    /* PANGO_SCRIPT_TAGBANWA */

    "",    /* PANGO_SCRIPT_BRAILLE */
    "",    /* PANGO_SCRIPT_CYPRIOT */
    "",    /* PANGO_SCRIPT_LIMBU */
    /* Used for Somali (so) in the past */
    "",    /* PANGO_SCRIPT_OSMANYA */
    /* The Shavian alphabet was designed for English */
    "",    /* PANGO_SCRIPT_SHAVIAN */
    "",    /* PANGO_SCRIPT_LINEAR_B */
    "",    /* PANGO_SCRIPT_TAI_LE */
    "uga", /* PANGO_SCRIPT_UGARITIC */

    "",    /* PANGO_SCRIPT_NEW_TAI_LUE */
    "bug", /* PANGO_SCRIPT_BUGINESE */
    /* The original script for Old Church Slavonic (chu), later
     * written with Cyrillic */
    "",    /* PANGO_SCRIPT_GLAGOLITIC */
    /* Used for for Berber (ber), but Arabic script is more common */
    "",    /* PANGO_SCRIPT_TIFINAGH */
    /* Syloti Nagri is used for Sylheti, no ISO 639 code */
    "",    /* PANGO_SCRIPT_SYLOTI_NAGRI */
    "peo", /* PANGO_SCRIPT_OLD_PERSIAN */
    "",    /* PANGO_SCRIPT_KHAROSHTHI */

    "",    /* PANGO_SCRIPT_UNKNOWN */
    "",    /* PANGO_SCRIPT_BALINESE */
    "",    /* PANGO_SCRIPT_CUNEIFORM */
    "",    /* PANGO_SCRIPT_PHOENICIAN */
    "",    /* PANGO_SCRIPT_PHAGS_PA */
    "nqo"  /* PANGO_SCRIPT_NKO */
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
