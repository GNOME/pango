#include "config.h"
#include "viewer.h"

extern const Pango2Viewer pangocairo_viewer;
extern const Pango2Viewer pangox_viewer;

const Pango2Viewer *viewers[] = {
#ifdef HAVE_CAIRO
  &pangocairo_viewer,
#endif
#ifdef HAVE_X
  &pangox_viewer,
#endif
  NULL
};
