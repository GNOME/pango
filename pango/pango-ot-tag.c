/* Pango
 * pango-ot-tag.h:
 *
 * Copyright (C) 2007 Red Hat Software
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

#include "pango-ot.h"

typedef union {
  char string[4];
  guint32 integer;
} Tag;

/*
 * complete list at:
 * http://www.microsoft.com/typography/developers/opentype/scripttags.aspx
 */
static const Tag ot_scripts[] = {
  {"DFLT"},	/* PANGO_SCRIPT_COMMON */
  {"DFLT"},	/* PANGO_SCRIPT_INHERITED */
  {"arab"},	/* PANGO_SCRIPT_ARABIC */
  {"armn"},	/* PANGO_SCRIPT_ARMENIAN */
  {"beng"},	/* PANGO_SCRIPT_BENGALI */
  {"bopo"},	/* PANGO_SCRIPT_BOPOMOFO */
  {"cher"},	/* PANGO_SCRIPT_CHEROKEE */
  {"copt"},	/* PANGO_SCRIPT_COPTIC */
  {"cyrl"},	/* PANGO_SCRIPT_CYRILLIC */
  {"dsrt"},	/* PANGO_SCRIPT_DESERET */
  {"deva"},	/* PANGO_SCRIPT_DEVANAGARI */
  {"ethi"},	/* PANGO_SCRIPT_ETHIOPIC */
  {"geor"},	/* PANGO_SCRIPT_GEORGIAN */
  {"goth"},	/* PANGO_SCRIPT_GOTHIC */
  {"grek"},	/* PANGO_SCRIPT_GREEK */
  {"gujr"},	/* PANGO_SCRIPT_GUJARATI */
  {"guru"},	/* PANGO_SCRIPT_GURMUKHI */
  {"hani"},	/* PANGO_SCRIPT_HAN */
  {"hang"},	/* PANGO_SCRIPT_HANGUL */
  {"hebr"},	/* PANGO_SCRIPT_HEBREW */
  {"kana"},	/* PANGO_SCRIPT_HIRAGANA */
  {"knda"},	/* PANGO_SCRIPT_KANNADA */
  {"kana"},	/* PANGO_SCRIPT_KATAKANA */
  {"khmr"},	/* PANGO_SCRIPT_KHMER */
  {"lao "},	/* PANGO_SCRIPT_LAO */
  {"latn"},	/* PANGO_SCRIPT_LATIN */
  {"mlym"},	/* PANGO_SCRIPT_MALAYALAM */
  {"mong"},	/* PANGO_SCRIPT_MONGOLIAN */
  {"mymr"},	/* PANGO_SCRIPT_MYANMAR */
  {"ogam"},	/* PANGO_SCRIPT_OGHAM */
  {"ital"},	/* PANGO_SCRIPT_OLD_ITALIC */
  {"orya"},	/* PANGO_SCRIPT_ORIYA */
  {"runr"},	/* PANGO_SCRIPT_RUNIC */
  {"sinh"},	/* PANGO_SCRIPT_SINHALA */
  {"syrc"},	/* PANGO_SCRIPT_SYRIAC */
  {"taml"},	/* PANGO_SCRIPT_TAMIL */
  {"telu"},	/* PANGO_SCRIPT_TELUGU */
  {"thaa"},	/* PANGO_SCRIPT_THAANA */
  {"thai"},	/* PANGO_SCRIPT_THAI */
  {"tibt"},	/* PANGO_SCRIPT_TIBETAN */
  {"cans"},	/* PANGO_SCRIPT_CANADIAN_ABORIGINAL */
  {"yi  "},	/* PANGO_SCRIPT_YI */
  {"tglg"},	/* PANGO_SCRIPT_TAGALOG */
  {"hano"},	/* PANGO_SCRIPT_HANUNOO */
  {"buhd"},	/* PANGO_SCRIPT_BUHID */
  {"tagb"},	/* PANGO_SCRIPT_TAGBANWA */
  {"brai"},	/* PANGO_SCRIPT_BRAILLE */
  {"cprt"},	/* PANGO_SCRIPT_CYPRIOT */
  {"limb"},	/* PANGO_SCRIPT_LIMBU */
  {"osma"},	/* PANGO_SCRIPT_OSMANYA */
  {"shaw"},	/* PANGO_SCRIPT_SHAVIAN */
  {"linb"},	/* PANGO_SCRIPT_LINEAR_B */
  {"tale"},	/* PANGO_SCRIPT_TAI_LE */
  {"ugar"},	/* PANGO_SCRIPT_UGARITIC */
  {"talu"},	/* PANGO_SCRIPT_NEW_TAI_LUE */
  {"bugi"},	/* PANGO_SCRIPT_BUGINESE */
  {"glag"},	/* PANGO_SCRIPT_GLAGOLITIC */
  {"tfng"},	/* PANGO_SCRIPT_TIFINAGH */
  {"sylo"},	/* PANGO_SCRIPT_SYLOTI_NAGRI */
  {"xpeo"},	/* PANGO_SCRIPT_OLD_PERSIAN */
  {"khar"},	/* PANGO_SCRIPT_KHAROSHTHI */
  {"DFLT"},	/* PANGO_SCRIPT_UNKNOWN */
  {"bali"},	/* PANGO_SCRIPT_BALINESE */
  {"xsux"},	/* PANGO_SCRIPT_CUNEIFORM */
  {"phnx"},	/* PANGO_SCRIPT_PHOENICIAN */
  {"phag"},	/* PANGO_SCRIPT_PHAGS_PA */
  {"nko "}	/* PANGO_SCRIPT_NKO */
};

