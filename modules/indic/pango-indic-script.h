#ifndef __INDIC_SCRIPT__
#define __INDIC_SCRIPT__

#define RANGE_END (RANGE_START + RANGE_SIZE - 1)

#ifdef ISCII_BASED
#define VIRAMA  (0x4d + RANGE_START)
#define CANDRA  (0x01 + RANGE_START)
#define ANUSWAR (0x02 + RANGE_START)
#define NUKTA   (0x3c + RANGE_START)
#define RA      (0x30 + RANGE_START)
#endif

#define SCRIPT_ENGINE_DEFINITION \
  static PangoEngineInfo script_engines[] = \
  { \
    { \
      SCRIPT_STRING "ScriptEngineX", \
      PANGO_ENGINE_TYPE_SHAPE, \
      PANGO_RENDER_TYPE_X, \
      pango_indic_range, G_N_ELEMENTS (pango_indic_range)} \
  }; \
  static gint n_script_engines = G_N_ELEMENTS (script_engines);

#define pango_indic_get_char(chars,end) ( (chars) >= (end) ? 0 : *(chars) )

#endif
