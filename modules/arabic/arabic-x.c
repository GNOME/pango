/* Pango - Arabic module 
 * arabic module
 *
 * (C) 2000 Karl Koehler<koehler@or.uni-bonn.de>
 *          Owen Taylor <otaylor@redhat.com> 
 * 
 */

#include <stdio.h>
#include <glib.h>
#include <string.h>
#include "pango-engine.h"
#include "pangox.h"

#include "arconv.h" 
#include "mulefont.h"
#include "langboxfont.h"
#include "naqshfont.h"

/*  #define DEBUG       */
#ifdef DEBUG
#include <stdio.h>
#endif

#define SCRIPT_ENGINE_NAME "ArabicScriptEngineX" 

static PangoEngineRange arabic_range[] = {
    { 0x060B, 0x06D3, "*" }  /* 0x060B, 0x06D3 */
};

static PangoEngineInfo script_engines[] = {
    {
        SCRIPT_ENGINE_NAME,
        PANGO_ENGINE_TYPE_SHAPE,
        PANGO_RENDER_TYPE_X,
        arabic_range, G_N_ELEMENTS(arabic_range)
    }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);

/*
 * X window system script engine portion
 */

static ArabicFontInfo*
arabic_unicodeinit(PangoFont  *font, PangoXSubfont subfont)
{ 
    ArabicFontInfo    *fs = NULL;

    if (subfont != 0)
        {
            if ( pango_x_has_glyph /* Alif-Madda */
                 (font,PANGO_X_MAKE_GLYPH(subfont,0xFE81)))
                {
                    fs              = g_new (ArabicFontInfo,1);
                    fs->level       = ar_standard | ar_unifont;
                    fs->subfonts[0] = subfont;

                    if ( pango_x_has_glyph /* Shadda+Kasra */
                         (font,PANGO_X_MAKE_GLYPH(subfont,0xFC62)))
                        {
                            fs->level   |= ar_composedtashkeel;
                            /* extra vowels in font, hopefully */
                        }
                    if ( pango_x_has_glyph /* Lam-Min alone */
                         (font,PANGO_X_MAKE_GLYPH(subfont,0xFC42)))
                        {
                            fs->level |= ar_lig; 
                            /* extra ligatures in font, hopefully */
                        }
                }
        }
    return fs;
}

static ArabicFontInfo*
find_unic_font (PangoFont *font)
{
    static char *charsets[] = {
        "iso10646-1",  
        "iso8859-6.8x",
        "mulearabic-2",
	"urdunaqsh-0",
/*          "symbol-0" */
    };
    
    ArabicFontInfo    *fs = NULL;
    PangoXSubfont     *subfonts;
    int               *subfont_charsets;
    int                n_subfonts;
    int                i;
    
    GQuark info_id = g_quark_from_string ("arabic-font-info");
    fs = g_object_get_qdata (G_OBJECT (font), info_id);
    if (fs) return fs;

    n_subfonts = pango_x_list_subfonts (font, charsets, 4, 
                                        &subfonts, &subfont_charsets);
    
    for (i=0; i < n_subfonts; i++)
        {
            if  ( !strcmp (charsets[subfont_charsets[i]], "mulearabic-2"))
                {
#ifdef DEBUG
                    if (getenv("PANGO_AR_NOMULEFONT") == NULL )
#endif
                        fs = arabic_muleinit(font);
                }
            else if ( !strcmp (charsets[subfont_charsets[i]], "iso8859-6.8x"))
                {
#ifdef DEBUG
                    if (getenv("PANGO_AR_NOLBOXFONT") == NULL )
#endif
                        fs = arabic_lboxinit(font);
                } 
            else if ( !strcmp (charsets[subfont_charsets[i]], "urdunaqsh-0"))
                {
#ifdef DEBUG
                    if (getenv("PANGO_AR_NONQFONT") == NULL )
#endif
                        fs = urdu_naqshinit(font);
                } 
            else
                { 
#ifdef DEBUG
                    if (getenv("PANGO_AR_NOUNIFONT") == NULL )
#endif
                        fs = arabic_unicodeinit(font,subfonts[i]);
                }
            if (fs){ 
                g_object_set_qdata_full (G_OBJECT (font), info_id, 
                                         fs, (GDestroyNotify)g_free);
                break;
            }
        }

    g_free (subfonts);
    g_free (subfont_charsets);

    return fs;
}



static void
set_glyph (PangoGlyphString *glyphs, 
           PangoFont *font, PangoXSubfont subfont,
           int i, int cluster_start, int glyph, int is_vowel)
{
    PangoRectangle logical_rect;

    glyphs->glyphs[i].glyph = PANGO_X_MAKE_GLYPH (subfont, glyph);
  
    glyphs->glyphs[i].geometry.x_offset = 0;
    glyphs->glyphs[i].geometry.y_offset = 0;

    pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
    glyphs->log_clusters[i] = cluster_start;
    if (is_vowel)
        {
            glyphs->glyphs[i].geometry.width = 0;
        }
    else
        {
            glyphs->glyphs[i].geometry.width = logical_rect.width;
        }
}