/**
 * pango_ot_tag_from_script:
 * @script: A #PangoScript
 *
 * Finds the OpenType script tag corresponding to @script.
 *
 * The %PANGO_SCRIPT_COMMON, %PANGO_SCRIPT_INHERITED, and
 * %PANGO_SCRIPT_UNKNOWN scripts are mapped to the OpenType
 * 'DFLT' script tag that is also defined as
 * %PANGO_OT_TAG_DEFAULT_SCRIPT.
 *
 * Note that multiple #PangoScript values may map to the same
 * OpenType script tag.  In particular, %PANGO_SCRIPT_HIRAGANA
 * and %PANGO_SCRIPT_KATAKANA both map to the OT tag 'kana'.
 *
 * Return value: #PangoOTTag corresponding to @script or
 * %PANGO_OT_TAG_DEFAULT_SCRIPT if none found.
 *
 * Since: 1.18
 **/
PangoOTTag
pango_ot_tag_from_script (PangoScript script)
{
  g_return_val_if_fail (script >= 0, PANGO_OT_TAG_DEFAULT_SCRIPT);

  if ((guint)script >= G_N_ELEMENTS (ot_scripts))
    return PANGO_OT_TAG_DEFAULT_SCRIPT;

  return GUINT32_FROM_BE (ot_scripts[script].integer);
}

/**
 * pango_ot_tag_to_script:
 * @script_tag: A #PangoOTTag OpenType script tag
 *
 * Finds the #PangoScript corresponding to @script_tag.
 *
 * The 'DFLT' script tag is mapped to %PANGO_SCRIPT_COMMON.
 *
 * Note that an OpenType script tag may correspond to multiple
 * #PangoScript values.  In such cases, the #PangoScript value
 * with the smallest value is returned.
 * In particular, %PANGO_SCRIPT_HIRAGANA
 * and %PANGO_SCRIPT_KATAKANA both map to the OT tag 'kana'.
 * This function will return %PANGO_SCRIPT_HIRAGANA for
 * 'kana'.
 *
 * Return value: #PangoScript corresponding to @script_tag or
 * %PANGO_SCRIPT_UNKNOWN if none found.
 *
 * Since: 1.18
 **/
PangoScript
pango_ot_tag_to_script (PangoOTTag script_tag)
{
  PangoScript i;
  guint32 be_tag = GUINT32_TO_BE (script_tag);

  for (i = 0; i < (PangoScript) G_N_ELEMENTS (ot_scripts); i++)
    {
      guint32 tag = ot_scripts[i].integer;

      if (tag == be_tag)
        return i;
    }

  return PANGO_SCRIPT_UNKNOWN;
}


typedef struct {
  char language[6];
  Tag tag;
} LangTag;

/*
 * complete list at:
 * http://www.microsoft.com/OpenType/OTSpec/languagetags.htm
 *
 * Generated by intersecting the above list with the ISO 639-2 codes
 * and then adjusting manually.  A lot of items missing still, feel
 * free to add.  Keep sorted for bsearch purpose.
 */
