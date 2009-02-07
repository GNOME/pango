#include <glib.h>
#include <stdio.h>

int
main (int argc G_GNUC_UNUSED,
      char **argv G_GNUC_UNUSED)
{
  gunichar i;
  gint j;

  /* Output all characters in the BMP twice, once directly
   * concatenated, once with spaces between them
   */
  for (j = 0 ; j < 2 ; j++)
    {
      for (i = 0; i < 65536; i++)
	{
	  if (g_unichar_validate (i))
	    {
	      gchar buffer[7];
	      int len = g_unichar_to_utf8 (i, buffer);
	      buffer[len] = '\0';

	      if (j == 1)
		fputs (" ", stdout);

	      fputs (buffer, stdout);

	      if (j == 0)
		{
		  if (i % 40 == 0 && i != 0)
		    fputs ("\n", stdout);
		}
	      else
		{
		  if (i % 20 == 0 && i != 0)
		    fputs ("\n", stdout);
		}
	    }
	}
    }
  fputs ("\n", stdout);

  return 0;
}

