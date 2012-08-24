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

#include "config.h"

#include "pango-ot-private.h"

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
  hb_tag_t tag1, tag2;
  hb_ot_tags_from_script (hb_glib_script_to_script (script), &tag1, &tag2);
  return (PangoOTTag) tag1;
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
  return (PangoScript) hb_glib_script_from_script (hb_ot_tag_to_script ((hb_tag_t) script_tag));
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
  return (PangoOTTag) hb_ot_tag_from_language (hb_language_from_string (pango_language_to_string (language), -1));
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
  return pango_language_from_string (hb_language_to_string (hb_ot_tag_to_language ((hb_tag_t) language_tag)));
}
