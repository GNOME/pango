/* Pango
 * indic-ot.c:
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

#include "config.h"

#include "indic-ot.h"
#include "mprefixups.h"
/*
 * FIXME: should the IndicOutput stuff be moved
 * to a separate .h and .c file just to keep the
 * clutter down here? (it's not really usefull
 * anyplace else, is it?)
 */
struct _Output
{
    glong fOutIndex;

    const glong   *fOriginalOffsets;

    gunichar *fOutChars;
    glong   *fCharIndices;
    gulong  *fCharTags;

    gunichar fMpre;
    gunichar fMbelow;
    gunichar fMabove;
    gunichar fMpost;
    gunichar fLengthMark;
    gunichar fAlLakuna; /* to handle Al-Lakuna in sinhala split matras */
    glong    fMatraIndex;
    gulong   fMatraTags;
    gboolean fMatraWordStart;
    glong    fMPreOutIndex;

    MPreFixups *fMPreFixups;
};

typedef struct _Output Output;

static void initOutput(Output *output, const glong *originalOffsets, gunichar *outChars, glong *charIndices, gulong *charTags, MPreFixups *mpreFixups)
{
    output->fOriginalOffsets = originalOffsets;

    output->fOutChars    = outChars;
    output->fCharIndices = charIndices;
    output->fCharTags    = charTags;

    output->fOutIndex    = 0;
    output->fMatraTags   = 0;

    output->fMpre = output->fMbelow = output->fMabove = output->fMpost = output->fLengthMark = output->fAlLakuna = 0;

    output->fMPreOutIndex = -1;
    output->fMPreFixups = mpreFixups;
}

static void saveMatra(Output *output, gunichar matra, IndicOTCharClass matraClass)
{
    /* FIXME: check if already set, or if not a matra... */
    if (IS_M_PRE(matraClass)) {
	output->fMpre = matra;
    } else if (IS_M_BELOW(matraClass)) {
	output->fMbelow = matra;
    } else if (IS_M_ABOVE(matraClass)) {
	output->fMabove = matra;
    } else if (IS_M_POST(matraClass)) {
	output->fMpost = matra;
    } else if (IS_LENGTH_MARK(matraClass)) {
	output->fLengthMark = matra;
    } else if (IS_AL_LAKUNA(matraClass)) {
	output->fAlLakuna = matra;
    }
}

static void initMatra(Output *output, guint32 matraIndex, gulong matraTags, gboolean wordStart)
{
    output->fMpre = output->fMbelow = output->fMabove = output->fMpost = output->fLengthMark = output->fAlLakuna = 0;
    output->fMPreOutIndex = -1;
    output->fMatraIndex = matraIndex;
    output->fMatraTags = matraTags;
    output->fMatraWordStart = wordStart;
}

static gboolean noteMatra(Output *output, const IndicOTClassTable *classTable, gunichar matra)
{
    IndicOTCharClass matraClass = indic_ot_get_char_class(classTable, matra);

    if (IS_MATRA(matraClass)) {
	if (IS_SPLIT_MATRA(matraClass)) {
	    const IndicOTSplitMatra *splitMatra = indic_ot_get_split_matra(classTable, matraClass);
	    int i;

	    for (i = 0; i < 3 && (*splitMatra)[i] != 0; i += 1) {
		gunichar piece = (*splitMatra)[i];
		IndicOTCharClass pieceClass = indic_ot_get_char_class(classTable, piece);

		saveMatra(output, piece, pieceClass);
	    }
	} else {
	    saveMatra(output, matra, matraClass);
	}

	return TRUE;
    } else
      return FALSE;
}

static void noteBaseConsonant(Output *output)
{
    if (output->fMPreFixups && output->fMPreOutIndex >= 0) {
	indic_mprefixups_add(output->fMPreFixups, output->fOutIndex, output->fMPreOutIndex);
    }
}

static void writeChar(Output *output, gunichar ch, guint32 charIndex, gulong charTags)
{
    if (output->fOutChars != NULL) {
	output->fOutChars[output->fOutIndex]    = ch;
	output->fCharIndices[output->fOutIndex] = output->fOriginalOffsets[charIndex];
	output->fCharTags[output->fOutIndex]    = charTags;
    }

    output->fOutIndex += 1;
}

