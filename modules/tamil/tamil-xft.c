/* Pango
 * tamil-xft.c:
 *
 * Author: Vikram Subramanian (upender@vsnl.com)
 *         
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

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "pangoxft.h"
#include "pango-engine.h"
#include "pango-utils.h"
#include "tadefs.h"

#define SCRIPT_ENGINE_NAME "TamilScriptEngineXft"


/* Bitmap to test whether a char is a consonant     */
/** Remember to change this when adding KSHA later **/
static const char cons_map[] = {0xB1, 0xC6, 0x38, 0xFE, 0x1D};


/* Start of the seperate ligature block in the PUA */
#define LIG_BLOCK_START 0xEB80


/* An array giving the start of the ligature block for a given vowel
 * modifier.Defined for KOKKI1,KOKKI2,U_UMODI1,U_UMODI2,U_PULLI
 * 
 * First element corresponds to U_KOKKI1
 *
 * The starting positions are given as offsets from LIG_BLOCK_START
 */
static const signed char modifier_block_pos[] = {
  0x00, /* U_KOKKI1  */
  0x18, /* U_KOKKI2  */
  0x30, /* U_UMODI1  */
  0x48, /* U_UMODI2  */
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* Undefined */
  0x60  /* U_PULLI   */
};

/* An array giving the offset corresponding to the base consonant in the
 * ligature block for a given vowel modifier (Clear?)
 *
 * Example:NGA + KOKKI1 comes as the second char in the ligature block for 
 *         KOKKI1
 *         (U_NGA - U_KA) = 4.Therefore cons_pos[4] = (2 - 1) = 1 
 */
static const signed char cons_pos[] = { 0,-1,-1,-1, 1, 2,-1, 3,-1, 4, 5,
				       -1,-1,-1, 6, 7,-1,-1,-1, 8, 9,10,
				       -1,-1,-1,11,12,13,14,15,16,17,18,
				       -1,20,21,22};

static PangoEngineRange tamil_range[] = {
  { 0x0b80, 0x0bff, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_XFT,
    tamil_range, G_N_ELEMENTS(tamil_range)
  }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);


/* Return non-zero if c is a akara mey, 0 otherwise
 */
static int
is_consonant (unsigned int c)
{
  /* Too many gaps between consonants - So use a bit map */
  /** KSHA might come at 0xBBA later ?? **/
  return ((c >= 0x0B95 && c <= 0x0BB9) &&
	  (cons_map[(c - 0x0B95) >> 3] & (1 << ((c - 0xB95) & 7))));
}

/* Return 1 if c is a modifier, 0 otherwise
 */
static int
is_modifier (unsigned int c)
{
  if ((c >= 0x0BBE && c <= 0x0BC2) ||
      (c >= 0x0BC6 && c <= 0x0BC8) ||
      (c >= 0x0BCA && c <= 0x0BCD) ||
      (c == 0x0BD7))
    return 1;
  else 
    return 0;
}


/* Apply the modifier to the base character to get the string of glyph
 * indices 
 */
static void
apply_modifier (gunichar base,
		gunichar modifier,
		gunichar *glyph_str,
	       int *num_glyphs)
{
  
  /* Modifier which appears as suffix */
  if (modifier == U_KAAL)
    {
      glyph_str[0] = base;
      glyph_str[1] = U_KAAL;
      *num_glyphs  = 2;
      return;
    }
  
  /* Modifiers which produce completely new glyphs */
  if ((modifier >= U_KOKKI1 && modifier <= U_UMODI2) ||
      (modifier == U_PULLI))
    {
      /* modifier_block_pos and cons_pos are global variables */
      glyph_str[0] = (LIG_BLOCK_START + 
		      modifier_block_pos[modifier - U_KOKKI1] +
		      cons_pos[base - U_KA]);
    
      *num_glyphs  = 1;
      return;
    }
  
  /* Modifiers which appear as prefix */
  if (modifier >= U_KOMBU1 && modifier <= U_AIMODI)
    {
      glyph_str[0] = modifier;
      glyph_str[1] = base;
      *num_glyphs  = 2;
      return;
    }
  
  /* Modifiers appearing as both suffix and prefix */
  if (modifier == U_OMODI1)
    {
      glyph_str[0] = U_KOMBU1; 
      glyph_str[1] = base;
      glyph_str[2] = U_KAAL;
      *num_glyphs  = 3;
      return;
    }
  
  if (modifier == U_OMODI2)
    {    
      glyph_str[0] = U_KOMBU2; 
      glyph_str[1] = base;
      glyph_str[2] = U_KAAL;
      *num_glyphs  = 3;
      return;
    }
  
  if (modifier == U_AUMODI)
    {
      glyph_str[0] = U_KOMBU1; 
      glyph_str[1] = base;
      glyph_str[2] = U_AUMARK;
      *num_glyphs  = 3;
      return;
    }
  
  /* U_AUMARK after a consonant?? */
  glyph_str[0] = base;
  *num_glyphs = 1;
}


/*
 * Xft script engine portion
 */

