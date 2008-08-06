/* Pango
 * indic-ot-class-tables.c:
 *
 * Copyright (C) 2001, 2002 IBM Corporation.  All Rights Reserved.
 * Author: Eric Mader <mader@jtcsv.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished
 * to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software and that
 * both the above copyright notice(s) and this permission notice
 * appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR
 * ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization of the copyright holder.
 */

#include "config.h"

#include "indic-ot.h"


/*
 * Split matra table indices
 */
#define _x1 (1 << CF_INDEX_SHIFT)
#define _x2 (2 << CF_INDEX_SHIFT)
#define _x3 (3 << CF_INDEX_SHIFT)
#define _x4 (4 << CF_INDEX_SHIFT)
#define _x5 (5 << CF_INDEX_SHIFT)
#define _x6 (6 << CF_INDEX_SHIFT)
#define _x7 (7 << CF_INDEX_SHIFT)
#define _x8 (8 << CF_INDEX_SHIFT)
#define _x9 (9 << CF_INDEX_SHIFT)

/*
 * Simple classes
 */
#define _xx (CC_RESERVED)
#define _ma (CC_MODIFYING_MARK_ABOVE)
#define _mp (CC_MODIFYING_MARK_POST)
#define _iv (CC_INDEPENDENT_VOWEL)
#define _ct (CC_CONSONANT | CF_CONSONANT)
#define _cn (CC_CONSONANT_WITH_NUKTA | CF_CONSONANT)
#define _nu (CC_NUKTA)
#define _dv (CC_DEPENDENT_VOWEL)
#define _dl (_dv | CF_MATRA_PRE)
#define _db (_dv | CF_MATRA_BELOW)
#define _da (_dv | CF_MATRA_ABOVE)
#define _dr (_dv | CF_MATRA_POST)
#define _lm (_dv | CF_LENGTH_MARK)
#define _vr (CC_VIRAMA)
#define _al (CC_AL_LAKUNA)

/*
 * Split matras
 */
#define _s1 (_dv | _x1)
#define _s2 (_dv | _x2)
#define _s3 (_dv | _x3)
#define _s4 (_dv | _x4)
#define _s5 (_dv | _x5)
#define _s6 (_dv | _x6)
#define _s7 (_dv | _x7)
#define _s8 (_dv | _x8)
#define _s9 (_dv | _x9)

/*
 * consonants with special forms
 * NOTE: this assumes that no consonants with nukta have
 * special forms... (Bengali RA?)
 */
#define _bb (_ct | CF_BELOW_BASE)
#define _pb (_ct | CF_POST_BASE)
#define _vt (_bb | CF_VATTU)
#define _rv (_vt | CF_REPH)
#define _rp (_pb | CF_REPH)
#define _rb (_bb | CF_REPH)


/*
 * Character class tables
 */
static const IndicOTCharClass devaCharClasses[] =
{
    _xx, _ma, _ma, _mp, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, /* 0900 - 090F */
    _iv, _iv, _iv, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, /* 0910 - 091F */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _cn, _ct, _ct, _ct, _ct, _ct, _ct, /* 0920 - 092F */
    _rv, _cn, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _dr, _dl, /* 0930 - 093F */
    _dr, _db, _db, _db, _db, _da, _da, _da, _da, _dr, _dr, _dr, _dr, _vr, _xx, _xx, /* 0940 - 094F */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _cn, _cn, _cn, _cn, _cn, _cn, _cn, _cn, /* 0950 - 095F */
    _iv, _iv, _db, _db, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0960 - 096F */
    _xx, _xx, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _ct, _ct, _ct, _ct, _ct  /* 0970 - 097F */
};

/* As a hack, BENGALI LETTER A (U+0985) and BENGALI LETTER E (U+098F)
 * are marked as consonants below; this gives approximately the
 * right behavior for the sequences "a halant ya aa" and
 * "e halant ya aa".
 */