static void writeMpre(Output *output)
{
    if (output->fMpre != 0) {
	gulong tags = output->fMatraTags;
	if (output->fMatraWordStart)
	    tags &= ~init;

	output->fMPreOutIndex = output->fOutIndex;
	writeChar(output, output->fMpre, output->fMatraIndex, tags);
    }
}

static void writeMbelow(Output *output)
{
    if (output->fMbelow != 0) {
	writeChar(output, output->fMbelow, output->fMatraIndex, output->fMatraTags);
    }
}

static void writeMabove(Output *output)
{
    if (output->fMabove != 0) {
	writeChar(output, output->fMabove, output->fMatraIndex, output->fMatraTags);
    }
}

static void writeMpost(Output *output)
{
    if (output->fMpost != 0) {
	writeChar(output, output->fMpost, output->fMatraIndex, output->fMatraTags);
    }
}

static void writeLengthMark(Output *output)
{
    if (output->fLengthMark != 0) {
	writeChar(output, output->fLengthMark, output->fMatraIndex, output->fMatraTags);
    }
}

static void writeAlLakuna(Output *output)
{
    if (output->fAlLakuna != 0) {
	writeChar(output, output->fAlLakuna, output->fMatraIndex, output->fMatraTags);
    }
}

static glong getOutputIndex(Output *output)
{
    return output->fOutIndex;
}

#define false 0
#define true  1