static const LangTag ot_languages[] = {
  {"aa",	{"AFR "}},
  {"ab",	{"ABK "}},
  {"ady",	{"ADY "}},
  {"af",	{"AFK "}},
  {"am",	{"AMH "}},
  {"ar",	{"ARA "}},
  {"as",	{"ASM "}},
  {"awa",	{"AWA "}},
  {"ay",	{"AYM "}},
  {"az",	{"AZE "}},
  {"ba",	{"BSH "}},
  {"bal",	{"BLI "}},
  {"bem",	{"BEM "}},
  {"ber",	{"BBR "}},
  {"bg",	{"BGR "}},
  {"bho",	{"BHO "}},
  {"bik",	{"BIK "}},
  {"bin",	{"EDO "}},
  {"bm",	{"BMB "}},
  {"bn",	{"BEN "}},
  {"bo",	{"TIB "}},
  {"br",	{"BRE "}},
  {"brh",	{"BRH "}},
  {"ca",	{"CAT "}},
  {"ce",	{"CHE "}},
  {"ceb",	{"CEB "}},
  {"chp",	{"CHP "}},
  {"chr",	{"CHR "}},
  {"cop",	{"COP "}},
  {"cr",	{"CRE "}},
  {"crh",	{"CRT "}},
  {"cs",	{"CSY "}},
  {"cu",	{"CSL "}},
  {"cv",	{"CHU "}},
  {"cy",	{"WEL "}},
  {"da",	{"DAN "}},
  {"dar",	{"DAR "}},
  {"de",	{"DEU "}},
  {"din",	{"DNK "}},
  {"doi",	{"DGR "}},
  {"dsb",	{"LSB "}},
  {"dv",	{"DHV "}},
  {"dz",	{"DZN "}},
  {"ee",	{"EWE "}},
  {"efi",	{"EFI "}},
  {"el",	{"ELL "}},
  {"en",	{"ENG "}},
  {"eo",	{"NTO "}},
  {"es",	{"ESP "}},
  {"et",	{"ETI "}},
  {"eu",	{"EUQ "}},
  {"fa",	{"FAR "}},
  {"ff",	{"FUL "}},
  {"fi",	{"FIN "}},
  {"fil",	{"PIL "}},
  {"fj",	{"FJI "}},
  {"fo",	{"FOS "}},
  {"fon",	{"FON "}},
  {"fr",	{"FRA "}},
  {"fur",	{"FRL "}},
  {"fy",	{"FRI "}},
  {"ga",	{"IRI "}},
  {"gaa",	{"GAD "}},
  {"gd",	{"GAE "}},
  {"gl",	{"GAL "}},
  {"gn",	{"GUA "}},
  {"gon",	{"GON "}},
  {"gu",	{"GUJ "}},
  {"ha",	{"HAU "}},
  {"he",	{"IWR "}},
  {"hi",	{"HIN "}},
  {"hil",	{"HIL "}},
  {"hr",	{"HRV "}},
  {"hsb",	{"USB "}},
  {"ht",	{"HAI "}},
  {"hu",	{"HUN "}},
  {"hy",	{"HYE "}},
  {"id",	{"IND "}},
  {"ig",	{"IBO "}},
  {"inc",	{"SRK "}},
  {"ine",	{"KHW "}},
  {"inh",	{"ING "}},
  {"is",	{"ISL "}},
  {"it",	{"ITA "}},
  {"iu",	{"INU "}},
  {"ja",	{"JAN "}},
  {"jv",	{"JAV "}},
  {"ka",	{"KAT "}},
  {"kam",	{"KMB "}},
  {"kbd",	{"KAB "}},
  {"kha",	{"KSI "}},
  {"ki",	{"KIK "}},
  {"kk",	{"KAZ "}},
  {"kl",	{"GRN "}},
  {"km",	{"KHM "}},
  {"kn",	{"KAN "}},
  {"ko",	{"KOR "}},
  {"kok",	{"KOK "}},
  {"kpe",	{"KPL "}},
  {"kr",	{"KNR "}},
  {"krl",	{"KRL "}},
  {"kru",	{"KUU "}},
  {"ks",	{"KSH "}},
  {"ku",	{"KUR "}},
  {"kum",	{"KUM "}},
  {"ky",	{"KIR "}},
  {"la",	{"LAT "}},
  {"lad",	{"JUD "}},
  {"lbj",	{"LDK "}},
  {"ln",	{"LIN "}},
  {"lo",	{"LAO "}},
  {"lt",	{"LTH "}},
  {"lv",	{"LVI "}},
  {"mai",	{"MTH "}},
  {"mdf",	{"MOK "}},
  {"men",	{"MDE "}},
  {"mg",	{"MLG "}},
  {"mi",	{"MRI "}},
  {"mk",	{"MKD "}},
  {"mkh",	{"KUY "}},
  {"ml",	{"MLR "}},
  {"mnc",	{"MCH "}},
  {"mni",	{"MNI "}},
  {"mnk",	{"MND "}},
  {"mo",	{"MOL "}},
  {"mr",	{"MAR "}},
  {"ms",	{"MLY "}},
  {"mt",	{"MTS "}},
  {"mwr",	{"MAW "}},
  {"my",	{"BRM "}},
  {"myv",	{"ERZ "}},
  {"ne",	{"NEP "}},
  {"nl",	{"NLD "}},
  {"no",	{"NOR "}},
  {"ny",	{"CHI "}},
  {"oc",	{"PRO "}},
  {"om",	{"ORO "}},
  {"or",	{"ORI "}},
  {"os",	{"OSS "}},
  {"pa",	{"PAN "}},
  {"pi",	{"PAL "}},
  {"pl",	{"PLK "}},
  {"ps",	{"PAS "}},
  {"pt",	{"PTG "}},
  {"ro",	{"ROM "}},
  {"rom",	{"ROY "}},
  {"ru",	{"RUS "}},
  {"sa",	{"SAN "}},
  {"sat",	{"SAT "}},
  {"sd",	{"SND "}},
  {"sel",	{"SEL "}},
  {"sg",	{"SGO "}},
  {"shn",	{"SHN "}},
  {"si",	{"SNH "}},
  {"sid",	{"SID "}},
  {"sk",	{"SKY "}},
  {"sl",	{"SLV "}},
  {"sm",	{"SMO "}},
  {"smj",	{"LSM "}},
  {"smn",	{"ISM "}},
  {"sms",	{"SKS "}},
  {"snk",	{"SNK "}},
  {"so",	{"SML "}},
  {"sq",	{"SQI "}},
  {"sr",	{"SRB "}},
  {"srr",	{"SRR "}},
  {"sv",	{"SVE "}},
  {"sw",	{"SWK "}},
  {"syr",	{"SYR "}},
  {"ta",	{"TAM "}},
  {"te",	{"TEL "}},
  {"tg",	{"TAJ "}},
  {"th",	{"THA "}},
  {"ti",	{"TGY "}},
  {"tig",	{"TGR "}},
  {"tk",	{"TKM "}},
  {"tn",	{"TNA "}},
  {"tr",	{"TRK "}},
  {"ts",	{"TSG "}},
  {"tw",	{"TWI "}},
  {"udm",	{"UDM "}},
  {"ug",	{"UYG "}},
  {"uk",	{"UKR "}},
  {"ur",	{"URD "}},
  {"uz",	{"UZB "}},
  {"ve",	{"VEN "}},
  {"vi",	{"VIT "}},
  {"wo",	{"WLF "}},
  {"xal",	{"KLM "}},
  {"xh",	{"XHS "}},
  {"yi",	{"JII "}},
  {"yo",	{"YBA "}},
  {"zh-cn",	{"ZHS "}},
  {"zh-hk",	{"ZHH "}},
  {"zh-mo",	{"ZHT "}},
  {"zh-sg",	{"ZHS "}},
  {"zh-tw",	{"ZHT "}},
  {"zu",	{"ZUL "}}
};

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

