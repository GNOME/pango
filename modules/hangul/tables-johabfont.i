/* -*-c-*- */

#define JOHAB_LBASE 0x0001
#define JOHAB_VBASE 0x0137
#define JOHAB_TBASE 0x0195

#define JOHAB_FILLER 0x0000

const static guint16 __choseong_johabfont_base[] =
{
  /* the basic 19 CHOSEONGs */
  0x0001, 0x000b, 0x0015, 0x001f, 0x0029,
  0x0033, 0x003d, 0x0047, 0x0051, 0x005b,
  0x0065, 0x006f, 0x0079, 0x0083, 0x008d,
  0x0097, 0x00a1, 0x00ab, 0x00b5,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0x00c9,	/* PIEUP-KIYEOK */
  0,
  0x00dd,	/* PIEUP-TIKEUT */
  0x00fb,	/* PIEUP-SIOS */
  0, 0, 0, 0, 0,
  0x0119,	/* PIEUP-CIEUC */
  0, 0, 0,
  0x00bf, /* KAPYEOUNPIEUP */
  0,
  0x00d3,	/* SIOS-KIYEOK */
  0,
  0x00e7,	/* SIOS-TIKEUT */
  0, 0,
  0x00f1,	/* SIOS-PIEUP */
  0, 0, 0,
  0x0123,	/* SIOS-CIEUC */
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0x0105,	/* PANSIOS */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,
  0x012d,	/* YESIEUNG */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0,
  0x010f,	/* YEORINHIEUH */
  0, 0, 0, 0, 0, /* reserved */
  0	/* CHOSEONG filler */
};

const static guint16 __jungseong_johabfont_base[] = 
{
  /* the basic 21 JUNGSEONGs */
  0x0137, 0x013a, 0x013d, 0x0140, 0x0143,
  0x0146, 0x0149, 0x014c, 0x014f, 0x0153,
  0x0157, 0x015b, 0x015f, 0x0163, 0x0166,
  0x0169, 0x016c, 0x016f, 0x0172, 0x0176,
  0x017a,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0,
  0x017d, /* YO-YA */
  0x0180, /* YO-YAE */
  0, 0,
  0x0183, /* YO-I */
  0, 0, 0, 0, 0, 0, 0, 0,
  0x0186, /* YU-YEO */
  0x0189, /* YU-YE */
  0, 
  0x018c, /* YU-I */
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0x018f, /* ARAEA */
  0, 0,
  0x0192, /* ARAEA-I */
  0,
  0, 0, 0, 0 /* reserved */
};

const static gint16 __jongseong_johabfont_base[] = 
{
  0,	/* (INTERNAL) JONGSEONG filler */
  /* The basic 27 (28 - jongseong filter) CHONGSEONGs  */
  0x0195, 0x0199, 0x019d, 0x01a1, 0x01a5,
  0x01a9, 0x01ad, 0x01b1, 0x01b5, 0x01b9,
  0x01bd, 0x01c1, 0x01c5, 0x01c9, 0x01cd,
  0x01d1, 0x01d5, 0x01d9, 0x01dd, 0x01e1,
  0x01e5, 0x01e9, 0x01ed, 0x01f1, 0x01f5,
  0x01f9, 0x01fd,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0,
  0x0201,	/* RIEUL-YEORINHIEUH */
  0x0205,	/* MIEUM-KIYEOK */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,
  0x020d,	/* YESIEUNG */
  0, 0, 0, 0, 0, 0, 0, 0,
  0x0209,	/* YEORINHIEUH */
  0, 0, 0, 0, 0, 0 /* reserved */
};

/*
 * This table represents how many glyphs are available for each
 * JUNGSEONGs in johab-font -- some are 3 and some are 4.
 */
const static gint __johabfont_jungseong_kind[] =
{
  /* The basic 21 JUNGSEONGs.  */
  3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
  4, 4, 4, 3, 3, 3, 3, 3, 4, 4,
  3,
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0,
  3,	/* YO-YA */
  3,	/* YO-YAE */
  0, 0,
  3,	/* YO-I */
  0, 0, 0, 0, 0, 0, 0, 0,
  3,	/* YU-YEO */
  3,	/* YU-YE */
  0,
  3,	/* YU-I */
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  3, /* ARAEA */
  0, 0,
  3, /* ARAEA-I */
  0,
  0, 0, 0, 0 /* reserved */
};