static const IndicOTCharClass bengCharClasses[] =
{
    _xx, _ma, _mp, _mp, _xx, _ct, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _ct, /* 0980 - 098F */
    _iv, _xx, _xx, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, /* 0990 - 099F */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _bb, _ct, _ct, _pb, /* 09A0 - 09AF */
    _rv, _xx, _ct, _xx, _xx, _xx, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _dr, _dl, /* 09B0 - 09BF */
    _dr, _db, _db, _db, _db, _xx, _xx, _dl, _dl, _xx, _xx, _s1, _s2, _vr, _xx, _xx, /* 09C0 - 09CF */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _dr, _xx, _xx, _xx, _xx, _cn, _cn, _xx, _cn, /* 09D0 - 09DF */
    _iv, _iv, _dv, _dv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 09E0 - 09EF */
    _rv, _ct, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx                           /* 09F0 - 09FA */
};

static const IndicOTCharClass guruCharClasses[] =
{
    _xx, _ma, _ma, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _xx, _xx, _iv, /* 0A00 - 0A0F */
    _iv, _xx, _xx, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, /* 0A10 - 0A1F */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _bb, /* 0A20 - 0A2F */
    _vt, _xx, _ct, _cn, _xx, _bb, _cn, _xx, _ct, _bb, _xx, _xx, _nu, _xx, _dr, _dl, /* 0A30 - 0A3F */
    _dr, _db, _db, _xx, _xx, _xx, _xx, _da, _da, _xx, _xx, _da, _da, _vr, _xx, _xx, /* 0A40 - 0A4F */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _cn, _cn, _cn, _ct, _xx, _cn, _xx, /* 0A50 - 0A5F */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0A60 - 0A6F */
    _ma, _ma, _ct, _ct, _xx                                                         /* 0A70 - 0A74 */
};

static const IndicOTCharClass gujrCharClasses[] =
{
    _xx, _ma, _ma, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _xx, _iv, /* 0A80 - 0A8F */
    _iv, _iv, _xx, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, /* 0A90 - 0A9F */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _ct, /* 0AA0 - 0AAF */
    _rv, _xx, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _nu, _xx, _dr, _dl, /* 0AB0 - 0ABF */
    _dr, _db, _db, _db, _db, _da, _xx, _da, _da, _dr, _xx, _dr, _dr, _vr, _xx, _xx, /* 0AC0 - 0ACF */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0AD0 - 0ADF */
    _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  /* 0AE0 - 0AEF */
};

static const IndicOTCharClass oryaCharClasses[] =
{
    _xx, _ma, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _iv, /* 0B00 - 0B0F */
    _iv, _xx, _xx, _iv, _iv, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _ct, _bb, /* 0B10 - 0B1F */
    _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _pb, /* 0B20 - 0B2F */
    _rb, _xx, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _xx, _xx, _nu, _xx, _dr, _da, /* 0B30 - 0B3F */
    _dr, _db, _db, _db, _xx, _xx, _xx, _dl, _s1, _xx, _xx, _s2, _s3, _vr, _xx, _xx, /* 0B40 - 0B4F */
    _xx, _xx, _xx, _xx, _xx, _xx, _da, _dr, _xx, _xx, _xx, _xx, _cn, _cn, _xx, _pb, /* 0B50 - 0B5F */
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0B60 - 0B6F */
    _xx, _bb                                                                        /* 0B70 - 0B71 */
};

static const IndicOTCharClass tamlCharClasses[] =
{
    _xx, _xx, _ma, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _xx, _iv, _iv, /* 0B80 - 0B8F */
    _iv, _xx, _iv, _iv, _iv, _ct, _xx, _xx, _xx, _ct, _ct, _xx, _ct, _xx, _ct, _ct, /* 0B90 - 0B9F */
    _xx, _xx, _xx, _ct, _ct, _xx, _xx, _xx, _ct, _ct, _ct, _xx, _xx, _xx, _ct, _ct, /* 0BA0 - 0BAF */
    _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _xx, _xx, _xx, _xx, _dr, _dr, /* 0BB0 - 0BBF */
    _da, _dr, _dr, _xx, _xx, _xx, _dl, _dl, _dl, _xx, _s1, _s2, _s3, _vr, _xx, _xx, /* 0BC0 - 0BCF */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _dr, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0BD0 - 0BDF */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0BE0 - 0BEF */
    _xx, _xx, _xx                                                                   /* 0BF0 - 0BF2 */
};

