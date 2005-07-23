/* Pango
 * indic-ot.h:
 *
 * Copyright (C) 2001, 2002 IBM Corporation. All Rights Reserved.
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

#ifndef __INDIC_OT_H__
#define __INDIC_OT_H__

#include <pango/pango-glyph.h>
#include <pango/pango-types.h>
#include "mprefixups.h"

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_ENGINE

/* Characters that get refered to by name... */
enum
{
  C_SIGN_ZWNJ     = 0x200C,
  C_SIGN_ZWJ      = 0x200D,
  C_DOTTED_CIRCLE = 0x25CC
};

/*
 * The characters that a split matra splits into.
 * Unused characters will be zero.
 */
typedef gunichar IndicOTSplitMatra[3];

/*
 * Character class values
 */
/* FIXME: does this need to be a typedef? */
typedef enum
{
  CC_RESERVED             = 0,
  CC_MODIFYING_MARK_ABOVE = 1,
  CC_MODIFYING_MARK_POST  = 2,
  CC_INDEPENDENT_VOWEL    = 3,
  CC_CONSONANT            = 4,
  CC_CONSONANT_WITH_NUKTA = 5,
  CC_NUKTA                = 6,
  CC_DEPENDENT_VOWEL      = 7,
  CC_VIRAMA               = 8,
  CC_ZERO_WIDTH_MARK      = 9,
  CC_AL_LAKUNA            = 10,
  CC_COUNT                = 11
} IndicOTCharClassValues;

/*
 * Character class flags
 */
/* FIXME: does this need to be a typedef? */
typedef enum
{
  CF_CLASS_MASK   = 0x0000FFFF,

  CF_CONSONANT    = (int)0x80000000,

  CF_REPH         = 0x40000000,
  CF_VATTU        = 0x20000000,
  CF_BELOW_BASE   = 0x10000000,
  CF_POST_BASE    = 0x08000000,

  CF_MATRA_PRE    = 0x04000000,
  CF_MATRA_BELOW  = 0x02000000,
  CF_MATRA_ABOVE  = 0x01000000,
  CF_MATRA_POST   = 0x00800000,
  CF_LENGTH_MARK  = 0x00400000,
  CF_INDEX_MASK   = 0x000F0000,
  CF_INDEX_SHIFT  = 16
} IndicOTCharClassFlags;

/*
 * Character class: a character class value
 * ORed with character class flags.
 */
typedef glong IndicOTCharClass;

/*
 * Script flags
 */
typedef enum
{
  SF_MATRAS_AFTER_BASE    = (int)0x80000000,
  SF_REPH_AFTER_BELOW     = 0x40000000,
  SF_EYELASH_RA           = 0x20000000,
  SF_MPRE_FIXUP           = 0x10000000,
  SF_PROCESS_ZWJ          = 0x08000000,

  SF_POST_BASE_LIMIT_MASK = 0x0000FFFF,
  SF_NO_POST_BASE_LIMIT   = 0x00007FFF
} IndicOTScriptFlags;

/*
 * Bit flags for the indic feature tags
 */
enum indic_glyph_feature_
{
  nukt = 0x0001,
  akhn = 0x0002,
  rphf = 0x0004,
  blwf = 0x0008,
  half = 0x0010,
  pstf = 0x0020,
  vatu = 0x0040,
  pres = 0x0080,
  blws = 0x0100,
  abvs = 0x0200,
  psts = 0x0400,
  haln = 0x0800,
  blwm = 0x1000,
  abvm = 0x2000,
  dist = 0x4000,
  junk = 0x8000,
  init = 0x10000
};

/*
 * Complement of the feature flags that
 * will be assigned to specific glyphs.
 *
 * The names come from the ICU implementation,
 * which listed the actual tags in an order
 * such that tags could be assigned using the
 * address of the first one: &tags[0], &tags[1],
 * &tags[2], &tags[3]. The name of each set here
 * is the name of the first tag in the ICU list.
 */
enum indic_glyph_property_
{
  rphf_p = (junk | dist | init),
  blwf_p = (junk | dist | init | rphf),
  half_p = (junk | dist | init | rphf | blwf),
  pstf_p = (junk | dist | init | rphf | blwf | half),
  nukt_p = (junk | dist | init | rphf | blwf | half | pstf)
};

