#include "config.h"
#include "viewer.h"

extern const PangoViewer pangocairo_viewer;
extern const PangoViewer pangoxft_viewer;
extern const PangoViewer pangoft2_viewer;
extern const PangoViewer pangox_viewer;

const PangoViewer *viewers[] = {
#ifdef HAVE_CAIRO_XLIB
  &pangocairo_viewer,
#endif
#ifdef HAVE_XFT
  &pangoxft_viewer,
#endif
#ifdef HAVE_FREETYPE
  &pangoft2_viewer,
#endif
#ifdef HAVE_X
  &pangox_viewer,
#endif
  NULL
};
