#include <stdio.h>
#include <glib.h>

/* Table copied from pango-script.h */
enum {
      PANGO_SCRIPT_COMMON       = 0,   /* Zyyy */
      PANGO_SCRIPT_INHERITED,          /* Qaai */
      PANGO_SCRIPT_ARABIC,             /* Arab */
      PANGO_SCRIPT_ARMENIAN,           /* Armn */
      PANGO_SCRIPT_BENGALI,            /* Beng */
      PANGO_SCRIPT_BOPOMOFO,           /* Bopo */
      PANGO_SCRIPT_CHEROKEE,           /* Cher */
      PANGO_SCRIPT_COPTIC,             /* Qaac */
      PANGO_SCRIPT_CYRILLIC,           /* Cyrl (Cyrs) */
      PANGO_SCRIPT_DESERET,            /* Dsrt */
      PANGO_SCRIPT_DEVANAGARI,         /* Deva */
      PANGO_SCRIPT_ETHIOPIC,           /* Ethi */
      PANGO_SCRIPT_GEORGIAN,           /* Geor (Geon, Geoa) */
      PANGO_SCRIPT_GOTHIC,             /* Goth */
      PANGO_SCRIPT_GREEK,              /* Grek */
      PANGO_SCRIPT_GUJARATI,           /* Gujr */
      PANGO_SCRIPT_GURMUKHI,           /* Guru */
      PANGO_SCRIPT_HAN,                /* Hani */
      PANGO_SCRIPT_HANGUL,             /* Hang */
      PANGO_SCRIPT_HEBREW,             /* Hebr */
      PANGO_SCRIPT_HIRAGANA,           /* Hira */
      PANGO_SCRIPT_KANNADA,            /* Knda */
      PANGO_SCRIPT_KATAKANA,           /* Kana */
      PANGO_SCRIPT_KHMER,              /* Khmr */
      PANGO_SCRIPT_LAO,                /* Laoo */
      PANGO_SCRIPT_LATIN,              /* Latn (Latf, Latg) */
      PANGO_SCRIPT_MALAYALAM,          /* Mlym */
      PANGO_SCRIPT_MONGOLIAN,          /* Mong */
      PANGO_SCRIPT_MYANMAR,            /* Mymr */
      PANGO_SCRIPT_OGHAM,              /* Ogam */
      PANGO_SCRIPT_OLD_ITALIC,         /* Ital */
      PANGO_SCRIPT_ORIYA,              /* Orya */
      PANGO_SCRIPT_RUNIC,              /* Runr */
      PANGO_SCRIPT_SINHALA,            /* Sinh */
      PANGO_SCRIPT_SYRIAC,             /* Syrc (Syrj, Syrn, Syre) */
      PANGO_SCRIPT_TAMIL,              /* Taml */
      PANGO_SCRIPT_TELUGU,             /* Telu */
      PANGO_SCRIPT_THAANA,             /* Thaa */
      PANGO_SCRIPT_THAI,               /* Thai */
      PANGO_SCRIPT_TIBETAN,            /* Tibt */
      PANGO_SCRIPT_CANADIAN_ABORIGINAL, /* Cans */
      PANGO_SCRIPT_YI,                 /* Yiii */
      PANGO_SCRIPT_TAGALOG,            /* Tglg */
      PANGO_SCRIPT_HANUNOO,            /* Hano */
      PANGO_SCRIPT_BUHID,              /* Buhd */
      PANGO_SCRIPT_TAGBANWA,           /* Tagb */

      /* Unicode-4.0 additions */
      PANGO_SCRIPT_BRAILLE,            /* Brai */
      PANGO_SCRIPT_CYPRIOT,            /* Cprt */
      PANGO_SCRIPT_LIMBU,              /* Limb */
      PANGO_SCRIPT_OSMANYA,            /* Osma */
      PANGO_SCRIPT_SHAVIAN,            /* Shaw */
      PANGO_SCRIPT_LINEAR_B,           /* Linb */
      PANGO_SCRIPT_TAI_LE,             /* Tale */
      PANGO_SCRIPT_UGARITIC,           /* Ugar */
      
