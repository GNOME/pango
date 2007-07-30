/* This test makes sure that all Pango headers can be included
 * and compiled in a C++ program.
 */
#define PANGO_ENABLE_ENGINE
#define PANGO_ENABLE_BACKEND

#include <pango/pango.h>

#ifdef HAVE_WIN32
#include <pango/pangowin32.h>
#endif

#ifdef HAVE_X
#include <pango/pangox.h>
#endif

#ifdef HAVE_XFT
#include <pango/pangoxft.h>
#endif

#ifdef HAVE_FREETYPE
#include <pango/pangoft2.h>
#endif

#ifdef HAVE_CAIRO
#include <pango/pangocairo.h>
#endif

int
main ()
{
  return 0;
}
