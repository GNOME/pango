/* Pango
 * Copyright (C) 1999 Red Hat Software
 *
 * testfonts.c:
 * Copyright (C) 2001 Hans Breuer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib.h>
#include <stdlib.h>
#include "pango.h"
#include "pango-impl-utils.h"
#include "pangowin32.h"


#include <errno.h>
#include <string.h>
#ifndef G_OS_WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <locale.h>

#include <windows.h>

static HDC pre_render (int width, int height);
static void post_render (HDC hdc, const char* sFile);

static float
calc_duration (GTimeVal *tv1, GTimeVal *tv0)
{
  return (  ((float)tv1->tv_sec - tv0->tv_sec)
	  + (tv1->tv_usec - tv0->tv_usec) / 1000000.0);
}

static int
compare_font_family (gconstpointer a,
                     gconstpointer b)
{
  PangoFontFamily **aa = (PangoFontFamily **)a;
  PangoFontFamily **bb = (PangoFontFamily **)b;
  return strcmp (pango_font_family_get_name (*aa),
                 pango_font_family_get_name (*bb));
}

int main (int argc, char **argv)
{
  PangoFontMap *fontmap = pango_win32_font_map_for_display();
  PangoContext *context;
  PangoCoverage * coverage = NULL;
  PangoFont* font = NULL;
  PangoFontFamily** families = NULL;
  PangoFontFace** faces = NULL;
  int nb, i;
  gchar* family_name = NULL;
  PangoLanguage *lang = pango_language_from_string (g_win32_getlocale ());
  HDC hdc = NULL;
  int line = 0;
  GTimeVal tv0, tv1;
  int my_font_size = 12;

  printf ("# Pango Font Test\n"
	  "# Language: %s\n"
	  "#\n", pango_language_to_string (lang));

  /* this wasn't necessary with previous version
   *
   * force initialization of built-in engines, otherwise
   * the rendering get's really fast - too fast to work :-(
   */
  context = pango_font_map_create_context (pango_win32_font_map_for_display ());

  if (argc == 1)		/* No arguments given */
    {
      char *std_fonts[] = {"Sans 12", "Serif 12", "Monospace 12"};

      /* try to load some fonts often hardcoded */
      for (i = 0; i < G_N_ELEMENTS (std_fonts); i++)
	{
	  PangoFontDescription *desc = pango_font_description_from_string(std_fonts[i]);

	  /* spits warnings if font cannot be loaded */
	  font = pango_font_map_load_font (fontmap, context, desc);

	  g_object_unref (font);
	}
    }
  else
    {
      PangoFontDescription *desc = NULL;
      GString *s;

      s = g_string_new (argv[1]);
      for (i = 2; i < argc; i++)
	{
	  s = g_string_append_c (s, ' ');
	  s = g_string_append (s, argv[i]);

	  if (0 != atoi (argv[i]))
	    my_font_size = atoi (argv[i]);
	}

      desc = pango_font_description_from_string(s->str);
      family_name = g_strdup (pango_font_description_get_family (desc));

      font = pango_font_map_load_font (fontmap, context, desc);

      coverage = pango_font_get_coverage (font, lang);

      /* ... */

      g_object_unref (coverage);
      pango_font_description_free (desc);
      g_object_unref (font);
    }

  pango_font_map_list_families (fontmap, &families, &nb);

  if (!family_name)
    {
      qsort (families, nb, sizeof (PangoFontFamily*), compare_font_family);
    }
  else
    {
      /* Get on the family faces. No simple way ? */
      for (i = 0; i < nb; i++)
	{
	  if (0 == g_ascii_strcasecmp (pango_font_family_get_name (families[i]), family_name))
	    {
	      pango_font_family_list_faces (families[i], &faces, &nb);
	      /* now nb is the number of faces */
	      break;
	    }
	}
      g_free (families);
      families = NULL;
      g_free (family_name);
      family_name = NULL;
    }

  hdc = pre_render(my_font_size * 64, 3 * my_font_size * nb / 2);

  for (i = 0; i < nb; i++)
    {
      PangoFontDescription *desc;
      const char *f_name;
      PangoWeight weight;
      PangoStyle  style;

      if (families)
	{
	  desc = pango_font_description_new ();

	  f_name =  pango_font_family_get_name (families[i]);
	  pango_font_description_set_family (desc, f_name);
	}
      else
	{
	  desc = pango_font_face_describe (faces[i]);
	  /* this is _not_ the family name from above */
	  f_name = pango_font_description_get_family (desc);
	}
      weight = pango_font_description_get_weight (desc);
      style  = pango_font_description_get_style  (desc);

      g_print ("%s; Style: %d; Weight: %d\n",
	       f_name, style, weight);

      /* give it an arbitray size to load it */
      pango_font_description_set_size (desc, my_font_size * PANGO_SCALE);

      g_get_current_time (&tv0);
      font = pango_font_map_load_font (fontmap, context, desc);
      g_get_current_time (&tv1);
      g_print ("\tpango_font_map_load_font took %.3f sec\n", calc_duration (&tv1, &tv0));

      if (font)
	{
	  PangoItem     *item;
	  PangoGlyphString * glyphs;
	  char s[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		     "abcdefghijklmnopqrstuvwxyz"
		     "1234567890 -+*/!\xc2\xa7$%&()[]{}<>|#=?@";

	  g_get_current_time (&tv0);
	  coverage = pango_font_get_coverage (font, lang);
	  g_get_current_time (&tv1);
	  g_print ("\tpango_font_get_coverage took %.3f sec\n", calc_duration (&tv1, &tv0));

	  /* ... */
	  pango_context_set_language (context, lang);
	  pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
	  pango_context_set_font_description (context, desc);

	  glyphs = pango_glyph_string_new ();
	  item = pango_item_new ();

	  item->analysis.font = g_object_ref (font);
	  pango_shape ( s, sizeof(s), &(item->analysis), glyphs);

	  if (hdc)
	    {
	      /* the positioning isn't correct */
	      char* name = g_strdup_printf ("%s (%s%s)",
					    f_name,
					    weight == PANGO_WEIGHT_NORMAL ? "n" :
					      (weight == PANGO_WEIGHT_HEAVY ? "h" :
					      (weight > PANGO_WEIGHT_NORMAL ? "b" : "l")),
					    style == PANGO_STYLE_OBLIQUE ? "o" :
					      (style == PANGO_STYLE_ITALIC ? "i" : "n"));

	      TextOut (hdc, 0, line, name, strlen(name));
	      g_get_current_time (&tv0);
	      pango_win32_render (hdc, font, glyphs, 200, line);
	      g_get_current_time (&tv1);
	      g_print ("\tpango_win32_render took %.3f sec\n",
		       calc_duration (&tv1, &tv0));
	      line += (3 * my_font_size / 2);
	      g_free(name);
	    }

	  /* free glyphs, ... */
	  pango_glyph_string_free (glyphs);
	  pango_item_free (item);

	  g_object_unref (coverage);
	  g_object_unref (font);
	}
      pango_font_description_free (desc);
    }

  if (hdc)
    post_render (hdc, "pango-fonts.bmp");

  g_free (families);
  g_free (faces);

  return 0;
}

