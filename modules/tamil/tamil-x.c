/* Pango
 * tamil-x.c:
 *
 * Authors: Sivaraj D (sivaraj@tamil.net)
 *          Vikram Subramanian (upender@vsnl.com)
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

/* A version using Unicode based font with ligature information in it 
 *
 * Vikram Subramanian <upender@vsnl.com>
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "pango-engine.h"
#include "pangox.h"
#include "tadefs.h"

#define SCRIPT_ENGINE_NAME "TamilScriptEngineX"


/* The charset for the font */
static char *default_charset = "iso10646-tam";

/* Bitmap to test whether a char is a consonant     */
/** Remember to change this when adding KSHA later **/
static const char cons_map[] = {0xB1, 0xC6, 0x38, 0xFE, 0x1D};

static PangoEngineRange tamil_range[] = {
  { 0x0b80, 0x0bff, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
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


/* Remove unwanted U+0000 */
void
tamil_compact (gunichar         *chars, 
	       int              *num,                      
	       int              *cluster)
{
  gunichar *dest = chars;
  gunichar *end = chars + *num;
  int *cluster_dest = cluster;
  while (chars < end)
    {
      if (*chars)
	{
	  *dest = *chars;
	  *cluster_dest = *cluster;
	  dest++;
	  chars++;
	  cluster++;
	  cluster_dest++;
	}
      else
	{
	  chars++;
	  cluster++;
	}
    }
  *num -= (chars - dest);
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
  if ((modifier >= U_KAAL && modifier <= U_UMODI2) ||
      (modifier == U_PULLI))
    {        
      glyph_str[0] = base;
      glyph_str[1] = modifier;
      *num_glyphs  = 2;
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
 * X window system script engine portion
 */

/* Find a font of the reqd. charset
 */
static PangoXSubfont
find_tamil_font (PangoFont *font)
{
  PangoXSubfont result;
  
  if (pango_x_find_first_subfont (font, &default_charset, 1, &result))
    return result;
  else
    return 0; /* Could not find a font */
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
  int i;
  const char *cluster_start;
  const char *p;
  gunichar *wc, prevchar;
  gunichar currchar = 0; /* Quiet compiler */
  int complete; /* Whether the prev char is gauranteed to be complete 
		   i.e not modified by modifiers */
  int nuni;     /* No. of unicode characters in a cluster */
  
  PangoXSubfont tamil_font;
  
  
  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);
  
  n_chars = g_utf8_strlen (text, length);
  
  tamil_font = find_tamil_font (font);
  if (!tamil_font) 
    {
      pango_x_fallback_shape (font, glyphs, text, n_chars);
      return;
    }

  /* temporarily set the size to 2 times the number of unicode chars */
  pango_glyph_string_set_size (glyphs, n_chars * 2);
  wc = (gunichar *)g_malloc (sizeof(gunichar) * n_chars * 2); 
  
  p = text;
  prevchar = 0;complete = 1;/* One character look behind */
  n_glyph = 0;
  cluster_start = text;

  /* The following loop is similar to the indic engine's split out chars */ 
  for (i=0; i < n_chars; i++) {
    
    currchar = g_utf8_get_char (p);
    
    /* Two classes - Modifiers and Non-Modifiers */
    if (is_modifier (currchar))
      {      
	if (complete){ /* The previous char has been tagged complete already */
	
	  /* Leave the vowel sign itself in wc */
	  /* Useful in lessons                 */
	  wc[n_glyph] = currchar;
	  glyphs->log_clusters[n_glyph] = cluster_start - text;
	  
	  n_glyph++;
	  nuni = 1;	
	}
	else
	  { /* Modify the previous char */  
	    int num_glyphs;
	    gunichar glyph_str[3];
	    int k;
	
	    /* Modify the previous char and get a glyph string */
	    apply_modifier (prevchar,currchar,glyph_str,&num_glyphs);
	    
	    for (k = 0;k < num_glyphs;k++)
	      {	  	      
		wc[n_glyph] = glyph_str[k];
		glyphs->log_clusters[n_glyph] = cluster_start - text;
		
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
	if (!complete)
	  {	
	    wc[n_glyph] = prevchar;
	    glyphs->log_clusters[n_glyph] = cluster_start - text;
	    
	    n_glyph++;
	    
	    /* Set the cluster start properly for the current char */
	    cluster_start = g_utf8_next_char (cluster_start);	
	  }
	
	/* Check the type of the current char further */
	if (is_consonant (currchar))
	  {			
	    prevchar = currchar; /* Store this consonant         */
	    complete = 0;        /* May get modified             */
	    nuni     = 0;        /* NO unicode character written */
	    
	  }
	else 
	  {	  
	    /* Vowels, numerals and other unhandled stuff come here */ 
	    
	    /* Write it out then and there */
	    wc[n_glyph] = currchar;
	    glyphs->log_clusters[n_glyph] = cluster_start - text;
	    
	    n_glyph++;
	    nuni = 1;
	    
	    complete = 1;	/* A character has ended */  		
	  }
      }
    
    /* Set the beginning for the next cluster */
    while (nuni-- > 0)
      cluster_start = g_utf8_next_char (cluster_start);
    
    p = g_utf8_next_char (p);
  }
  
  /* Flush out the last char if waiting to get completed */
  if (!complete)
    {
      wc[n_glyph] = currchar;
      glyphs->log_clusters[n_glyph] = cluster_start - text;
      
      n_glyph++;    
    }
  
  /* Apply ligatures as specified in the X Font */
  pango_x_apply_ligatures (font, tamil_font, &wc, &n_glyph, 
			   &glyphs->log_clusters);
  
  /* Remove unwanted U+0000 */
  tamil_compact (wc, &n_glyph, glyphs->log_clusters);
  
  /* Make glyphs */
  pango_glyph_string_set_size (glyphs, n_glyph);
  
  for (i = 0;i < n_glyph;i++) 
    {  
      PangoRectangle logical_rect;
      glyphs->glyphs[i].glyph = PANGO_X_MAKE_GLYPH (tamil_font, wc[i]);
      pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph,
				    NULL, &logical_rect);
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = logical_rect.width;      
    }
  
  g_free(wc);
}

static PangoCoverage *
tamil_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  
  PangoXSubfont tamil_font = find_tamil_font (font);
  if (tamil_font)
    {
      gunichar i;
      
      for (i = 0xb80; i <= 0xbff; i++)
	pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);
    }
  
  return result;
}


static PangoEngine *
tamil_engine_x_new ()
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

#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_tamil_x_##func
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
    return tamil_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
