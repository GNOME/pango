#include "viewer.h"

extern const PangoViewer pangocairo_viewer;

const PangoViewer *viewers[] = {
  &pangocairo_viewer,
  NULL
};