/*
 * Macros to test the charClass flags for various things.
 */
#define IS_VM_ABOVE(charClass) ((charClass & CF_CLASS_MASK) == CC_MODIFYING_MARK_ABOVE)

#define IS_VM_POST(charClass) ((charClass & CF_CLASS_MASK) == CC_MODIFYING_MARK_POST)

#define IS_CONSONANT(charClass) ((charClass & CF_CONSONANT) != 0)

#define IS_REPH(charClass) ((charClass & CF_REPH) != 0)

#define IS_NUKTA(charClass) ((charClass & CF_CLASS_MASK) == CC_NUKTA)

#define IS_VIRAMA(charClass) ((charClass & CF_CLASS_MASK) == CC_VIRAMA)

#define IS_AL_LAKUNA(charClass) ((charClass & CF_CLASS_MASK) == CC_AL_LAKUNA)

#define IS_VATTU(charClass) ((charClass & CF_VATTU) != 0)

#define IS_MATRA(charClass) ((charClass & CF_CLASS_MASK) == CC_DEPENDENT_VOWEL)

#define IS_SPLIT_MATRA(charClass) ((charClass & CF_INDEX_MASK) != 0)

#define IS_M_PRE(charClass) ((charClass & CF_MATRA_PRE) != 0)

#define IS_M_BELOW(charClass) ((charClass & CF_MATRA_BELOW) != 0)

#define IS_M_ABOVE(charClass) ((charClass & CF_MATRA_ABOVE) != 0)

#define IS_M_POST(charClass) ((charClass & CF_MATRA_POST) != 0)

#define IS_LENGTH_MARK(charClass) ((charClass & CF_LENGTH_MARK) != 0)

#define HAS_POST_OR_BELOW_BASE_FORM(charClass) ((charClass & (CF_POST_BASE | CF_BELOW_BASE)) != 0)

#define HAS_POST_BASE_FORM(charClass) ((charClass & CF_POST_BASE) != 0)

#define HAS_BELOW_BASE_FORM(charClass) ((charClass & CF_BELOW_BASE) != 0)

struct _IndicOTClassTable
{
  gunichar	      firstChar;
  gunichar	      lastChar;
  glong  	      worstCaseExpansion;
  IndicOTScriptFlags  scriptFlags;

  const IndicOTCharClass  *charClasses;
  const IndicOTSplitMatra *splitMatraTable;
};

typedef struct _IndicOTClassTable IndicOTClassTable;

extern const IndicOTClassTable deva_class_table;
extern const IndicOTClassTable beng_class_table;
extern const IndicOTClassTable guru_class_table;
extern const IndicOTClassTable gujr_class_table;
extern const IndicOTClassTable orya_class_table;
extern const IndicOTClassTable taml_class_table;
extern const IndicOTClassTable telu_class_table;
extern const IndicOTClassTable knda_class_table;
extern const IndicOTClassTable mlym_class_table;
extern const IndicOTClassTable sinh_class_table;

const IndicOTSplitMatra *indic_ot_get_split_matra(const IndicOTClassTable *class_table, IndicOTCharClass char_class);

IndicOTCharClass indic_ot_get_char_class(const IndicOTClassTable *class_table, gunichar ch);

gboolean indic_ot_is_vm_above(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_vm_post(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_consonant(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_reph(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_virama(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_al_lakuna(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_nukta(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_vattu(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_matra(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_split_matra(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_m_pre(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_m_below(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_m_above(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_m_post(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_is_length_mark(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_has_post_or_below_base_form(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_has_post_base_form(const IndicOTClassTable *class_table, gunichar ch);
gboolean indic_ot_has_below_base_form(const IndicOTClassTable *class_table, gunichar ch);

glong indic_ot_find_syllable(const IndicOTClassTable *class_table, const gunichar *chars, glong prev, glong char_count);

glong indic_ot_reorder(const gunichar *chars, const glong *utf8_offsets, glong char_count, const IndicOTClassTable *class_table, gunichar *out_chars, glong *char_indices, gulong *char_tags, MPreFixups **outMPreFixups);

#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __INDIC_OT_H__ */