/* FIXME: Should some of the bb's be pb's? (KA, NA, MA, YA, VA, etc. (approx 13)) */
static const IndicOTCharClass teluCharClasses[] =
{
    _xx, _mp, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _iv, /* 0C00 - 0C0F */
    _iv, _xx, _iv, _iv, _iv, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, /* 0C10 - 0C1F */
    _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _bb, /* 0C20 - 0C2F */
    _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _xx, _xx, _xx, _xx, _da, _da, /* 0C30 - 0C3F */
    _da, _dr, _dr, _dr, _dr, _xx, _da, _da, _s1, _xx, _da, _da, _da, _vr, _xx, _xx, /* 0C40 - 0C4F */
    _xx, _xx, _xx, _xx, _xx, _da, _db, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0C50 - 0C5F */
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  /* 0C60 - 0C6F */
};

/* U+CC3 and U+CC4 are _lm here not _dr since the Kannada rendering
 * rules want them below and to the right of the entire cluster. They
 * aren't, strictly speaking, length marks, however.
 *
 * There's some information about this in:
 *
 *  http://brahmi.sourceforge.net/docs/KannadaComputing.html
 */
static const IndicOTCharClass kndaCharClasses[] =
{
    _xx, _xx, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _iv, /* 0C80 - 0C8F */
    _iv, _xx, _iv, _iv, _iv, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, /* 0C90 - 0C9F */
    _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _bb, /* 0CA0 - 0CAF */
    _rb, _ct, _bb, _bb, _xx, _bb, _bb, _bb, _bb, _bb, _xx, _xx, _xx, _xx, _dr, _da, /* 0CB0 - 0CBF */
    _s1, _dr, _dr, _lm, _lm, _xx, _da, _s2, _s3, _xx, _s4, _s5, _da, _vr, _xx, _xx, /* 0CC0 - 0CCF */
    _xx, _xx, _xx, _xx, _xx, _lm, _lm, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _ct, _xx, /* 0CD0 - 0CDF */
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  /* 0CE0 - 0CEF */
};

/*
 * FIXME: this is correct for old-style Malayalam (MAL) but not for reformed Malayalam (MLR)
 * FIXME: should there be a REPH for old-style Malayalam?
 */
static const IndicOTCharClass mlymCharClasses[] =
{
    _xx, _xx, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _iv, _iv, /* 0D00 - 0D0F */
    _iv, _xx, _iv, _iv, _iv, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, /* 0D10 - 0D1F */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _pb, /* 0D20 - 0D2F */
    _cn, _cn, _ct, _ct, _ct, _pb, _ct, _ct, _ct, _ct, _xx, _xx, _xx, _xx, _dr, _dr, /* 0D30 - 0D3F */
    _dr, _dr, _dr, _dr, _xx, _xx, _dl, _dl, _dl, _xx, _s1, _s2, _s3, _vr, _xx, _xx, /* 0D40 - 0D4F */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _dr, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0D50 - 0D5F */
    _iv, _iv, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx  /* 0D60 - 0D6F */
};

static const IndicOTCharClass sinhCharClasses[] =
{
    _xx, _xx, _mp, _mp, _xx, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, _iv, /* 0D80 - 0D8F */
    _iv, _iv, _iv, _iv, _iv, _iv, _iv, _xx, _xx, _xx, _ct, _ct, _ct, _ct, _ct, _ct, /* 0D90 - 0D9F */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, /* 0DA0 - 0DAF */
    _ct, _ct, _xx, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _ct, _xx, _xx, /* 0DB0 - 0DBF */
    _ct, _ct, _ct, _ct, _ct, _ct, _ct, _xx, _xx, _xx, _al, _xx, _xx, _xx, _xx, _dr, /* 0DC0 - 0DCF */
    _dr, _dr, _da, _da, _db, _xx, _db, _xx, _dr, _dl, _s1, _dl, _s2, _s3, _s4, _dr, /* 0DD0 - 0DDF */
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0DE0 - 0DEF */
    _xx, _xx, _dr, _dr, _xx                                                         /* 0DF0 - 0DF4 */
};