/**
 * pango_ot_tag_from_language:
 * @language: A #PangoLanguage, or %NULL
 *
 * Finds the OpenType language-system tag best describing @language.
 *
 * Return value: #PangoOTTag best matching @language or
 * %PANGO_OT_TAG_DEFAULT_LANGUAGE if none found or if @language
 * is %NULL.
 *
 * Since: 1.18
 **/
PangoOTTag
pango_ot_tag_from_language (PangoLanguage *language)
{
  const char *lang_str;
  LangTag *lang_tag;

  if (language == NULL)
    return PANGO_OT_TAG_DEFAULT_LANGUAGE;

  lang_str = pango_language_to_string (language);

  /* find a language matching in the first component */
  lang_tag = bsearch (lang_str, ot_languages,
		      G_N_ELEMENTS (ot_languages), sizeof (LangTag),
		      lang_compare_first_component);

  /* we now need to find the best language matching */
  if (lang_tag)
    {
      gboolean found = FALSE;

      /* go to the final one matching in the first component */
      while (lang_tag + 1 < ot_languages + G_N_ELEMENTS (ot_languages) &&
	     lang_compare_first_component (lang_str, lang_tag + 1) == 0)
        lang_tag++;

      /* go back, find which one matches completely */
      while (lang_tag >= ot_languages &&
	     lang_compare_first_component (lang_str, lang_tag) == 0)
        {
	  if (pango_language_matches (language, lang_tag->language))
	    {
	      found = TRUE;
	      break;
	    }

          lang_tag--;
	}

      if (!found)
        lang_tag = NULL;
    }

  if (lang_tag)
    return GUINT32_FROM_BE (lang_tag->tag.integer);

  return PANGO_OT_TAG_DEFAULT_LANGUAGE;
}

/**
 * pango_ot_tag_to_language:
 * @language_tag: A #PangoOTTag OpenType language-system tag
 *
 * Finds a #PangoLanguage corresponding to @language_tag.
 *
 * Return value: #PangoLanguage best matching @language_tag or
 * #PangoLanguage corresponding to the string "xx" if none found.
 *
 * Since: 1.18
 **/
PangoLanguage *
pango_ot_tag_to_language (PangoOTTag language_tag)
{
  int i;
  guint32 be_tag = GUINT32_TO_BE (language_tag);

  for (i = 0; i < (int) G_N_ELEMENTS (ot_languages); i++)
    {
      guint32 tag = ot_languages[i].tag.integer;

      if (tag == be_tag)
        return pango_language_from_string (ot_languages[i].language);
    }

  return pango_language_from_string ("xx");
}
