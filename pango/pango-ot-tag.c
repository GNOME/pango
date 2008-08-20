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
 * Generated by intersecting the OpenType language tag list from
 * Draft OpenType 1.5 spec, with with the ISO 639-3 codes from
 * 2008/08/04, matching on name, and finally adjusted manually.
 *
 * Many items still missing.  Those are commented out at the end.
 * Keep sorted for bsearch.
 */
static const LangTag ot_languages[] = {
  {"aa",	{"AFR "}},	/* Afar */
  {"ab",	{"ABK "}},	/* Abkhazian */
  {"abq",	{"ABA "}},	/* Abaza */
  {"ady",	{"ADY "}},	/* Adyghe */
  {"af",	{"AFK "}},	/* Afrikaans */
  {"aiw",	{"ARI "}},	/* Aari */
  {"am",	{"AMH "}},	/* Amharic */
  {"ar",	{"ARA "}},	/* Arabic */
  {"arn",	{"MAP "}},	/* Mapudungun */
  {"as",	{"ASM "}},	/* Assamese */
  {"av",	{"AVR "}},	/* Avaric */
  {"awa",	{"AWA "}},	/* Awadhi */
  {"ay",	{"AYM "}},	/* Aymara */
  {"az",	{"AZE "}},	/* Azerbaijani */
  {"ba",	{"BSH "}},	/* Bashkir */
  {"bal",	{"BLI "}},	/* Baluchi */
  {"bcq",	{"BCH "}},	/* Bench */
  {"bem",	{"BEM "}},	/* Bemba (Zambia) */
  {"bfq",	{"BAD "}},	/* Badaga */
  {"bft",	{"BLT "}},	/* Balti */
  {"bg",	{"BGR "}},	/* Bulgarian */
  {"bhb",	{"BHI "}},	/* Bhili */
  {"bho",	{"BHO "}},	/* Bhojpuri */
  {"bik",	{"BIK "}},	/* Bikol */
  {"bin",	{"EDO "}},	/* Bini */
  {"bm",	{"BMB "}},	/* Bambara */
  {"bn",	{"BEN "}},	/* Bengali */
  {"bo",	{"TIB "}},	/* Tibetan */
  {"br",	{"BRE "}},	/* Breton */
  {"brh",	{"BRH "}},	/* Brahui */
  {"bs",	{"BOS "}},	/* Bosnian */
  {"btb",	{"BTI "}},	/* Beti (Cameroon) */
  {"ca",	{"CAT "}},	/* Catalan */
  {"ce",	{"CHE "}},	/* Chechen */
  {"ceb",	{"CEB "}},	/* Cebuano */
  {"chp",	{"CHP "}},	/* Chipewyan */
  {"chr",	{"CHR "}},	/* Cherokee */
  {"cop",	{"COP "}},	/* Coptic */
  {"cr",	{"CRE "}},	/* Cree */
  {"crh",	{"CRT "}},	/* Crimean Tatar */
  {"crm",	{"MCR "}},	/* Moose Cree */
  {"crx",	{"CRR "}},	/* Carrier */
  {"cs",	{"CSY "}},	/* Czech */
  {"cu",	{"CSL "}},	/* Church Slavic */
  {"cv",	{"CHU "}},	/* Chuvash */
  {"cwd",	{"DCR "}},	/* Woods Cree */
  {"cy",	{"WEL "}},	/* Welsh */
  {"da",	{"DAN "}},	/* Danish */
  {"dap",	{"NIS "}},	/* Nisi (India) */
  {"dar",	{"DAR "}},	/* Dargwa */
  {"de",	{"DEU "}},	/* German */
  {"din",	{"DNK "}},	/* Dinka */
  {"dng",	{"DUN "}},	/* Dungan */
  {"doi",	{"DGR "}},	/* Dogri */
  {"dsb",	{"LSB "}},	/* Lower Sorbian */
  {"dv",	{"DIV "}},	/* Dhivehi */
  {"dz",	{"DZN "}},	/* Dzongkha */
  {"ee",	{"EWE "}},	/* Ewe */
  {"efi",	{"EFI "}},	/* Efik */
  {"el",	{"ELL "}},	/* Modern Greek (1453-) */
  {"en",	{"ENG "}},	/* English */
  {"eo",	{"NTO "}},	/* Esperanto */
  {"eot",	{"BTI "}},	/* Beti (Côte d'Ivoire) */
  {"es",	{"ESP "}},	/* Spanish */
  {"et",	{"ETI "}},	/* Estonian */
  {"eu",	{"EUQ "}},	/* Basque */
  {"eve",	{"EVN "}},	/* Even */
  {"evn",	{"EVK "}},	/* Evenki */
  {"fa",	{"FAR "}},	/* Persian */
  {"ff",	{"FUL "}},	/* Fulah */
  {"fi",	{"FIN "}},	/* Finnish */
  {"fil",	{"PIL "}},	/* Filipino */
  {"fj",	{"FJI "}},	/* Fijian */
  {"fo",	{"FOS "}},	/* Faroese */
  {"fon",	{"FON "}},	/* Fon */
  {"fr",	{"FRA "}},	/* French */
  {"fur",	{"FRL "}},	/* Friulian */
  {"fy",	{"FRI "}},	/* Western Frisian */
  {"ga",	{"IRI "}},	/* Irish */
  {"gaa",	{"GAD "}},	/* Ga */
  {"gag",	{"GAG "}},	/* Gagauz */
  {"gbm",	{"GAW "}},	/* Garhwali */
  {"gd",	{"GAE "}},	/* Scottish Gaelic */
  {"gl",	{"GAL "}},	/* Galician */
  {"gld",	{"NAN "}},	/* Nanai */
  {"gn",	{"GUA "}},	/* Guarani */
  {"gon",	{"GON "}},	/* Gondi */
  {"grt",	{"GRO "}},	/* Garo */
  {"gu",	{"GUJ "}},	/* Gujarati */
  {"guk",	{"GMZ "}},	/* Gumuz */
  {"gv",	{"MNX "}},	/* Manx Gaelic */
  {"ha",	{"HAU "}},	/* Hausa */
  {"har",	{"HRI "}},	/* Harari */
  {"he",	{"IWR "}},	/* Hebrew */
  {"hi",	{"HIN "}},	/* Hindi */
  {"hil",	{"HIL "}},	/* Hiligaynon */
  {"hoc",	{"HO  "}},	/* Ho */
  {"hr",	{"HRV "}},	/* Croatian */
  {"hsb",	{"USB "}},	/* Upper Sorbian */
  {"ht",	{"HAI "}},	/* Haitian */
  {"hu",	{"HUN "}},	/* Hungarian */
  {"hy",	{"HYE "}},	/* Armenian */
  {"id",	{"IND "}},	/* Indonesian */
  {"ig",	{"IBO "}},	/* Igbo */
  {"igb",	{"EBI "}},	/* Ebira */
  {"inh",	{"ING "}},	/* Ingush */
  {"is",	{"ISL "}},	/* Icelandic */
  {"it",	{"ITA "}},	/* Italian */
  {"iu",	{"INU "}},	/* Inuktitut */
  {"ja",	{"JAN "}},	/* Japanese */
  {"jv",	{"JAV "}},	/* Javanese */
  {"ka",	{"KAT "}},	/* Georgian */
  {"kam",	{"KMB "}},	/* Kamba (Kenya) */
  {"kbd",	{"KAB "}},	/* Kabardian */
  {"kdr",	{"KRM "}},	/* Karaim */
  {"kdt",	{"KUY "}},	/* Kuy */
  {"kfr",	{"KAC "}},	/* Kachchi */
  {"kfy",	{"KMN "}},	/* Kumaoni */
  {"kha",	{"KSI "}},	/* Khasi */
  {"khw",	{"KHW "}},	/* Khowar */
  {"ki",	{"KIK "}},	/* Kikuyu */
  {"kk",	{"KAZ "}},	/* Kazakh */
  {"kl",	{"GRN "}},	/* Kalaallisut */
  {"kln",	{"KAL "}},	/* Kalenjin */
  {"km",	{"KHM "}},	/* Central Khmer */
  {"kmw",	{"KMO "}},	/* Komo (Democratic Republic of Congo) */
  {"kn",	{"KAN "}},	/* Kannada */
  {"ko",	{"KOR "}},	/* Korean */
  {"koi",	{"KOP "}},	/* Komi-Permyak */
  {"kok",	{"KOK "}},	/* Konkani */
  {"kpe",	{"KPL "}},	/* Kpelle */
  {"kpv",	{"KOZ "}},	/* Komi-Zyrian */
  {"kpy",	{"KYK "}},	/* Koryak */
  {"kqy",	{"KRT "}},	/* Koorete */
  {"kr",	{"KNR "}},	/* Kanuri */
  {"kri",	{"KRI "}},	/* Krio */
  {"krl",	{"KRL "}},	/* Karelian */
  {"kru",	{"KUU "}},	/* Kurukh */
  {"ks",	{"KSH "}},	/* Kashmiri */
  {"ku",	{"KUR "}},	/* Kurdish */
  {"kum",	{"KUM "}},	/* Kumyk */
  {"kvd",	{"KUI "}},	/* Kui (Indonesia) */
  {"kxu",	{"KUI "}},	/* Kui (India) */
  {"ky",	{"KIR "}},	/* Kirghiz */
  {"la",	{"LAT "}},	/* Latin */
  {"lad",	{"JUD "}},	/* Ladino */
  {"lb",	{"LTZ "}},	/* Luxembourgish */
  {"lbe",	{"LAK "}},	/* Lak */
  {"lbj",	{"LDK "}},	/* Ladakhi */
  {"lif",	{"LMB "}},	/* Limbu */
  {"lld",	{"LAD "}},	/* Ladin */
  {"ln",	{"LIN "}},	/* Lingala */
  {"lo",	{"LAO "}},	/* Lao */
  {"lt",	{"LTH "}},	/* Lithuanian */
  {"luo",	{"LUO "}},	/* Luo (Kenya and Tanzania) */
  {"luw",	{"LUO "}},	/* Luo (Cameroon) */
  {"lv",	{"LVI "}},	/* Latvian */
  {"lzz",	{"LAZ "}},	/* Laz */
  {"mai",	{"MTH "}},	/* Maithili */
  {"mdc",	{"MLE "}},	/* Male (Papua New Guinea) */
  {"mdf",	{"MOK "}},	/* Moksha */
  {"mdy",	{"MLE "}},	/* Male (Ethiopia) */
  {"men",	{"MDE "}},	/* Mende (Sierra Leone) */
  {"mg",	{"MLG "}},	/* Malagasy */
  {"mi",	{"MRI "}},	/* Maori */
  {"mk",	{"MKD "}},	/* Macedonian */
  {"ml",	{"MLR "}},	/* Malayalam */
  {"mn",	{"MNG "}},	/* Mongolian */
  {"mnc",	{"MCH "}},	/* Manchu */
  {"mni",	{"MNI "}},	/* Manipuri */
  {"mnk",	{"MND "}},	/* Mandinka */
  {"mns",	{"MAN "}},	/* Mansi */
  {"mnw",	{"MON "}},	/* Mon */
  {"mo",	{"MOL "}},	/* Moldavian */
  {"moh",	{"MOH "}},	/* Mohawk */
  {"mpe",	{"MAJ "}},	/* Majang */
  {"mr",	{"MAR "}},	/* Marathi */
  {"ms",	{"MLY "}},	/* Malay */
  {"mt",	{"MTS "}},	/* Maltese */
  {"mwr",	{"MAW "}},	/* Marwari */
  {"my",	{"BRM "}},	/* Burmese */
  {"mym",	{"MEN "}},	/* Me'en */
  {"myv",	{"ERZ "}},	/* Erzya */
  {"nb",	{"NOR "}},	/* Norwegian Bokmål */
  {"nco",	{"SIB "}},	/* Sibe */
  {"ne",	{"NEP "}},	/* Nepali */
  {"new",	{"NEW "}},	/* Newari */
  {"ng",	{"NDG "}},	/* Ndonga */
  {"ngl",	{"LMW "}},	/* Lomwe */
  {"niu",	{"NIU "}},	/* Niuean */
  {"niv",	{"GIL "}},	/* Gilyak */
  {"nl",	{"NLD "}},	/* Dutch */
  {"nn",	{"NYN "}},	/* Norwegian Nynorsk */
  {"no",	{"NOR "}},	/* Norwegian (deprecated) */
  {"nog",	{"NOG "}},	/* Nogai */
  {"nqo",	{"NKO "}},	/* N'Ko */
  {"nsk",	{"NAS "}},	/* Naskapi */
  {"ny",	{"CHI "}},	/* Nyanja */
  {"oc",	{"OCI "}},	/* Occitan (post 1500) */
  {"oj",	{"OJB "}},	/* Ojibwa */
  {"om",	{"ORO "}},	/* Oromo */
  {"or",	{"ORI "}},	/* Oriya */
  {"os",	{"OSS "}},	/* Ossetian */
  {"pa",	{"PAN "}},	/* Panjabi */
  {"pi",	{"PAL "}},	/* Pali */
  {"pl",	{"PLK "}},	/* Polish */
  {"plp",	{"PAP "}},	/* Palpa */
  {"prs",	{"DRI "}},	/* Dari */
  {"ps",	{"PAS "}},	/* Pushto */
  {"pt",	{"PTG "}},	/* Portuguese */
  {"raj",	{"RAJ "}},	/* Rajasthani */
  {"ria",	{"RIA "}},	/* Riang (India) */
  {"ril",	{"RIA "}},	/* Riang (Myanmar) */
  {"ro",	{"ROM "}},	/* Romanian */
  {"rom",	{"ROY "}},	/* Romany */
  {"ru",	{"RUS "}},	/* Russian */
  {"rue",	{"RSY "}},	/* Rusyn */
  {"sa",	{"SAN "}},	/* Sanskrit */
  {"sah",	{"YAK "}},	/* Yakut */
  {"sat",	{"SAT "}},	/* Santali */
  {"sck",	{"SAD "}},	/* Sadri */
  {"sd",	{"SND "}},	/* Sindhi */
  {"se",	{"NSM "}},	/* Northern Sami */
  {"seh",	{"SNA "}},	/* Sena */
  {"sel",	{"SEL "}},	/* Selkup */
  {"sg",	{"SGO "}},	/* Sango */
  {"shn",	{"SHN "}},	/* Shan */
  {"si",	{"SNH "}},	/* Sinhala */
  {"sid",	{"SID "}},	/* Sidamo */
  {"sjd",	{"KSM "}},	/* Kildin Sami */
  {"sk",	{"SKY "}},	/* Slovak */
  {"skr",	{"SRK "}},	/* Seraiki */
  {"sl",	{"SLV "}},	/* Slovenian */
  {"sm",	{"SMO "}},	/* Samoan */
  {"sma",	{"SSM "}},	/* Southern Sami */
  {"smj",	{"LSM "}},	/* Lule Sami */
  {"smn",	{"ISM "}},	/* Inari Sami */
  {"sms",	{"SKS "}},	/* Skolt Sami */
  {"snk",	{"SNK "}},	/* Soninke */
  {"so",	{"SML "}},	/* Somali */
  {"sq",	{"SQI "}},	/* Albanian */
  {"sr",	{"SRB "}},	/* Serbian */
  {"srr",	{"SRR "}},	/* Serer */
  {"suq",	{"SUR "}},	/* Suri */
  {"sv",	{"SVE "}},	/* Swedish */
  {"sva",	{"SVA "}},	/* Svan */
  {"sw",	{"SWK "}},	/* Swahili */
  {"swb",	{"CMR "}},	/* Comorian */
  {"syr",	{"SYR "}},	/* Syriac */
  {"ta",	{"TAM "}},	/* Tamil */
  {"tcy",	{"TUL "}},	/* Tulu */
  {"te",	{"TEL "}},	/* Telugu */
  {"tg",	{"TAJ "}},	/* Tajik */
  {"th",	{"THA "}},	/* Thai */
  {"ti",	{"TGY "}},	/* Tigrinya */
  {"tig",	{"TGR "}},	/* Tigre */
  {"tk",	{"TKM "}},	/* Turkmen */
  {"tn",	{"TNA "}},	/* Tswana */
  {"tnz",	{"TNG "}},	/* Tonga (Thailand) */
  {"to",	{"TNG "}},	/* Tonga (Tonga Islands) */
  {"tog",	{"TNG "}},	/* Tonga (Nyasa) */
  {"toi",	{"TNG "}},	/* Tonga (Zambia) */
  {"tr",	{"TRK "}},	/* Turkish */
  {"ts",	{"TSG "}},	/* Tsonga */
  {"tt",	{"TAT "}},	/* Tatar */
  {"tw",	{"TWI "}},	/* Twi */
  {"ty",	{"THT "}},	/* Tahitian */
  {"udm",	{"UDM "}},	/* Udmurt */
  {"ug",	{"UYG "}},	/* Uighur */
  {"uk",	{"UKR "}},	/* Ukrainian */
  {"unr",	{"MUN "}},	/* Mundari */
  {"ur",	{"URD "}},	/* Urdu */
  {"uz",	{"UZB "}},	/* Uzbek */
  {"ve",	{"VEN "}},	/* Venda */
  {"vi",	{"VIT "}},	/* Vietnamese */
  {"wbm",	{"WA  "}},	/* Wa */
  {"wbr",	{"WAG "}},	/* Wagdi */
  {"wo",	{"WLF "}},	/* Wolof */
  {"xal",	{"KLM "}},	/* Kalmyk */
  {"xh",	{"XHS "}},	/* Xhosa */
  {"xom",	{"KMO "}},	/* Komo (Sudan) */
  {"xsl",	{"SSL "}},	/* South Slavey */
  {"yi",	{"JII "}},	/* Yiddish */
  {"yo",	{"YBA "}},	/* Yoruba */
  {"yso",	{"NIS "}},	/* Nisi (China) */
  {"zh-cn",	{"ZHS "}},	/* Chinese (China) */
  {"zh-hk",	{"ZHH "}},	/* Chinese (Hong Kong) */
  {"zh-mo",	{"ZHT "}},	/* Chinese (Macao) */
  {"zh-sg",	{"ZHS "}},	/* Chinese (Singapore) */
  {"zh-tw",	{"ZHT "}},	/* Chinese (Taiwan) */
  {"zne",	{"ZND "}},	/* Zande */
  {"zu",	{"ZUL "}} 	/* Zulu */

  /* I couldn't find the language id for these */

/*{"??",	{"AGW "}},*/	/* Agaw */
/*{"??",	{"ALS "}},*/	/* Alsatian */
/*{"??",	{"ALT "}},*/	/* Altai */
/*{"??",	{"ARK "}},*/	/* Arakanese */
/*{"??",	{"ATH "}},*/	/* Athapaskan */
/*{"??",	{"BAG "}},*/	/* Baghelkhandi */
/*{"??",	{"BAL "}},*/	/* Balkar */
/*{"??",	{"BAU "}},*/	/* Baule */
/*{"??",	{"BBR "}},*/	/* Berber */
/*{"??",	{"BCR "}},*/	/* Bible Cree */
/*{"??",	{"BEL "}},*/	/* Belarussian */
/*{"??",	{"BIL "}},*/	/* Bilen */
/*{"??",	{"BKF "}},*/	/* Blackfoot */
/*{"??",	{"BLN "}},*/	/* Balante */
/*{"??",	{"BML "}},*/	/* Bamileke */
/*{"??",	{"BRI "}},*/	/* Braj Bhasha */
/*{"??",	{"CHG "}},*/	/* Chaha Gurage */
/*{"??",	{"CHH "}},*/	/* Chattisgarhi */
/*{"??",	{"CHK "}},*/	/* Chukchi */
/*{"??",	{"DJR "}},*/	/* Djerma */
/*{"??",	{"DNG "}},*/	/* Dangme */
/*{"??",	{"ECR "}},*/	/* Eastern Cree */
/*{"??",	{"FAN "}},*/	/* French Antillean */
/*{"??",	{"FLE "}},*/	/* Flemish */
/*{"??",	{"FNE "}},*/	/* Forest Nenets */
/*{"??",	{"FTA "}},*/	/* Futa */
/*{"??",	{"GAR "}},*/	/* Garshuni */
/*{"??",	{"GEZ "}},*/	/* Ge'ez */
/*{"??",	{"HAL "}},*/	/* Halam */
/*{"??",	{"HAR "}},*/	/* Harauti */
/*{"??",	{"HAW "}},*/	/* Hawaiin */
/*{"??",	{"HBN "}},*/	/* Hammer-Banna */
/*{"??",	{"HMA "}},*/	/* High Mari */
/*{"??",	{"HND "}},*/	/* Hindko */
/*{"??",	{"IJO "}},*/	/* Ijo */
/*{"??",	{"ILO "}},*/	/* Ilokano */
/*{"??",	{"IRT "}},*/	/* Irish Traditional */
/*{"??",	{"JUL "}},*/	/* Jula */
/*{"??",	{"KAR "}},*/	/* Karachay */
/*{"??",	{"KEB "}},*/	/* Kebena */
/*{"??",	{"KGE "}},*/	/* Khutsuri Georgian */
/*{"??",	{"KHA "}},*/	/* Khakass */
/*{"??",	{"KHK "}},*/	/* Khanty-Kazim */
/*{"??",	{"KHS "}},*/	/* Khanty-Shurishkar */
/*{"??",	{"KHV "}},*/	/* Khanty-Vakhi */
/*{"??",	{"KIS "}},*/	/* Kisii */
/*{"??",	{"KKN "}},*/	/* Kokni */
/*{"??",	{"KMS "}},*/	/* Komso */
/*{"??",	{"KOD "}},*/	/* Kodagu */
/*{"??",	{"KOH "}},*/	/* Korean Old Hangul */
/*{"??",	{"KON "}},*/	/* Kikongo */
/*{"??",	{"KRK "}},*/	/* Karakalpak */
/*{"??",	{"KRN "}},*/	/* Karen */
/*{"??",	{"KUL "}},*/	/* Kulvi */
/*{"??",	{"LAH "}},*/	/* Lahuli */
/*{"??",	{"LAM "}},*/	/* Lambani */
/*{"??",	{"LCR "}},*/	/* L-Cree */
/*{"??",	{"LEZ "}},*/	/* Lezgi */
/*{"??",	{"LMA "}},*/	/* Low Mari */
/*{"??",	{"LUB "}},*/	/* Luba */
/*{"??",	{"LUG "}},*/	/* Luganda */
/*{"??",	{"LUH "}},*/	/* Luhya */
/*{"??",	{"MAK "}},*/	/* Makua */
/*{"??",	{"MAL "}},*/	/* Malayalam Traditional */
/*{"??",	{"MBN "}},*/	/* Mbundu */
/*{"??",	{"MIZ "}},*/	/* Mizo */
/*{"??",	{"MLN "}},*/	/* Malinke */
/*{"??",	{"MNK "}},*/	/* Maninka */
/*{"??",	{"MOR "}},*/	/* Moroccan */
/*{"??",	{"NAG "}},*/	/* Naga-Assamese */
/*{"??",	{"NCR "}},*/	/* N-Cree */
/*{"??",	{"NDB "}},*/	/* Ndebele */
/*{"??",	{"NGR "}},*/	/* Nagari */
/*{"??",	{"NHC "}},*/	/* Norway House Cree */
/*{"??",	{"NKL "}},*/	/* Nkole */
/*{"??",	{"NTA "}},*/	/* Northern Tai */
/*{"??",	{"OCR "}},*/	/* Oji-Cree */
/*{"??",	{"PAA "}},*/	/* Palestinian Aramaic */
/*{"??",	{"PGR "}},*/	/* Polytonic Greek */
/*{"??",	{"PLG "}},*/	/* Palaung */
/*{"??",	{"QIN "}},*/	/* Chin */
/*{"??",	{"RBU "}},*/	/* Russian Buriat */
/*{"??",	{"RCR "}},*/	/* R-Cree */
/*{"??",	{"RMS "}},*/	/* Rhaeto-Romanic */
/*{"??",	{"RUA "}},*/	/* Ruanda */
/*{"??",	{"SAY "}},*/	/* Sayisi */
/*{"??",	{"SEK "}},*/	/* Sekota */
/*{"??",	{"SIG "}},*/	/* Silte Gurage */
/*{"??",	{"SLA "}},*/	/* Slavey */
/*{"??",	{"SOG "}},*/	/* Sodo Gurage */
/*{"??",	{"SOT "}},*/	/* Sotho */
/*{"??",	{"SWA "}},*/	/* Swadaya Aramaic */
/*{"??",	{"SWZ "}},*/	/* Swazi */
/*{"??",	{"SXT "}},*/	/* Sutu */
/*{"??",	{"TAB "}},*/	/* Tabasaran */
/*{"??",	{"TCR "}},*/	/* TH-Cree */
/*{"??",	{"TGN "}},*/	/* Tongan */
/*{"??",	{"TMN "}},*/	/* Temne */
/*{"??",	{"TNE "}},*/	/* Tundra Nenets */
/*{"??",	{"TOD "}},*/	/* Todo */
/*{"??",	{"TUA "}},*/	/* Turoyo Aramaic */
/*{"??",	{"TUV "}},*/	/* Tuvin */
/*{"??",	{"WCR "}},*/	/* West-Cree */
/*{"??",	{"XBD "}},*/	/* Tai Lue */
/*{"??",	{"YCR "}},*/	/* Y-Cree */
/*{"??",	{"YIC "}},*/	/* Yi Classic */
/*{"??",	{"YIM "}},*/	/* Yi Modern */
/*{"??",	{"ZHP "}},*/	/* Chinese Phonetic */
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