const static gint __choseong_map_1[] = {
  /* The basic 21 JUNGSEONGs.  */
  0, 0, 0, 0, 0, 0, 0, 0, 1, 3,
  3, 3, 1, 2, 4, 4, 4, 2, 1, 3,
  0,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,
  3,	/* YO-YA */
  3,	/* YO-YAE */
  -1, -1,
  3,	/* YO-I */
  -1, -1, -1, -1, -1, -1, -1, -1,
  4,	/* YU-YEO */
  4,	/* YU-YE */
  -1,
  4,	/* YU-I */
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  2, /* ARAEA */
  -1, -1,
  4, /* ARAEA-I */
  -1,
  -1, -1, -1, -1 /* reserved */
};

const static gint __choseong_map_2[] = {
  /* The basic 21 JUNGSEONGs.  */
  5, 5, 5, 5, 5, 5, 5, 5, 6, 8,
  8, 8, 6, 7, 9, 9, 9, 7, 6, 8,
  5,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,
  8,	/* YO-YA */
  8,	/* YO-YAE */
  -1, -1,
  8,	/* YO-I */
  -1, -1, -1, -1, -1, -1, -1, -1,
  9,	/* YU-YEO */
  9,	/* YU-YE */
  -1,
  9,	/* YU-I */
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  6, /* ARAEA */
  -1, -1,
  9, /* ARAEA-I */
  -1,
  -1, -1, -1, -1 /* reserved */
};

const static gint __johabfont_jongseong_kind[] =
{
  0,	/* (internal) CHONGSEONG filler */
  /* The basic 27 (28 - jongseong filter) CHONGSEONGs  */
  1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1,
  1,       /* YEORINHIEUH */ 
  1,       /* MIEUM-KIYEOK */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1,
  1,	/* YESIEUNG */
  -1, -1, -1, -1, -1, -1, -1, -1,
  1,	/* YEORINHIEUH */
  -1, -1, -1, -1, -1, -1 /* reserved */
};

const static gint __jongseong_map[] = 
{
  /* The basic 21 JUNGSEONGs.  */
  0, 2, 0, 2, 1, 2, 1, 2, 3, 0,
  0, 0, 3, 3, 1, 1, 1, 3, 3, 0,
  1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,
  0,	/* YO-YA */
  0,	/* YO-YAE */
  -1, -1,
  1,	/* YO-I */
  -1, -1, -1, -1, -1, -1, -1, -1,
  1,	/* YU-YEO */
  1,	/* YU-YE */
  -1,
  1,	/* YU-I */
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  3, /* ARAEA */
  -1, -1,
  0, /* ARAEA-I */
  -1,
  -1, -1, -1, -1 /* reserved */
};


/*
 * Jamos
 */

/* from the CHOSEONG section -- mordern */
#define JOHAB_KIYEOK		0x0002
#define JOHAB_SSANGKIYEOK	0x000c
#define JOHAB_NIEUN		0x0016
#define JOHAB_TIKEUT		0x0020
#define JOHAB_SSANGTIKEUT	0x002a
#define JOHAB_RIEUL		0x0034
#define JOHAB_MIEUM		0x003e
#define JOHAB_PIEUP		0x0048
#define JOHAB_SSANGPIEUP	0x0052
#define JOHAB_SIOS		0x005c
#define JOHAB_SSANGSIOS		0x0066
#define JOHAB_IEUNG		0x0070
#define JOHAB_CIEUC		0x007a
#define JOHAB_SSANGCIEUC	0x0084
#define JOHAB_CHIEUCH		0x008e
#define JOHAB_KHIEUKH		0x0098
#define JOHAB_THIEUTH		0x00a2
#define JOHAB_PHIEUPH		0x00ac
#define JOHAB_HIEUH		0x00b6

