/* This test makes sure that all Pango headers can be included
 * and compiled in a C++ program.
 */

#include <pango/pango.h>

#ifdef HAVE_WIN32
#include <pango/pangowin32.h>
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