/*
 * Real Win32 specific render support
 */
static HBITMAP hbmpold = NULL;
static HWND hwndRender = NULL;

static BOOL
SaveBitmap (HBITMAP hBmp, const char* pszFile);

static HDC
pre_render (int width, int height)
{
  HDC hmemdc;
  HDC hdc;
  HBITMAP hbmp;
  RECT r;
  r.top = 0; r.left = 0;
  r.right  = width;
  r.bottom = height;

  hwndRender = CreateWindow ("EDIT",
			     "pango-render-window",
			     WS_DISABLED,
			     0, 0, width, height,
			     GetDesktopWindow(),
			     NULL,
			     GetModuleHandle(NULL),
			     NULL);

  if (hwndRender == NULL)
    fprintf (stderr, "Couldn't create window\n"), exit (1);

  hdc = GetDC(hwndRender);
  hmemdc = CreateCompatibleDC (hdc);
  if (hdc == NULL)
    fprintf (stderr, "CreateCompatibleDC failed\n"), exit (1);

  hbmp = CreateCompatibleBitmap (hdc, width, height);
  if (hbmp == NULL)
    fprintf (stderr, "CreateCompatibleBitmap failed\n"), exit (1);

  hbmpold = SelectObject(hmemdc, hbmp);

  FillRect (hmemdc, &r, GetStockObject(WHITE_BRUSH));
  SetTextColor (hmemdc, RGB (0,0,0));
  SetBkMode (hmemdc, TRANSPARENT);
  return hmemdc;
}

static void
post_render (HDC hdc, const char* sFile)
{
  HBITMAP hbmp = SelectObject(hdc, hbmpold);
  if (sFile)
    SaveBitmap (hbmp, sFile);
  DeleteObject (hbmp);
  DeleteDC (hdc);
  ReleaseDC(hwndRender, GetDC(hwndRender));
  DestroyWindow (hwndRender);
}