/* The following thing is actually critical ... */

static void 
arabic_engine_shape (PangoFont        *font, 
                     const char       *text,
                     int               length,
                     PangoAnalysis    *analysis, 
                     PangoGlyphString *glyphs)
{
    PangoXSubfont   subfont;
    long            n_chars;
    int             i;
    ArabicFontInfo *fs;
    const char     *p;
    const char     *pold;
    gunichar       *wc;

    g_return_if_fail (font != NULL);
    g_return_if_fail (text != NULL);
    g_return_if_fail (length >= 0);
    g_return_if_fail (analysis != NULL);

    /* We hope there is a suitible font installed ..
     */
  
    if (! (fs = find_unic_font (font)) )
        {

            PangoGlyph unknown_glyph = pango_x_get_unknown_glyph (font);
            
            n_chars = g_utf8_strlen(text,length);
            pango_glyph_string_set_size (glyphs, n_chars);

            p = text;
            for (i=0; i<n_chars; i++)
                {
                    set_glyph (glyphs, font, 
                               PANGO_X_GLYPH_SUBFONT (unknown_glyph), i, 
                               p - text, PANGO_X_GLYPH_INDEX (unknown_glyph),0);
                    p = g_utf8_next_char (p);
                }
            return;
        }


    p = text;
    if (analysis->level % 2 == 0)
        {
            wc      = g_utf8_to_ucs4_fast (text,length,&n_chars);
            /* We were called on a LTR directional run (e.g. some numbers); 
               fallback as simple as possible */
            pango_glyph_string_set_size (glyphs, n_chars);

        }
    else 
        {
            wc      = (gunichar *)g_malloc(sizeof(gunichar)* (length) ); /* length is succicient: all arabic chars use at
									    least 2 bytes in utf-8 encoding */
            n_chars = length;
            arabic_reshape(&n_chars,text,wc,fs->level);
            pango_glyph_string_set_size (glyphs, n_chars);
        }


    p    = text;
    pold = p;
    i    = 0; 
    subfont = fs->subfonts[0];

    while(i < n_chars)
        {
            if (wc[i] == 0)
                {
                    p = g_utf8_next_char (p);
#ifdef DEBUG
                    fprintf(stderr,"NULL-character detected in generated string.!");
#endif          
                    i++;
                }
            else 
                {   
                    int cluster_start ;
                    int is_vowel      = arabic_isvowel(wc[i]);
                    cluster_start     = is_vowel ? pold - text : p - text;

                    if ( fs->level & ar_mulefont )
                        {
                            arabic_mule_recode(&subfont,&(wc[i]),
                                               fs->subfonts);
                        }
                    else if ( fs->level & ar_lboxfont )
                        {
                            if (( i < n_chars-1 )&&(wc[i+1] == 0))
                                {
                                    arabic_lbox_recode(&subfont,&(wc[i]),
                                                       &(wc[i+1]),
                                                       fs->subfonts);
                                }
                            else 
                                arabic_lbox_recode(&subfont,&(wc[i]),NULL,
                                                   fs->subfonts);
                        }
                    else if ( fs->level & ar_naqshfont )
                        {
                            if (( i < n_chars-1 )&&(wc[i+1] == 0))
                                {
                                    urdu_naqsh_recode(&subfont,&(wc[i]),
                                                      &(wc[i+1]),
                                                      fs->subfonts);
                                }
                            else 
                                urdu_naqsh_recode(&subfont,&(wc[i]),NULL,
                                                  fs->subfonts);
                        }
                    
                    set_glyph(glyphs, font, subfont, n_chars - i - 1,
                              cluster_start, wc[i], is_vowel);
      
                    pold = p;
                    p    = g_utf8_next_char (p);
                    i++;
                }
        }

    g_free(wc);
}


static PangoCoverage *
arabic_engine_get_coverage (PangoFont  *font,
                            PangoLanguage *lang)
{
    gunichar i;
    PangoCoverage *result = pango_coverage_new ();

    for (i = 0x60B; i <= 0x66D; i++)
        pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);
    for (i = 0x670; i <= 0x6D3; i++)
        pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);

    return result;
}

static PangoEngine *
arabic_engine_x_new ()
{
    PangoEngineShape *result;
  
    result = g_new (PangoEngineShape, 1);

    result->engine.id = SCRIPT_ENGINE_NAME;
    result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
    result->engine.length = sizeof (result);
    result->script_shape = arabic_engine_shape;
    result->get_coverage = arabic_engine_get_coverage;

    return (PangoEngine *)result;
}





/* The following three functions provide the public module API for
 * Pango
 */
#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_arabic_x_##func
#else
#define MODULE_ENTRY(func) func
#endif

void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines, int *n_engines)
{
    *engines   = script_engines;
    *n_engines = n_script_engines;
}

PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return arabic_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