      /* Unicode-4.1 additions */
      PANGO_SCRIPT_NEW_TAI_LUE,        /* Talu */
      PANGO_SCRIPT_BUGINESE,           /* Bugi */
      PANGO_SCRIPT_GLAGOLITIC,         /* Glag */
      PANGO_SCRIPT_TIFINAGH,           /* Tfng */
      PANGO_SCRIPT_SYLOTI_NAGRI,       /* Sylo */
      PANGO_SCRIPT_OLD_PERSIAN,        /* Xpeo */
      PANGO_SCRIPT_KHAROSHTHI          /* Khar */
};

#define SCRIPT(x) #x

#define PANGO_SCRIPT_LATIN "PANGO_SCRIPT_LATIN"
#define PANGO_SCRIPT_INHERITED "PANGO_SCRIPT_INHERITED"

/* This table used to be part of pango-script-table.h */
static const struct {
    gunichar    start;
    guint16     chars;
    const char *script;
} script_table[] = { 
 { 0x0041,    26, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x0061,    26, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x00aa,     1, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x00ba,     1, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x00c0,    23, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x00d8,    31, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x00f8,   330, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x0250,   105, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x02e0,     5, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x0300,   112, SCRIPT (PANGO_SCRIPT_INHERITED) },
 { 0x0374,     2, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x037a,     1, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x0384,     3, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x0388,     3, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x038c,     1, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x038e,    20, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x03a3,    44, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x03d0,    18, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x03e2,    14, SCRIPT (PANGO_SCRIPT_COPTIC) },
 { 0x03f0,    16, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x0400,   135, SCRIPT (PANGO_SCRIPT_CYRILLIC) },
 { 0x0488,    71, SCRIPT (PANGO_SCRIPT_CYRILLIC) },
 { 0x04d0,    42, SCRIPT (PANGO_SCRIPT_CYRILLIC) },
 { 0x0500,    16, SCRIPT (PANGO_SCRIPT_CYRILLIC) },
 { 0x0531,    38, SCRIPT (PANGO_SCRIPT_ARMENIAN) },
 { 0x0559,     7, SCRIPT (PANGO_SCRIPT_ARMENIAN) },
 { 0x0561,    39, SCRIPT (PANGO_SCRIPT_ARMENIAN) },
 { 0x058a,     1, SCRIPT (PANGO_SCRIPT_ARMENIAN) },
 { 0x0591,    41, SCRIPT (PANGO_SCRIPT_HEBREW) },
 { 0x05bb,    13, SCRIPT (PANGO_SCRIPT_HEBREW) },
 { 0x05d0,    27, SCRIPT (PANGO_SCRIPT_HEBREW) },
 { 0x05f0,     5, SCRIPT (PANGO_SCRIPT_HEBREW) },
 { 0x060b,     1, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x060d,     9, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x061e,     1, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x0621,    26, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x0641,    10, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x064b,    11, SCRIPT (PANGO_SCRIPT_INHERITED) },
 { 0x0656,     9, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x066a,     6, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x0670,     1, SCRIPT (PANGO_SCRIPT_INHERITED) },
 { 0x0671,   108, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x06de,    34, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x0700,    14, SCRIPT (PANGO_SCRIPT_SYRIAC) },
 { 0x070f,    60, SCRIPT (PANGO_SCRIPT_SYRIAC) },
 { 0x074d,     3, SCRIPT (PANGO_SCRIPT_SYRIAC) },
 { 0x0750,    30, SCRIPT (PANGO_SCRIPT_ARABIC) },
 { 0x0780,    50, SCRIPT (PANGO_SCRIPT_THAANA) },
 { 0x0901,    57, SCRIPT (PANGO_SCRIPT_DEVANAGARI) },
 { 0x093c,    18, SCRIPT (PANGO_SCRIPT_DEVANAGARI) },
 { 0x0950,     5, SCRIPT (PANGO_SCRIPT_DEVANAGARI) },
 { 0x0958,    12, SCRIPT (PANGO_SCRIPT_DEVANAGARI) },
 { 0x0966,    10, SCRIPT (PANGO_SCRIPT_DEVANAGARI) },
 { 0x097d,     1, SCRIPT (PANGO_SCRIPT_DEVANAGARI) },
 { 0x0981,     3, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x0985,     8, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x098f,     2, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x0993,    22, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09aa,     7, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09b2,     1, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09b6,     4, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09bc,     9, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09c7,     2, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09cb,     4, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09d7,     1, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09dc,     2, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09df,     5, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x09e6,    21, SCRIPT (PANGO_SCRIPT_BENGALI) },
 { 0x0a01,     3, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a05,     6, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a0f,     2, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a13,    22, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a2a,     7, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a32,     2, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a35,     2, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a38,     2, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a3c,     1, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a3e,     5, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a47,     2, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a4b,     3, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a59,     4, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a5e,     1, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a66,    15, SCRIPT (PANGO_SCRIPT_GURMUKHI) },
 { 0x0a81,     3, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0a85,     9, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0a8f,     3, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0a93,    22, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0aaa,     7, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0ab2,     2, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0ab5,     5, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0abc,    10, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0ac7,     3, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0acb,     3, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0ad0,     1, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0ae0,     4, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0ae6,    10, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0af1,     1, SCRIPT (PANGO_SCRIPT_GUJARATI) },
 { 0x0b01,     3, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b05,     8, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b0f,     2, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b13,    22, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b2a,     7, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b32,     2, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b35,     5, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b3c,     8, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b47,     2, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b4b,     3, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b56,     2, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b5c,     2, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b5f,     3, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b66,    12, SCRIPT (PANGO_SCRIPT_ORIYA) },
 { 0x0b82,     2, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0b85,     6, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0b8e,     3, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0b92,     4, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0b99,     2, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0b9c,     1, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0b9e,     2, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0ba3,     2, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0ba8,     3, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0bae,    12, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0bbe,     5, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0bc6,     3, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0bca,     4, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0bd7,     1, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0be6,    21, SCRIPT (PANGO_SCRIPT_TAMIL) },
 { 0x0c01,     3, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c05,     8, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c0e,     3, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c12,    23, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c2a,    10, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c35,     5, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c3e,     7, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c46,     3, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c4a,     4, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c55,     2, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c60,     2, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c66,    10, SCRIPT (PANGO_SCRIPT_TELUGU) },
 { 0x0c82,     2, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0c85,     8, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0c8e,     3, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0c92,    23, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0caa,    10, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0cb5,     5, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0cbc,     9, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0cc6,     3, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0cca,     4, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0cd5,     2, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0cde,     1, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0ce0,     2, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0ce6,    10, SCRIPT (PANGO_SCRIPT_KANNADA) },
 { 0x0d02,     2, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d05,     8, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d0e,     3, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d12,    23, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d2a,    16, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d3e,     6, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d46,     3, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d4a,     4, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d57,     1, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d60,     2, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d66,    10, SCRIPT (PANGO_SCRIPT_MALAYALAM) },
 { 0x0d82,     2, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0d85,    18, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0d9a,    24, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0db3,     9, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0dbd,     1, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0dc0,     7, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0dca,     1, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0dcf,     6, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0dd6,     1, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0dd8,     8, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0df2,     3, SCRIPT (PANGO_SCRIPT_SINHALA) },
 { 0x0e01,    58, SCRIPT (PANGO_SCRIPT_THAI) },
 { 0x0e40,    28, SCRIPT (PANGO_SCRIPT_THAI) },
 { 0x0e81,     2, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0e84,     1, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0e87,     2, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0e8a,     1, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0e8d,     1, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0e94,     4, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0e99,     7, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ea1,     3, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ea5,     1, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ea7,     1, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0eaa,     2, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ead,    13, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ebb,     3, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ec0,     5, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ec6,     1, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ec8,     6, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0ed0,    10, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0edc,     2, SCRIPT (PANGO_SCRIPT_LAO) },
 { 0x0f00,    72, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x0f49,    34, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x0f71,    27, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x0f90,     8, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x0f99,    36, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x0fbe,    15, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x0fcf,     3, SCRIPT (PANGO_SCRIPT_TIBETAN) },
 { 0x1000,    34, SCRIPT (PANGO_SCRIPT_MYANMAR) },
 { 0x1023,     5, SCRIPT (PANGO_SCRIPT_MYANMAR) },
 { 0x1029,     2, SCRIPT (PANGO_SCRIPT_MYANMAR) },
 { 0x102c,     7, SCRIPT (PANGO_SCRIPT_MYANMAR) },
 { 0x1036,     4, SCRIPT (PANGO_SCRIPT_MYANMAR) },
 { 0x1040,    26, SCRIPT (PANGO_SCRIPT_MYANMAR) },
 { 0x10a0,    38, SCRIPT (PANGO_SCRIPT_GEORGIAN) },
 { 0x10d0,    43, SCRIPT (PANGO_SCRIPT_GEORGIAN) },
 { 0x10fc,     1, SCRIPT (PANGO_SCRIPT_GEORGIAN) },
 { 0x1100,    90, SCRIPT (PANGO_SCRIPT_HANGUL) },
 { 0x115f,    68, SCRIPT (PANGO_SCRIPT_HANGUL) },
 { 0x11a8,    82, SCRIPT (PANGO_SCRIPT_HANGUL) },
 { 0x1200,    73, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x124a,     4, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1250,     7, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1258,     1, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x125a,     4, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1260,    41, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x128a,     4, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1290,    33, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x12b2,     4, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x12b8,     7, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x12c0,     1, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x12c2,     4, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x12c8,    15, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x12d8,    57, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1312,     4, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1318,    67, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x135f,    30, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x1380,    26, SCRIPT (PANGO_SCRIPT_ETHIOPIC) },
 { 0x13a0,    85, SCRIPT (PANGO_SCRIPT_CHEROKEE) },
 { 0x1401,   630, SCRIPT (PANGO_SCRIPT_CANADIAN_ABORIGINAL) },
 { 0x1680,    29, SCRIPT (PANGO_SCRIPT_OGHAM) },
 { 0x16a0,    75, SCRIPT (PANGO_SCRIPT_RUNIC) },
 { 0x16ee,     3, SCRIPT (PANGO_SCRIPT_RUNIC) },
 { 0x1700,    13, SCRIPT (PANGO_SCRIPT_TAGALOG) },
 { 0x170e,     7, SCRIPT (PANGO_SCRIPT_TAGALOG) },
 { 0x1720,    21, SCRIPT (PANGO_SCRIPT_HANUNOO) },
 { 0x1740,    20, SCRIPT (PANGO_SCRIPT_BUHID) },
 { 0x1760,    13, SCRIPT (PANGO_SCRIPT_TAGBANWA) },
 { 0x176e,     3, SCRIPT (PANGO_SCRIPT_TAGBANWA) },
 { 0x1772,     2, SCRIPT (PANGO_SCRIPT_TAGBANWA) },
 { 0x1780,    94, SCRIPT (PANGO_SCRIPT_KHMER) },
 { 0x17e0,    10, SCRIPT (PANGO_SCRIPT_KHMER) },
 { 0x17f0,    10, SCRIPT (PANGO_SCRIPT_KHMER) },
 { 0x1800,    15, SCRIPT (PANGO_SCRIPT_MONGOLIAN) },
 { 0x1810,    10, SCRIPT (PANGO_SCRIPT_MONGOLIAN) },
 { 0x1820,    88, SCRIPT (PANGO_SCRIPT_MONGOLIAN) },
 { 0x1880,    42, SCRIPT (PANGO_SCRIPT_MONGOLIAN) },
 { 0x1900,    29, SCRIPT (PANGO_SCRIPT_LIMBU) },
 { 0x1920,    12, SCRIPT (PANGO_SCRIPT_LIMBU) },
 { 0x1930,    12, SCRIPT (PANGO_SCRIPT_LIMBU) },
 { 0x1940,     1, SCRIPT (PANGO_SCRIPT_LIMBU) },
 { 0x1944,    12, SCRIPT (PANGO_SCRIPT_LIMBU) },
 { 0x1950,    30, SCRIPT (PANGO_SCRIPT_TAI_LE) },
 { 0x1970,     5, SCRIPT (PANGO_SCRIPT_TAI_LE) },
 { 0x1980,    42, SCRIPT (PANGO_SCRIPT_NEW_TAI_LUE) },
 { 0x19b0,    26, SCRIPT (PANGO_SCRIPT_NEW_TAI_LUE) },
 { 0x19d0,    10, SCRIPT (PANGO_SCRIPT_NEW_TAI_LUE) },
 { 0x19de,     2, SCRIPT (PANGO_SCRIPT_NEW_TAI_LUE) },
 { 0x19e0,    32, SCRIPT (PANGO_SCRIPT_KHMER) },
 { 0x1a00,    28, SCRIPT (PANGO_SCRIPT_BUGINESE) },
 { 0x1a1e,     2, SCRIPT (PANGO_SCRIPT_BUGINESE) },
 { 0x1d00,    38, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1d26,     5, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1d2b,     1, SCRIPT (PANGO_SCRIPT_CYRILLIC) },
 { 0x1d2c,    49, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1d5d,     5, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1d62,     4, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1d66,     5, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1d6b,    13, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1d78,     1, SCRIPT (PANGO_SCRIPT_CYRILLIC) },
 { 0x1d79,    71, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1dc0,     4, SCRIPT (PANGO_SCRIPT_INHERITED) },
 { 0x1e00,   156, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1ea0,    90, SCRIPT (PANGO_SCRIPT_LATIN) },
 { 0x1f00,    22, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f18,     6, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f20,    38, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f48,     6, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f50,     8, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f59,     1, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f5b,     1, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f5d,     1, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f5f,    31, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1f80,    53, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1fb6,    15, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1fc6,    14, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1fd6,     6, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1fdd,    19, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1ff2,     3, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1ff6,     9, SCRIPT (PANGO_SCRIPT_GREEK) },
 { 0x1fff,     1, SCRIPT (PANGO_SCRIPT_COMMON) }, /* special case at the end */
};