glong indic_ot_reorder(const gunichar *chars, const glong *utf8_offsets, glong char_count, const IndicOTClassTable *class_table, gunichar *out_chars, glong *char_indices, gulong *char_tags, MPreFixups **outMPreFixups)
{
    MPreFixups *mpreFixups = NULL;
    Output output;
    glong i, prev = 0;
    gboolean last_in_word = FALSE;

    if (outMPreFixups && (class_table->scriptFlags & SF_MPRE_FIXUP)) {
	mpreFixups = indic_mprefixups_new (char_count);
    }

    initOutput(&output, utf8_offsets, out_chars, char_indices, char_tags, mpreFixups);

    while (prev < char_count) {
	glong syllable = indic_ot_find_syllable(class_table, chars, prev, char_count);
	glong matra, vmabove, vmpost = syllable;

	while (vmpost > prev && indic_ot_is_vm_post(class_table, chars[vmpost - 1])) {
	    vmpost -= 1;
	}

	vmabove = vmpost;
	while (vmabove > prev && indic_ot_is_vm_above(class_table, chars[vmabove - 1])) {
	    vmabove -= 1;
	}

	matra = vmabove - 1;
	initMatra(&output, prev, blwf_p, !last_in_word);
	while (noteMatra(&output, class_table, chars[matra]) &&
	       matra != prev)
	    matra--;

	last_in_word = TRUE;
	switch (indic_ot_get_char_class(class_table, chars[prev]) & CF_CLASS_MASK) {
	case CC_RESERVED:
	    last_in_word = FALSE;
	    /* Fall through */
	case CC_INDEPENDENT_VOWEL:
	case CC_ZERO_WIDTH_MARK:
	    for (i = prev; i < syllable; i += 1) {
		writeChar(&output, chars[i], /*i*/ prev, blwf_p);
	    }

	    break;

	case CC_MODIFYING_MARK_ABOVE:
	case CC_MODIFYING_MARK_POST:
	case CC_NUKTA:
	case CC_VIRAMA:
	case CC_AL_LAKUNA:
	    /* patch for rendering fix for Malayalam SAMVRUTHOKARA by suresh */
	    if (chars[prev - 1] == 0x0D41) {
	       	 writeChar(&output, chars[prev], prev, blwf_p);
		 break;
	    }
	    /* end patch */
	    writeChar(&output, C_DOTTED_CIRCLE, prev, blwf_p);
	    writeChar(&output, chars[prev], prev, blwf_p);
	    break;

	case CC_DEPENDENT_VOWEL:
	    writeMpre(&output);
	    writeChar(&output, C_DOTTED_CIRCLE, prev, blwf_p);
	    writeMbelow(&output);
	    writeMabove(&output);
	    writeMpost(&output);
	    writeLengthMark(&output);
	    writeAlLakuna(&output);
	    break;

	case CC_CONSONANT:
	case CC_CONSONANT_WITH_NUKTA:
	{
	    guint32 length = vmabove - prev;
	    glong lastConsonant = vmabove - 1;
	    glong baseLimit = prev;
	    glong baseConsonant, postBase, postBaseLimit;
	    gboolean seenVattu, seenBelowBaseForm, supressVattu;
	    glong bcSpan;

	    /* Check for REPH at front of syllable */
	    if (length > 2 && indic_ot_is_reph(class_table, chars[prev]) && indic_ot_is_virama(class_table, chars[prev + 1])) {
		baseLimit += 2;

		/* Check for eyelash RA, if the script supports it */
		if ((class_table->scriptFlags & SF_EYELASH_RA) != 0 &&
		    chars[baseLimit] == C_SIGN_ZWJ) {
		    if (length > 3) {
			baseLimit += 1;
		    } else {
			baseLimit -= 2;
		    }
		}
	    }

	    while (lastConsonant > baseLimit && !indic_ot_is_consonant(class_table, chars[lastConsonant])) {
		lastConsonant -= 1;
	    }

	    baseConsonant = lastConsonant;
	    postBase = lastConsonant + 1;

	    postBaseLimit = class_table->scriptFlags & SF_POST_BASE_LIMIT_MASK;
	    seenVattu = false;
	    seenBelowBaseForm = false;
	    supressVattu = true;

	    while (baseConsonant > baseLimit) {
		IndicOTCharClass charClass = indic_ot_get_char_class(class_table, chars[baseConsonant]);

		if (IS_CONSONANT(charClass)) {
		    if (postBaseLimit == 0 || seenVattu ||
			(baseConsonant > baseLimit && !indic_ot_is_virama(class_table, chars[baseConsonant - 1])) ||
			!HAS_POST_OR_BELOW_BASE_FORM(charClass)) {
			break;
		    }

		    seenVattu = IS_VATTU(charClass);

		    if (HAS_POST_BASE_FORM(charClass)) {
			if (seenBelowBaseForm) {
			    break;
			}

			postBase = baseConsonant;
		    } else if (HAS_BELOW_BASE_FORM(charClass)) {
			seenBelowBaseForm = true;
		    }

		    postBaseLimit -= 1;
		}

		baseConsonant -= 1;
	    }

	    /* Write Mpre */
	    writeMpre(&output);

	    /* Write eyelash RA */
	    /* NOTE: baseLimit == prev + 3 iff eyelash RA present... */
	    if (baseLimit == prev + 3) {
		writeChar(&output, chars[prev], prev, half_p);
		writeChar(&output, chars[prev + 1], prev /*+ 1*/, half_p);
		writeChar(&output, chars[prev + 2], prev /*+ 2*/, half_p);
	    }

	    /* write any pre-base consonants */
	    supressVattu = true;

	    for (i = baseLimit; i < baseConsonant; i += 1) {
		gunichar ch = chars[i];
		/* Applying blwf to the first consonant doesn't makes sense
		 * since the below-form follows the consonant that it is
		 * put under */
		gulong tag = (i == baseLimit) ? half_p : blwf_p;
		IndicOTCharClass charClass = indic_ot_get_char_class(class_table, ch);

		if (IS_CONSONANT(charClass)) {
		    if (IS_VATTU(charClass) && supressVattu) {
			tag = nukt_p;
		    }
		    else if ((i + 2 < baseConsonant) && (chars[i + 2] == C_SIGN_ZWNJ)) {
			tag = nukt_p;
		    }

		    supressVattu = IS_VATTU(charClass);
		} else if (IS_VIRAMA(charClass) && chars[i + 1] == C_SIGN_ZWNJ)
		{
		    tag = nukt_p;
		}

		writeChar(&output, ch, /*i*/ prev, tag);
	    }

	    bcSpan = baseConsonant + 1;

	    if (bcSpan < vmabove && indic_ot_is_nukta(class_table, chars[bcSpan])) {
		bcSpan += 1;
	    }

	    if (baseConsonant == lastConsonant && bcSpan < vmabove && indic_ot_is_virama(class_table, chars[bcSpan])) {
		bcSpan += 1;

		if (bcSpan < vmabove && chars[bcSpan] == C_SIGN_ZWNJ) {
		    bcSpan += 1;
		}
	    }

	    /* note the base consonant for post-GSUB fixups */
	    noteBaseConsonant(&output);

	    /* write base consonant */
	    for (i = baseConsonant; i < bcSpan; i += 1) {
		writeChar(&output, chars[i], /*i*/ prev, nukt_p);
	    }

	    if ((class_table->scriptFlags & SF_MATRAS_AFTER_BASE) != 0) {
		gboolean is_for_0C48 = FALSE;
		if (output.fOutChars != NULL) {  /*for 0x0C48 of Telugu*/
		    int t;
		    for (t = prev; t < syllable; t++) {
			if (chars[t] == 0x0C48) {
			    writeMabove(&output);
			    writeMbelow(&output);
			    writeMpost(&output);

			    is_for_0C48 = TRUE;
			    break;
			}
		    }
		}

		if (!is_for_0C48) {
		    writeMbelow(&output);
		    writeMabove(&output);
		    writeMpost(&output);
		}
	    }

	    /* write below-base consonants */
	    if (baseConsonant != lastConsonant) {
		for (i = bcSpan + 1; i < postBase; i += 1) {
		    writeChar(&output, chars[i], /*i*/ prev, blwf_p);
		}

		if (postBase > lastConsonant) {
		    /* write halant that was after base consonant */
		    writeChar(&output, chars[bcSpan], /*bcSpan*/ prev, blwf_p);
		}
	    }

	    /* write Mbelow, Mabove */
	    if ((class_table->scriptFlags & SF_MATRAS_AFTER_BASE) == 0) {
		writeMbelow(&output);
		writeMabove(&output);
	    }

	   if ((class_table->scriptFlags & SF_REPH_AFTER_BELOW) != 0) {
		if (baseLimit == prev + 2) {
		    writeChar(&output, chars[prev], prev, rphf_p);
		    writeChar(&output, chars[prev + 1], prev /*+ 1*/, rphf_p);
		}

		/* write VMabove */
		for (i = vmabove; i < vmpost; i += 1) {
		    writeChar(&output, chars[i], /*i*/ prev, blwf_p);
		}
	    }

	    /* write post-base consonants */
	    if (baseConsonant != lastConsonant) {
		if (postBase <= lastConsonant) {
		    for (i = postBase; i <= lastConsonant; i += 1) {
			writeChar(&output, chars[i], /*i*/ prev, pstf_p);
		    }

		    /* write halant that was after base consonant */
		    writeChar(&output, chars[bcSpan], /*bcSpan*/ prev, blwf_p);
		}

		/* write the training halant, if there is one */
		if (lastConsonant < matra && indic_ot_is_virama(class_table, chars[matra])) {
		    writeChar(&output, chars[matra], /*matra*/ prev, nukt_p);
		}
	    }

	    /* write Mpost */
	    if ((class_table->scriptFlags & SF_MATRAS_AFTER_BASE) == 0) {
		writeMpost(&output);
	    }

	    writeLengthMark(&output);
	    writeAlLakuna(&output);

	    /* write reph */
	    if ((class_table->scriptFlags & SF_REPH_AFTER_BELOW) == 0) {
		if (baseLimit == prev + 2) {
		    writeChar(&output, chars[prev], prev, rphf_p);
		    writeChar(&output, chars[prev + 1], prev /*+ 1*/, rphf_p);
		}

		/* write VMabove */
		for (i = vmabove; i < vmpost; i += 1) {
		    writeChar(&output, chars[i], /*i*/ prev, blwf_p);
		}
	    }

	    /* write VMpost */
	    for (i = vmpost; i < syllable; i += 1) {
		writeChar(&output, chars[i], /*i*/ prev, blwf_p);
	    }

	    break;
	}

	default:
	    break;
	}


	prev = syllable;
    }

    if (outMPreFixups) {
	*outMPreFixups = mpreFixups;
    }

    return getOutputIndex(&output);
}