/* from the CHOSEONG section -- extra */
#define JOHAB_KAPYEOUNPIEUP	0x00c0
#define JOHAB_PIEUP_KIYEOK	0x00ca
#define JOHAB_SIOS_KIYEOK	0x00d4
#define JOHAB_PIEUP_TIKEUT	0x00de
#define JOHAB_SIOS_TIKEUT	0x00e8
#define JOHAB_SIOS_PIEUP	0x00f2
#define JOHAB_PIEUP_SIOS	0x00fc
#define JOHAB_PANSIOS		0x0106
#define JOHAB_YESIEUNG		0x0110
#define JOHAB_PIEUP_CIEUC	0x011a
#define JOHAB_SIOS_CIEUC	0x0124
#define JOHAB_YEORINHIEUH	0x012e

/* from the JONGSEONG section -- modern */
#define JOHAB_KIYEOK_SIOS	0x019d
#define JOHAB_NIEUN_CIEUC	0x01a5
#define JOHAB_NIEUN_HIEUH	0x01a9
#define JOHAB_RIEUL_KIYEOK	0x01b5
#define JOHAB_RIEUL_MIEUM	0x01b9
#define JOHAB_RIEUL_PIEUP	0x01bd
#define JOHAB_RIEUL_SIOS	0x01c1
#define JOHAB_RIEUL_THIEUTH	0x01c5
#define JOHAB_RIEUL_PHIEUPH	0x01c9
#define JOHAB_RIEUL_HIEUH	0x01cd

/* from the JONGSEONG section -- extra */
#define JOHAB_RIEUL_YEORINHIEUH	0x0201
#define JOHAB_MIEUM_KIYEOK	0x0205

/* from the JUNGSEONG section -- modern */
#define JOHAB_A			0x0137
#define JOHAB_AE		0x013a
#define JOHAB_YA		0x013d
#define JOHAB_YAE		0x014d
#define JOHAB_EO		0x0143
#define JOHAB_E			0x0146
#define JOHAB_YEO		0x0149
#define JOHAB_YE		0x014c
#define JOHAB_O			0x014e
#define JOHAB_WA		0x0153
#define JOHAB_WAE		0x0157
#define JOHAB_OE		0x015b
#define JOHAB_YO		0x015e
#define JOHAB_U			0x0163
#define JOHAB_WEO		0x0166
#define JOHAB_WE		0x0169
#define JOHAB_WI		0x016c
#define JOHAB_YU		0x016f
#define JOHAB_EU		0x0172
#define JOHAB_YI		0x0176
#define JOHAB_I			0x017a

/* from the JUNGSEONG section -- extra */
#define JOHAB_YO_YA		0x017d
#define JOHAB_YO_YAE		0x0180
#define JOHAB_YO_I		0x0183
#define JOHAB_YU_YEO		0x0186
#define JOHAB_YU_YE		0x0189
#define JOHAB_YU_I		0x018c
#define JOHAB_ARAEA		0x018f
#define JOHAB_ARAEA_I		0x0192

/*
 * Some jamos are not representable with KSC5601.
 * Choose similar modern jamos, but different with glyphs.
 */
#define JOHAB_CHITUEUMSIOS	0x005e
#define JOHAB_CEONGCHITUEUMSIOS	0x0060
#define JOHAB_CHITUEUMCIEUC	0x007c
#define JOHAB_CEONGEUMCIEUC	0x007e
#define JOHAB_CHITUEUMCHIEUCH	0x0090
#define JOHAB_CEONGEUMCHIEUCH	0x0092

