/* This test makes sure that all Pango headers can be included
 * and compiled in a C++ program.
 */

#include <pango2/pango.h>

#if PANGO2_HAS_CAIRO
#include <pango2/pangocairo.h>
#endif

int
main ()
{
  return 0;
}