static guint
find_char (FT_Face face, PangoFont *font, gunichar wc)
{
  int index = FT_Get_Char_Index (face, wc);

  if (index && index <= face->num_glyphs)
    return index;
  else
    return 0;
}


/* Fills in the attributes of the ith glyph in the glyph string 
 */
static void
set_glyph (PangoFont *font, FT_Face face,PangoGlyphString *glyphs, int i, 
	   int offset, PangoGlyph glyph)
{
  PangoRectangle logical_rect;
  PangoGlyph index;

  index = find_char (face, font, glyph);

  if (index)
    glyphs->glyphs[i].glyph = index;
  else
    glyphs->glyphs[i].glyph = pango_xft_font_get_unknown_glyph (font, glyph);
  

  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;
  
  glyphs->log_clusters[i] = offset;
  
  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}



/* Convert UTF-8 to glyph string
 */
static void 
tamil_engine_shape (PangoFont        *font,
		    const char       *text,
		    int               length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars, n_glyph;
  int i, j;
  const char *cluster_start;
  const char *p;
  gunichar *wc, prevchar;  
  int complete; /* Whether the prev char is gauranteed to be complete 
		   i.e not modified by modifiers */
  
  int nuni;     /* No. of unicode characters in a cluster */
  FT_Face face;

  
  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);
  
  
  face = pango_xft_font_get_face (font);
  g_assert (face);
  
  /* temporarily set the size to 3 times the number of unicode chars */
  n_chars = g_utf8_strlen (text, length);  
  pango_glyph_string_set_size (glyphs, n_chars * 3);
  
  wc = (gunichar *)g_malloc(sizeof(gunichar) * n_chars);  
  p = text;
  for (i=0; i < n_chars; i++)
    {
      wc[i] = g_utf8_get_char (p);
      p = g_utf8_next_char (p);
    }

  
  /* Convertion starts here */
  prevchar = 0;complete = 1;/* One character look behind */
  n_glyph = 0;
  cluster_start = text;  
  for (j = 0;j < n_chars;j++)
    {
      /* Two classes - Modifiers and Non-Modifiers */
      if (is_modifier (wc[j]))
	{
	  if (complete)
	    { /* The previous char has been tagged complete already */
	
	      /* Write the vowel sign itself */
	      /* Useful in lessons           */
	      set_glyph (font, face, glyphs, n_glyph, 
			 cluster_start - text, wc[j]);
	      
	      n_glyph++;
	      nuni = 1;
	      
	    }
	  else
	    { /* Modify the previous char */  
	      int num_glyphs;
	      gunichar glyph_str[3];
	      int k;
	
	      /* Modify the previous char and get a glyph string */
	      apply_modifier (prevchar,wc[j],glyph_str,&num_glyphs);
	      
	      for (k = 0;k < num_glyphs;k++)
		{	  
		  set_glyph (font, face, glyphs, n_glyph, 
			     cluster_start - text,glyph_str[k]);
		  
		  n_glyph++;
		}	
	      
	      /* 2 unicode chars in this just written cluster */
	      nuni = 2;
	    }     
	  complete = 1; /* A character has ended */
	  
	  /* NOTE : Double modifiers are not handled but the display will be 
	   * correct since the vowel sign is appended.However cluster info
	   * will be wrong.
	   */
	  
	}
      else 
	{ /* Non-modifiers */
	  
	  /* Write out the previous char which is waiting to get completed */
	  if (!complete){
	    set_glyph (font, face, glyphs, n_glyph, cluster_start - text, 
		       prevchar);
	    
	    n_glyph++;
	    
	    /* Set the cluster start properly for the current char */
	    cluster_start = g_utf8_next_char (cluster_start);	
	  }
	  
	  /* Check the type of the current char further */
	  if (is_consonant(wc[j]))
	    {		
	      prevchar = wc[j]; /* Store this consonant         */
	      complete = 0;     /* May get modified             */
	      nuni     = 0;     /* NO unicode character written */
	      
	    }
	  else
	    {
	      /* Write it out then and there */
	      set_glyph (font, face, glyphs, n_glyph, cluster_start - text, 
			 wc[j]);
	      
	      n_glyph++;
	      nuni = 1;
	      
	      complete = 1;	/* A character has ended */  	
	      
	    }
	}

      /* Set the beginning for the next cluster */
      while (nuni-- > 0)
	cluster_start = g_utf8_next_char (cluster_start);
      
    } /* for */ 
  
  
  /* Flush out the last char if waiting to get completed */
  if (!complete)
    {    
      set_glyph (font, face, glyphs, n_glyph, cluster_start - text, 
		 prevchar);
      n_glyph++;    
    }
  
  /* Set the correct size for the glyph string */
  pango_glyph_string_set_size (glyphs, n_glyph);
  g_free(wc);
}

static PangoCoverage *
tamil_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  return pango_font_get_coverage (font, lang);
}


static PangoEngine *
tamil_engine_xft_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = tamil_engine_shape;
  result->get_coverage = tamil_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango
 */
#ifdef XFT_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_tamil_xft_##func
#else
#define MODULE_ENTRY(func) func
#endif


void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines, int *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
}

PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return tamil_engine_xft_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