static guint16 __jamo_to_johabfont[0x100][3] =
{
  /*
   * CHOSEONG
   */
  /* CHOSEONG 0x1100 -- 0x1112 : matched to each ksc5601 Jamos extactly.  */
  {JOHAB_KIYEOK, 0, 0},
  {JOHAB_SSANGKIYEOK, 0, 0},
  {JOHAB_NIEUN, 0, 0},
  {JOHAB_TIKEUT, 0, 0},
  {JOHAB_SSANGTIKEUT, 0, 0},
  {JOHAB_RIEUL, 0, 0},
  {JOHAB_MIEUM, 0, 0},
  {JOHAB_PIEUP, 0, 0},
  {JOHAB_SSANGPIEUP, 0, 0},
  {JOHAB_SIOS, 0, 0},
  {JOHAB_SSANGSIOS, 0, 0},
  {JOHAB_IEUNG, 0, 0},
  {JOHAB_CIEUC, 0, 0},
  {JOHAB_SSANGCIEUC, 0, 0},
  {JOHAB_CHIEUCH, 0, 0},
  {JOHAB_KHIEUKH, 0, 0},
  {JOHAB_THIEUTH, 0, 0},
  {JOHAB_PHIEUPH, 0, 0},
  {JOHAB_HIEUH, 0, 0},
  /* Some of the following are representable as a glyph, the others not. */
  {JOHAB_NIEUN, JOHAB_KIYEOK, 0},
  {JOHAB_NIEUN, JOHAB_NIEUN, 0},
  {JOHAB_NIEUN, JOHAB_TIKEUT, 0},
  {JOHAB_NIEUN, JOHAB_PIEUP, 0},
  {JOHAB_TIKEUT, JOHAB_KIYEOK, 0},
  {JOHAB_RIEUL, JOHAB_NIEUN, 0},
  {JOHAB_RIEUL, JOHAB_RIEUL, 0},
  {JOHAB_RIEUL_HIEUH, 0, 0},
  {JOHAB_RIEUL, JOHAB_IEUNG, 0},
  {JOHAB_MIEUM, JOHAB_PIEUP, 0},
  {JOHAB_MIEUM, JOHAB_IEUNG, 0}, /* KAPYEOUNMIEUM */
  {JOHAB_PIEUP_KIYEOK, 0, 0},
  {JOHAB_PIEUP, JOHAB_NIEUN, 0},
  {JOHAB_PIEUP_TIKEUT, 0, 0},
  {JOHAB_PIEUP_SIOS, 0, 0},
  {JOHAB_PIEUP, JOHAB_SIOS, JOHAB_KIYEOK},
  {JOHAB_PIEUP, JOHAB_SIOS, JOHAB_TIKEUT},
  {JOHAB_PIEUP, JOHAB_SIOS, JOHAB_PIEUP},
  {JOHAB_PIEUP, JOHAB_SIOS, JOHAB_SIOS},
  {JOHAB_PIEUP, JOHAB_SIOS, JOHAB_CIEUC},
  {JOHAB_PIEUP_CIEUC, 0, 0},
  {JOHAB_PIEUP, JOHAB_CHIEUCH, 0},
  {JOHAB_PIEUP, JOHAB_THIEUTH, 0},
  {JOHAB_PIEUP, JOHAB_PHIEUPH, 0},
  {JOHAB_KAPYEOUNPIEUP, 0, 0},
  {JOHAB_SSANGPIEUP, JOHAB_IEUNG, 0}, /* KAPYEOUNSSANGPIEUP */
  {JOHAB_SIOS_KIYEOK, 0, 0},
  {JOHAB_SIOS, JOHAB_NIEUN, 0},
  {JOHAB_SIOS_TIKEUT, 0, 0},
  {JOHAB_SIOS, JOHAB_RIEUL, 0},
  {JOHAB_SIOS, JOHAB_MIEUM, 0},
  {JOHAB_SIOS_PIEUP, 0, 0},
  {JOHAB_SIOS, JOHAB_PIEUP, JOHAB_KIYEOK},
  {JOHAB_SIOS, JOHAB_SIOS, JOHAB_SIOS},
  {JOHAB_SIOS, JOHAB_IEUNG, 0},
  {JOHAB_SIOS_CIEUC, 0, 0},
  {JOHAB_SIOS, JOHAB_CHIEUCH, 0},
  {JOHAB_SIOS, JOHAB_KHIEUKH, 0},
  {JOHAB_SIOS, JOHAB_THIEUTH, 0},
  {JOHAB_SIOS, JOHAB_PHIEUPH, 0},
  {JOHAB_SIOS, JOHAB_HIEUH, 0},
  {JOHAB_CHITUEUMSIOS, 0, 0},
  {JOHAB_CHITUEUMSIOS, JOHAB_CHITUEUMSIOS, 0},
  {JOHAB_CEONGCHITUEUMSIOS, 0, 0},
  {JOHAB_CEONGCHITUEUMSIOS, JOHAB_CEONGCHITUEUMSIOS, 0},
  {JOHAB_PANSIOS, 0, 0},
  {JOHAB_IEUNG, JOHAB_KIYEOK, 0},
  {JOHAB_IEUNG, JOHAB_TIKEUT, 0},
  {JOHAB_IEUNG, JOHAB_MIEUM, 0},
  {JOHAB_IEUNG, JOHAB_PIEUP, 0},
  {JOHAB_IEUNG, JOHAB_SIOS, 0},
  {JOHAB_IEUNG, JOHAB_PANSIOS, 0},
  {JOHAB_IEUNG, JOHAB_IEUNG, 0},
  {JOHAB_IEUNG, JOHAB_CIEUC, 0},
  {JOHAB_IEUNG, JOHAB_CHIEUCH, 0},
  {JOHAB_IEUNG, JOHAB_THIEUTH, 0},
  {JOHAB_IEUNG, JOHAB_PHIEUPH, 0},
  {JOHAB_YESIEUNG, 0, 0},
  {JOHAB_CIEUC, JOHAB_IEUNG, 0},
  {JOHAB_CHITUEUMCIEUC, 0, 0},
  {JOHAB_CHITUEUMCIEUC, JOHAB_CHITUEUMCIEUC, 0},
  {JOHAB_CEONGEUMCIEUC, 0, 0},
  {JOHAB_CEONGEUMCIEUC, JOHAB_CEONGEUMCIEUC, 0},
  {JOHAB_CHIEUCH, JOHAB_KHIEUKH, 0},
  {JOHAB_CHIEUCH, JOHAB_HIEUH, 0},
  {JOHAB_CHITUEUMCHIEUCH, 0, 0},
  {JOHAB_CEONGEUMCHIEUCH, 0, 0},
  {JOHAB_PHIEUPH, JOHAB_PIEUP, 0},
  {JOHAB_PHIEUPH, JOHAB_IEUNG, 0}, /* KAPYEOUNPHIEUPH */
  {JOHAB_HIEUH, JOHAB_HIEUH, 0},
  {JOHAB_YEORINHIEUH, 0, 0},
  /* 0x115A ~ 0x115E -- reserved */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  /* CHOSEONG FILLER */
  {0, 0, 0},

  /*
   * JUNGSEONG
   */
  /*
   * JUNGSEONG
   */
  {0, 0, 0},			/* JUNGSEONG FILL */
  /* JUNGSEONG 0x1161 -- 0x1175 : matched to each ksc5601 Jamos extactly.  */
  {JOHAB_A, 0, 0},
  {JOHAB_AE, 0, 0},
  {JOHAB_YA, 0, 0},
  {JOHAB_YAE, 0, 0},
  {JOHAB_EO, 0, 0},
  {JOHAB_E, 0, 0},
  {JOHAB_YEO, 0, 0},
  {JOHAB_YE, 0, 0},
  {JOHAB_O, 0, 0},
  {JOHAB_WA, 0, 0},
  {JOHAB_WAE, 0, 0},
  {JOHAB_OE, 0, 0},
  {JOHAB_YO, 0, 0},
  {JOHAB_U, 0, 0},
  {JOHAB_WEO, 0, 0},
  {JOHAB_WE, 0, 0},
  {JOHAB_WI, 0, 0},
  {JOHAB_YU, 0, 0},
  {JOHAB_EU, 0, 0},
  {JOHAB_YI, 0, 0},
  {JOHAB_I, 0, 0},
  /* Some of the following are representable as a glyph, the others not. */
  {JOHAB_A, JOHAB_O, 0},
  {JOHAB_A, JOHAB_U, 0},
  {JOHAB_YA, JOHAB_O, 0},
  {JOHAB_YA, JOHAB_YO, 0},
  {JOHAB_EO, JOHAB_O, 0},
  {JOHAB_EO, JOHAB_U, 0},
  {JOHAB_EO, JOHAB_EU, 0},
  {JOHAB_YEO, JOHAB_O, 0},
  {JOHAB_YEO, JOHAB_U, 0},
  {JOHAB_O, JOHAB_EO, 0},
  {JOHAB_O, JOHAB_E, 0},
  {JOHAB_O, JOHAB_YE, 0},
  {JOHAB_O, JOHAB_O, 0},
  {JOHAB_O, JOHAB_U, 0},
  {JOHAB_YO_YA, 0, 0},
  {JOHAB_YO_YAE, 0, 0},
  {JOHAB_YO, JOHAB_YEO, 0},
  {JOHAB_YO, JOHAB_O, 0},
  {JOHAB_YO_I, 0, 0},
  {JOHAB_U, JOHAB_A, 0},
  {JOHAB_U, JOHAB_AE, 0},
  {JOHAB_U, JOHAB_EO, JOHAB_EU},
  {JOHAB_U, JOHAB_YE, 0},
  {JOHAB_U, JOHAB_U, 0},
  {JOHAB_YU, JOHAB_A, 0},
  {JOHAB_YU, JOHAB_EO, 0},
  {JOHAB_YU, JOHAB_E, 0},
  {JOHAB_YU_YEO, 0, 0},
  {JOHAB_YU_YE, 0, 0},
  {JOHAB_YU, JOHAB_U, 0},
  {JOHAB_YU_I, 0, 0},
  {JOHAB_EU, JOHAB_U, 0},
  {JOHAB_EU, JOHAB_EU, 0},
  {JOHAB_YI, JOHAB_U, 0},
  {JOHAB_I, JOHAB_A, 0},
  {JOHAB_I, JOHAB_YA, 0},
  {JOHAB_I, JOHAB_O, 0},
  {JOHAB_I, JOHAB_U, 0},
  {JOHAB_I, JOHAB_EU, 0},
  {JOHAB_I, JOHAB_ARAEA, 0},
  {JOHAB_ARAEA, 0, 0},
  {JOHAB_ARAEA, JOHAB_EO, 0},
  {JOHAB_ARAEA, JOHAB_U, 0},
  {JOHAB_ARAEA, JOHAB_I, 0},
  {JOHAB_ARAEA, JOHAB_ARAEA, 0},
  /* 0x11A3 ~ 0x11A7 -- reserved */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},			/* (INTERNAL) JONGSEONG FILL */
  
  /*
   * JONGSEONG
   */
  {JOHAB_KIYEOK, 0, 0},
  {JOHAB_SSANGKIYEOK, 0, 0},
  {JOHAB_KIYEOK_SIOS, 0, 0},
  {JOHAB_NIEUN, 0, 0},
  {JOHAB_NIEUN_CIEUC, 0, 0},
  {JOHAB_NIEUN_HIEUH, 0, 0},
  {JOHAB_TIKEUT, 0, 0},
  {JOHAB_RIEUL, 0, 0},
  {JOHAB_RIEUL_KIYEOK, 0, 0},
  {JOHAB_RIEUL_MIEUM, 0, 0},
  {JOHAB_RIEUL_PIEUP, 0, 0},
  {JOHAB_RIEUL_SIOS, 0, 0},
  {JOHAB_RIEUL, JOHAB_TIKEUT, 0},
  {JOHAB_RIEUL_PHIEUPH, 0, 0},
  {JOHAB_RIEUL_HIEUH, 0, 0},
  {JOHAB_MIEUM, 0, 0},
  {JOHAB_PIEUP, 0, 0},
  {JOHAB_PIEUP_SIOS, 0, 0},
  {JOHAB_SIOS, 0, 0},
  {JOHAB_SSANGSIOS, 0, 0},
  {JOHAB_IEUNG, 0, 0},
  {JOHAB_CIEUC, 0, 0},
  {JOHAB_CHIEUCH, 0, 0},
  {JOHAB_KHIEUKH, 0, 0},
  {JOHAB_THIEUTH, 0, 0},
  {JOHAB_PHIEUPH, 0, 0},
  {JOHAB_HIEUH, 0, 0},
  {JOHAB_KIYEOK, JOHAB_RIEUL, 0},
  {JOHAB_KIYEOK, JOHAB_SIOS, JOHAB_KIYEOK},
  {JOHAB_NIEUN, JOHAB_KIYEOK, 0},
  {JOHAB_NIEUN, JOHAB_TIKEUT, 0},
  {JOHAB_NIEUN, JOHAB_SIOS, 0},
  {JOHAB_NIEUN, JOHAB_PANSIOS, 0},
  {JOHAB_NIEUN, JOHAB_THIEUTH, 0},
  {JOHAB_TIKEUT, JOHAB_KIYEOK, 0},
  {JOHAB_TIKEUT, JOHAB_RIEUL, 0},
  {JOHAB_RIEUL, JOHAB_KIYEOK, JOHAB_SIOS},
  {JOHAB_RIEUL, JOHAB_NIEUN, 0},
  {JOHAB_RIEUL, JOHAB_TIKEUT, 0},
  {JOHAB_RIEUL, JOHAB_TIKEUT, JOHAB_HIEUH},
  {JOHAB_RIEUL, JOHAB_RIEUL, 0},
  {JOHAB_RIEUL, JOHAB_MIEUM, JOHAB_KIYEOK},
  {JOHAB_RIEUL, JOHAB_MIEUM, JOHAB_SIOS},
  {JOHAB_RIEUL, JOHAB_PIEUP, JOHAB_SIOS},
  {JOHAB_RIEUL, JOHAB_PHIEUPH, JOHAB_HIEUH},
  {JOHAB_RIEUL, JOHAB_KAPYEOUNPIEUP, 0},
  {JOHAB_RIEUL, JOHAB_SIOS, JOHAB_SIOS},
  {JOHAB_RIEUL, JOHAB_PANSIOS, 0},
  {JOHAB_RIEUL, JOHAB_KHIEUKH, 0},
  {JOHAB_RIEUL_YEORINHIEUH, 0, 0},
  {JOHAB_MIEUM, JOHAB_KIYEOK, 0},
  {JOHAB_MIEUM, JOHAB_RIEUL, 0},
  {JOHAB_MIEUM, JOHAB_PIEUP, 0},
  {JOHAB_MIEUM, JOHAB_SIOS, 0},
  {JOHAB_MIEUM, JOHAB_SIOS, JOHAB_SIOS},
  {JOHAB_MIEUM, JOHAB_PANSIOS, 0},
  {JOHAB_MIEUM, JOHAB_CHIEUCH, 0},
  {JOHAB_MIEUM, JOHAB_HIEUH, 0},
  {JOHAB_MIEUM, JOHAB_IEUNG, 0}, /* KAPYEOUNMIEUM */
  {JOHAB_PIEUP, JOHAB_RIEUL, 0},
  {JOHAB_PIEUP, JOHAB_PHIEUPH, 0},
  {JOHAB_PIEUP, JOHAB_HIEUH, 0},
  {JOHAB_KAPYEOUNPIEUP, 0, 0},
  {JOHAB_SIOS_KIYEOK, 0, 0},
  {JOHAB_SIOS_TIKEUT, 0, 0},
  {JOHAB_SIOS, JOHAB_RIEUL, 0},
  {JOHAB_SIOS_PIEUP, 0, 0},
  {JOHAB_PANSIOS, 0, 0},
  {JOHAB_IEUNG, JOHAB_KIYEOK, 0},
  {JOHAB_IEUNG, JOHAB_KIYEOK, JOHAB_KIYEOK},
  {JOHAB_IEUNG, JOHAB_IEUNG, 0},
  {JOHAB_IEUNG, JOHAB_KHIEUKH, 0},
  {JOHAB_YESIEUNG, 0, 0},
  {JOHAB_YESIEUNG, JOHAB_SIOS, 0},
  {JOHAB_YESIEUNG, JOHAB_PANSIOS, 0},
  {JOHAB_PHIEUPH, JOHAB_PIEUP, 0},
  {JOHAB_PHIEUPH, JOHAB_IEUNG, 0}, /* KAPYEOUNPHIEUPH */
  {JOHAB_HIEUH, JOHAB_NIEUN, 0},
  {JOHAB_HIEUH, JOHAB_RIEUL, 0},
  {JOHAB_HIEUH, JOHAB_MIEUM, 0},
  {JOHAB_HIEUH, JOHAB_PIEUP, 0},
  {JOHAB_YEORINHIEUH, 0, 0},
  /* reserved */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

