#pragma once

#include "pango-lines.h"

typedef struct _Position Position;
struct _Position
{
  int x, y;
};

struct _PangoLines
{
  GObject parent_instance;

  GPtrArray *lines;
  GArray *positions;
  guint serial;
};
