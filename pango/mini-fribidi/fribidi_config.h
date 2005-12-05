#include <glib.h>

#define FRIBIDI_TRUE    TRUE
#define FRIBIDI_FALSE   FALSE
#define HAS_FRIBIDI_TAB_CHAR_TYPE_2_I 1
#define FRIBIDI_API

/* this was in fribidi_unicode.h.  we only need these bits from that
 * file, so moved here. */
#define UNI_MAX_BIDI_LEVEL 61

/* ripped off debugging functions, make sure it's not triggerred. */
#undef DEBUG

/* g_malloc and g_free verbatim */
#define malloc g_malloc
#define free g_free

/* rename symbols to pango internal namespace */
#define fribidi_log2vis_get_embedding_levels_new_utf8 _pango_fribidi_log2vis_get_embedding_levels_new_utf8
#define fribidi_prop_to_type _pango_fribidi_prop_to_type
#define fribidi_get_type _pango_fribidi_get_type
#define fribidi_get_type_internal fribidi_get_type