/*
 * Split matra tables
 */
static const IndicOTSplitMatra bengSplitTable[] = {{0x09C7, 0x09BE}, {0x09C7, 0x09D7}};

static const IndicOTSplitMatra oryaSplitTable[] = {{0x0B47, 0x0B56}, {0x0B47, 0x0B3E}, {0x0B47, 0x0B57}};

static const IndicOTSplitMatra tamlSplitTable[] = {{0x0BC6, 0x0BBE}, {0x0BC7, 0x0BBE}, {0x0BC6, 0x0BD7}};

static const IndicOTSplitMatra teluSplitTable[] = {{0x0C46, 0x0C56}};

static const IndicOTSplitMatra kndaSplitTable[] = {{0x0CBF, 0x0CD5}, {0x0CC6, 0x0CD5}, {0x0CC6, 0x0CD6}, {0x0CC6, 0x0CC2},
				      {0x0CC6, 0x0CC2, 0x0CD5}};

static const IndicOTSplitMatra mlymSplitTable[] = {{0x0D46, 0x0D3E}, {0x0D47, 0x0D3E}, {0x0D46, 0x0D57}};

static const IndicOTSplitMatra sinhSplitTable[] = {{0x0DD9, 0x0DCA}, {0x0DD9, 0x0DCF}, {0x0DD9, 0x0DCF, 0x0DCA},
						   {0x0DD9, 0x0DDF} };


/*
 * Script Flags
 */

/*
 * FIXME: post 'GSUB' reordering of MATRA_PRE's for Malayalam and Tamil
 * FIXME: reformed Malayalam needs to reorder VATTU to before base glyph...
 * FIXME: eyelash RA only for Devanagari??
 */
#define DEVA_SCRIPT_FLAGS (SF_EYELASH_RA | SF_NO_POST_BASE_LIMIT)
#define BENG_SCRIPT_FLAGS (SF_REPH_AFTER_BELOW | SF_NO_POST_BASE_LIMIT)
#define GURU_SCRIPT_FLAGS (SF_NO_POST_BASE_LIMIT)
#define GUJR_SCRIPT_FLAGS (SF_NO_POST_BASE_LIMIT)
#define ORYA_SCRIPT_FLAGS (SF_REPH_AFTER_BELOW | SF_NO_POST_BASE_LIMIT)
#define TAML_SCRIPT_FLAGS (SF_MPRE_FIXUP | SF_NO_POST_BASE_LIMIT)
#define TELU_SCRIPT_FLAGS (SF_MATRAS_AFTER_BASE | 3)
#define KNDA_SCRIPT_FLAGS (SF_MATRAS_AFTER_BASE | 3)
#define MLYM_SCRIPT_FLAGS (SF_MPRE_FIXUP | SF_NO_POST_BASE_LIMIT | SF_PROCESS_ZWJ)
#define SINH_SCRIPT_FLAGS (SF_NO_POST_BASE_LIMIT | SF_PROCESS_ZWJ)

/*
 * Indic Class Tables
 */
/* Add a little macro to compute lastChar based on size of the charClasses * table */
#define INDIC_OT_CLASS_TABLE_DEFINE(name, firstChar, worstCaseExpansion, scriptFlags, charClasses, splitMatraTable) \
  const IndicOTClassTable name = {firstChar, firstChar + G_N_ELEMENTS (charClasses) - 1, \
				  worstCaseExpansion, scriptFlags, charClasses, splitMatraTable}