#define EASY_SCRIPT_RANGE 8192

static const gunichar paired_chars[] = {
  0x0028, 0x0029, /* ascii paired punctuation */
  0x003c, 0x003e,
  0x005b, 0x005d,
  0x007b, 0x007d,
  0x00ab, 0x00bb, /* guillemets */
  0x2018, 0x2019, /* general punctuation */
  0x201c, 0x201d,
  0x2039, 0x203a,
  0x3008, 0x3009, /* chinese paired punctuation */
  0x300a, 0x300b,
  0x300c, 0x300d,
  0x300e, 0x300f,
  0x3010, 0x3011,
  0x3014, 0x3015,
  0x3016, 0x3017,
  0x3018, 0x3019,
  0x301a, 0x301b
};

static gboolean
is_pair_char (int ch)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (paired_chars); i++)
    if (paired_chars[i] == ch)
      return TRUE;

  return FALSE;
}

static void
print_char (const char *script, gboolean paired)
{
  printf ("  %s%s,\n", script, paired ? " | PANGO_PAIRED_CHAR_FLAG" : "");
}

int
main (int argc, char **argv)
{
  int i;
  int table_index;

  table_index = 0;

  puts ("/* Table generated by gen-easy-scripts-table - DO NOT EDIT\n"
	" *\n"
	" * This table is for the first bunch of Unicode points; it tells us\n"
	" * the script for each character ('a' -> latin).  The upper bit of each\n"
	" * entry tells us whether the character is a paired character (parentheses,\n"
	" * guillemots, etc.); if so, that character also appears in the pared_chars[]\n"
	" * table in pango-script.c\n"
	" */\n");

  printf ("#define PANGO_EASY_SCRIPTS_RANGE %d\n\n", EASY_SCRIPT_RANGE);
  puts ("#define PANGO_PAIRED_CHAR_FLAG (1 << 7)\n");
  puts ("#define PANGO_EASY_SCRIPTS_MASK ~(1 << 7)\n");
  puts ("static const guchar pango_easy_scripts_table[] = {");

  for (i = 0; i < EASY_SCRIPT_RANGE; i++)
    {
    retry_char:
      if (i < script_table[table_index].start)
	print_char ("PANGO_SCRIPT_COMMON", is_pair_char (i));
      else if (i < script_table[table_index].start + script_table[table_index].chars)
	print_char (script_table[table_index].script, is_pair_char (i));
      else
	{
	  table_index++;
	  g_assert (table_index < G_N_ELEMENTS (script_table));
	  goto retry_char;
	}
    }

  puts ("};");
  fflush (stdout);

  return 0;
}
