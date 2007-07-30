#include "viewer.h"

extern const PangoViewer pangoxft_viewer;

const PangoViewer *viewers[] = {
  &pangoxft_viewer,
  NULL
};