INDIC_OT_CLASS_TABLE_DEFINE (deva_class_table, 0x0900, 2, DEVA_SCRIPT_FLAGS, devaCharClasses, NULL);
INDIC_OT_CLASS_TABLE_DEFINE (beng_class_table, 0x0980, 3, BENG_SCRIPT_FLAGS, bengCharClasses, bengSplitTable);
INDIC_OT_CLASS_TABLE_DEFINE (guru_class_table, 0x0A00, 2, GURU_SCRIPT_FLAGS, guruCharClasses, NULL);
INDIC_OT_CLASS_TABLE_DEFINE (gujr_class_table, 0x0A80, 2, GUJR_SCRIPT_FLAGS, gujrCharClasses, NULL);
INDIC_OT_CLASS_TABLE_DEFINE (orya_class_table, 0x0B00, 3, ORYA_SCRIPT_FLAGS, oryaCharClasses, oryaSplitTable);
INDIC_OT_CLASS_TABLE_DEFINE (taml_class_table, 0x0B80, 3, TAML_SCRIPT_FLAGS, tamlCharClasses, tamlSplitTable);
INDIC_OT_CLASS_TABLE_DEFINE (telu_class_table, 0x0C00, 3, TELU_SCRIPT_FLAGS, teluCharClasses, teluSplitTable);
INDIC_OT_CLASS_TABLE_DEFINE (knda_class_table, 0x0C80, 4, KNDA_SCRIPT_FLAGS, kndaCharClasses, kndaSplitTable);
INDIC_OT_CLASS_TABLE_DEFINE (mlym_class_table, 0x0D00, 3, MLYM_SCRIPT_FLAGS, mlymCharClasses, mlymSplitTable);
INDIC_OT_CLASS_TABLE_DEFINE (sinh_class_table, 0x0D80, 4, SINH_SCRIPT_FLAGS, sinhCharClasses, sinhSplitTable);

const IndicOTSplitMatra *indic_ot_get_split_matra(const IndicOTClassTable *class_table, IndicOTCharClass char_class)
{
    gint32 index = (char_class & CF_INDEX_MASK) >> CF_INDEX_SHIFT;

    return &class_table->splitMatraTable[index - 1];
}

gboolean indic_ot_is_vm_above(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_VM_ABOVE(char_class);
}

gboolean indic_ot_is_vm_post(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_VM_POST(char_class);
}

gboolean indic_ot_is_consonant(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_CONSONANT(char_class);
}

gboolean indic_ot_is_reph(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_REPH(char_class);
}

gboolean indic_ot_is_virama(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return (IS_VIRAMA(char_class) || IS_AL_LAKUNA(char_class));
}

gboolean indic_ot_is_al_lakuna(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_AL_LAKUNA(char_class);
}

gboolean indic_ot_is_nukta(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_NUKTA(char_class);
}

gboolean indic_ot_is_vattu(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_VATTU(char_class);
}

gboolean indic_ot_is_matra(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_MATRA(char_class);
}

gboolean indic_ot_is_split_matra(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_SPLIT_MATRA(char_class);
}

gboolean indic_ot_is_m_pre(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_M_PRE(char_class);
}

gboolean indic_ot_is_m_below(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_M_BELOW(char_class);
}

gboolean indic_ot_is_m_above(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_M_ABOVE(char_class);
}

gboolean indic_ot_is_m_post(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_M_POST(char_class);
}

gboolean indic_ot_is_length_mark(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return IS_LENGTH_MARK(char_class);
}

gboolean indic_ot_has_post_or_below_base_form(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return HAS_POST_OR_BELOW_BASE_FORM(char_class);
}

gboolean indic_ot_has_post_base_form(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return HAS_POST_BASE_FORM(char_class);
}

gboolean indic_ot_has_below_base_form(const IndicOTClassTable *class_table, gunichar ch)
{
  IndicOTCharClass char_class = indic_ot_get_char_class(class_table, ch);

  return HAS_BELOW_BASE_FORM(char_class);
}

IndicOTCharClass indic_ot_get_char_class(const IndicOTClassTable *class_table, gunichar ch)
{
    if (ch == C_SIGN_ZWJ) {
	return CF_CONSONANT | CC_ZERO_WIDTH_MARK;
    }

    if (ch == C_SIGN_ZWNJ) {
	return CC_ZERO_WIDTH_MARK;
    }

    if (ch < class_table->firstChar || ch > class_table->lastChar) {
	return CC_RESERVED;
    }

    return class_table->charClasses[ch - class_table->firstChar];
}

