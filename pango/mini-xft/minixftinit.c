/*
 * $XFree86: xc/lib/MiniXft/xftinit.c,v 1.2 2000/12/15 17:12:53 keithp Exp $
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <stdlib.h>

#include <glib.h>

#include "minixftint.h"

MiniXftFontSet  *_MiniXftFontSet;
Bool	         _MiniXftConfigInitialized;

Bool
MiniXftInit (char *config)
{
    if (_MiniXftConfigInitialized)
	return True;
    _MiniXftConfigInitialized = True;
    if (!config)
    {
	config = getenv ("XFT_CONFIG");
	if (!config)
	    config = mini_xft_get_default_path ();
    }
    if (MiniXftConfigLexFile (config))
    {
	MiniXftConfigparse ();
    }
    return True;
}

extern char *pango_get_sysconf_subdirectory (void);

#ifdef MINI_XFTCONFIG_DIR
#define SYSCONF_INDEX 1
#else
#define SYSCONF_INDEX 0
#endif

char *
mini_xft_get_default_path (void)
{
  static char *result = NULL;
  char *paths[] = {
#ifdef MINI_XFTCONFIG_DIR
    MINI_XFTCONFIG_DIR,
#endif
    NULL,
#ifndef _WIN32
    "/etc/X11",
    "/usr/X11R6/lib/X11"
#endif
  };
  int i;
  gboolean found = FALSE;
  
  if (result)
    return result;

  paths[SYSCONF_INDEX] = g_build_path (G_DIR_SEPARATOR_S,
				       pango_get_sysconf_subdirectory (),
				       "..",
				       NULL);

  for (i = 0; i < G_N_ELEMENTS (paths); i++)
    {
      if (result)
	g_free (result);

      result = g_build_filename (paths[i], "XftConfig", NULL);

      if (g_file_test (result, G_FILE_TEST_EXISTS))
	{
	  found = TRUE;
	  break;
	}
    }

  if (!found)
    g_warning ("Could not find XftConfig file");

  g_free (paths[SYSCONF_INDEX]);
  return result;
}
