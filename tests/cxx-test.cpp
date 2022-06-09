/* This test makes sure that all Pango headers can be included
 * and compiled in a C++ program.
 */

#include <pango/pango.h>

#ifdef HAVE_CAIRO
#include <pango/pangocairo.h>
#endif

int
main ()
{
  return 0;
}