static const gint8 stateTable[][CC_COUNT] =
{
/*   xx  ma  mp  iv  ct  cn  nu  dv  vr  zw  al */
    { 1,  1,  1,  5,  3,  2,  1,  1,  1,  1,  1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1,  6,  1, -1, -1, -1, -1,  5,  4, -1, -1},
    {-1,  6,  1, -1, -1, -1,  2,  5,  4, 10,  9},
    {-1, -1, -1, -1,  3,  2, -1, -1, -1,  8, -1},
    {-1,  6,  1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1,  7,  1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1,  3,  2, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1,  8, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1,  8, -1,  8}

};

glong indic_ot_find_syllable(const IndicOTClassTable *class_table, const gunichar *chars, glong prev, glong char_count)
{
    glong cursor = prev;
    gint8 state = 0;

    while (cursor < char_count) {
	IndicOTCharClass char_class = indic_ot_get_char_class(class_table, chars[cursor]);

	state = stateTable[state][char_class & CF_CLASS_MASK];

	/*for the components of split matra*/
	if ((char_count >= cursor + 3) &&
	    (chars[cursor] == 0x0DD9 && chars[cursor + 1] == 0x0DCF && chars[cursor + 2] == 0x0DCA)) {  /*for 3 split matra of Sinhala*/
	    return cursor + 3;
	}
	else if ((char_count >= cursor + 3) &&
		 (chars[cursor] == 0x0CC6 && chars[cursor + 1] == 0x0CC2 && chars[cursor + 2] == 0x0CD5)) {  /*for 3 split matra of Kannada*/
	    return cursor + 3;
	}
	/*for 2 split matra*/
	else if (char_count >= cursor + 2) {
		/*for Bengali*/
	    if ((chars[cursor] == 0x09C7 && chars[cursor + 1] == 0x09BE) ||
		(chars[cursor] == 0x09C7 && chars[cursor + 1] == 0x09D7) ||
		/*for Oriya*/
		(chars[cursor] == 0x0B47 && chars[cursor + 1] == 0x0B3E) ||
		(chars[cursor] == 0x0B47 && chars[cursor + 1] == 0x0B56) ||
		(chars[cursor] == 0x0B47 && chars[cursor + 1] == 0x0B57) ||
		/*for Tamil*/
		(chars[cursor] == 0x0BC6 && chars[cursor + 1] == 0x0BBE) ||
		(chars[cursor] == 0x0BC6 && chars[cursor + 1] == 0x0BD7) ||
		(chars[cursor] == 0x0BC7 && chars[cursor + 1] == 0x0BBE) ||
		/*for Malayalam*/
		(chars[cursor] == 0x0D46 && chars[cursor + 1] == 0x0D3E) ||
		(chars[cursor] == 0x0D46 && chars[cursor + 1] == 0x0D57) ||
		(chars[cursor] == 0x0D47 && chars[cursor + 1] == 0x0D3E) ||
		/*for Sinhala*/
		(chars[cursor] == 0x0DD9 && chars[cursor + 1] == 0x0DCA) ||
		(chars[cursor] == 0x0DD9 && chars[cursor + 1] == 0x0DCF) ||
		(chars[cursor] == 0x0DD9 && chars[cursor + 1] == 0x0DDF) ||
		(chars[cursor] == 0x0DDC && chars[cursor + 1] == 0x0DCA) ||
		/*for Telugu*/
		(chars[cursor] == 0x0C46 && chars[cursor + 1] == 0x0C56) ||
		/*for Kannada*/
		(chars[cursor] == 0x0CBF && chars[cursor + 1] == 0x0CD5) ||
		(chars[cursor] == 0x0CC6 && chars[cursor + 1] == 0x0CD5) ||
		(chars[cursor] == 0x0CC6 && chars[cursor + 1] == 0x0CD6) ||
		(chars[cursor] == 0x0CC6 && chars[cursor + 1] == 0x0CC2) ||
		(chars[cursor] == 0x0CCA && chars[cursor + 1] == 0x0CD5))
		    return cursor + 2;
	}

	if (state < 0) {
	    break;
	}

	cursor += 1;
    }

    return cursor;
}

