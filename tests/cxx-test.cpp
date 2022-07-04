/* This test makes sure that all Pango headers can be included
 * and compiled in a C++ program.
 */

#include <pango2/pango.h>

#if PANGO_RENDERING_CAIRO
#include <pango2/pangocairo.h>
#endif

int
main ()
{
  return 0;
}