static BOOL
SaveBitmap (HBITMAP hBmp, const char* pszFile)
{
  BITMAP bmp;
  PBITMAPINFO pbmi;
  WORD    cClrBits;
  /* Retrieve the bitmap's color format, width, and height. */
  if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp))
    return FALSE;
  /* Convert the color format to a count of bits. */
  cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
  if (cClrBits == 1)
    cClrBits = 1;
  else if (cClrBits <= 4)
    cClrBits = 4;
  else if (cClrBits <= 8)
    cClrBits = 8;
  else if (cClrBits <= 16)
    cClrBits = 16;
  else if (cClrBits <= 24)
    cClrBits = 24;
  else
    cClrBits = 32;

  /*
   * Allocate memory for the BITMAPINFO structure. (This structure
   * contains a BITMAPINFOHEADER structure and an array of RGBQUAD data
   * structures.)      */
  if (cClrBits < 24)
    pbmi = (PBITMAPINFO) GlobalAlloc(LPTR,
				    sizeof(BITMAPINFOHEADER) +
				    sizeof(RGBQUAD) * (1 << cClrBits));
  /*
   * There is no RGBQUAD array for the 24-bit-per-pixel format.      */
  else
    pbmi = (PBITMAPINFO) GlobalAlloc(LPTR,
				    sizeof(BITMAPINFOHEADER));
  /* Initialize the fields in the BITMAPINFO structure. */
  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth = bmp.bmWidth;
  pbmi->bmiHeader.biHeight = bmp.bmHeight;
  pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
  pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
  if (cClrBits < 24)
    pbmi->bmiHeader.biClrUsed = (1 << cClrBits);
  else
    pbmi->bmiHeader.biClrUsed = 0;
  /* If the bitmap is not compressed, set the BI_RGB flag. */
  pbmi->bmiHeader.biCompression = BI_RGB;
  /*
   * Compute the number of bytes in the array of color
   * indices and store the result in biSizeImage.
   */
  pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) /8
				  * pbmi->bmiHeader.biHeight
				  * cClrBits;
  /*
   * Set biClrImportant to 0, indicating that all of the
   * device colors are important.
   */
  pbmi->bmiHeader.biClrImportant = 0;

  { /* C sucks */
  HANDLE hf;                  /* file handle */
  BITMAPFILEHEADER hdr;       /* bitmap file-header */
  PBITMAPINFOHEADER pbih;     /* bitmap info-header */
  LPBYTE lpBits;              /* memory pointer */
  DWORD dwTotal;              /* total count of bytes */
  DWORD cb;                   /* incremental count of bytes */
  DWORD dwTmp;
  HDC hDC;

  pbih = (PBITMAPINFOHEADER) pbmi;
  lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
  if (!lpBits)
    return FALSE;
  /*
   * Retrieve the color table (RGBQUAD array) and the bits
   * (array of palette indices) from the DIB.
   */
  hDC = CreateCompatibleDC(NULL);
  if (!GetDIBits(hDC, hBmp, 0, (WORD) pbih->biHeight,
		 lpBits, pbmi, DIB_RGB_COLORS))
    return FALSE;
  /* Create the .BMP file. */
  hf = CreateFile (pszFile,
		   GENERIC_READ | GENERIC_WRITE,
		   (DWORD) 0,
		   (LPSECURITY_ATTRIBUTES) NULL,
		   CREATE_ALWAYS,
		   FILE_ATTRIBUTE_NORMAL,
		   (HANDLE) NULL);

  if (hf == INVALID_HANDLE_VALUE)
    return FALSE;
  hdr.bfType = 0x4d42;        /* 0x42 = "B" 0x4d = "M" */
  /* Compute the size of the entire file. */
  hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER)
			+ pbih->biSize + pbih->biClrUsed
			* sizeof(RGBQUAD) + pbih->biSizeImage);
  hdr.bfReserved1 = 0;
  hdr.bfReserved2 = 0;
  /* Compute the offset to the array of color indices. */
  hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER)
			  + pbih->biSize + pbih->biClrUsed
			  * sizeof (RGBQUAD);
  /* Copy the BITMAPFILEHEADER into the .BMP file. */
  if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER),
		 (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL))
    return FALSE;
  /* Copy the BITMAPINFOHEADER and RGBQUAD array into the file. */
  if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER)
		     + pbih->biClrUsed * sizeof (RGBQUAD),
		     (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL))
    return FALSE;
  /* Copy the array of color indices into the .BMP file. */
  dwTotal = cb = pbih->biSizeImage;

  if (!WriteFile(hf, (LPSTR) lpBits, (int) cb,
		 (LPDWORD) &dwTotal, (LPOVERLAPPED) NULL))
      return FALSE;

  /* Close the .BMP file. */
  if (!CloseHandle(hf))
    return FALSE;

  /* Free memory. */
  GlobalFree((HGLOBAL)lpBits);
  GlobalFree((HGLOBAL)pbmi);

  DeleteDC(hDC);
  } /* C sucks */
  return TRUE;
}
