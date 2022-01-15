#pragma once

#include "pango-lines.h"

struct _PangoLines
{
  GObject parent_instance;

  GArray *lines;
  guint serial;
};
