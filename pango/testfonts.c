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
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <gmodule.h>
#include "pango.h"
#include "pango-utils.h"
#include "pangowin32.h"


#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <locale.h>

#include <windows.h>

gchar*
gtk_get_default_language (void)
{
  gchar *lang;
  gchar *p;
  
  lang = g_strdup (setlocale (LC_CTYPE, NULL));
  p = strchr (lang, '.');
  if (p)
    *p = '\0';
  p = strchr (lang, '@');
  if (p)
    *p = '\0';

  return lang;
}

HDC  pre_render (int width, int height);
void post_render (HDC hdc, const char* sFile);

float 
calc_duration (GTimeVal *tv1, GTimeVal *tv0)
{
  return (  ((float)tv1->tv_sec - tv0->tv_sec)
          + (tv1->tv_usec - tv0->tv_usec) / 1000000.0);
}

int main (int argc, char **argv)
{
  PangoFontMap *fontmap = pango_win32_font_map_for_display();
  PangoCoverage * coverage = NULL;
  PangoFont* font = NULL;
  PangoFontFamily** families; 
  int nb;
  int i;
  gchar* family_name = NULL;
  gchar* lang = gtk_get_default_language ();
  HDC hdc = NULL;
  int line = 0;
  GTimeVal tv0, tv1;
  int my_font_size = 64;

  printf ("# Pango Font Test\n"
	  "# Language: %s\n",
	  "#\n", lang);

  /* FIXME: this wasn't necessary with previous version
   *   and isn't done with current gtk+ win32 
   *
   * force initialization of built-in engines, otherwise
   * the rendering get's really fast - too fast to work :-(
   */
  pango_win32_get_context ();

  if (argc == 1)		/* No arguments given */
    {
      PangoFontDescription *desc = pango_font_description_from_string("Sans 12");

      font = pango_font_map_load_font (fontmap, desc);
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
      family_name = pango_font_description_get_family (desc);

      font = pango_font_map_load_font (fontmap, desc);
      
      coverage = pango_font_get_coverage (font, lang);

      /* ... */

      pango_coverage_unref (coverage);
      pango_font_description_free (desc);
      g_object_unref (G_OBJECT (font));
    }

  pango_font_map_list_families (fontmap, &families, &nb);

  hdc = pre_render(my_font_size * 64, 3 * my_font_size * nb / 2);

  for (i = 0; i < nb; i++)
  {
    PangoFontDescription *desc = pango_font_description_new ();
    char *family_name = pango_font_family_get_name (families[i]);
    PangoWeight weight = pango_font_description_get_weight (desc);
    PangoStyle  style  = pango_font_description_get_style  (desc);

    g_print ("Family: %s; Style: %d; Weight: %d\n",
	       family_name, style, weight);

    pango_font_description_set_family (desc, family_name);

    /* give it an arbitray size to load it */
    pango_font_description_set_size (desc, my_font_size * PANGO_SCALE);

    g_get_current_time (&tv0);
    font = pango_font_map_load_font (fontmap, desc);
    g_get_current_time (&tv1);
    g_print ("\tpango_font_map_load_font took %.3f sec\n", calc_duration (&tv1, &tv0));

    if (font) 
    {
      PangoContext  *context;
      PangoLayout   *layout;
      PangoItem     *item;
      PangoGlyphString * glyphs;
      char s[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "1234567890 -+*/!§$%&()[]{}<>|#=?@";

      g_get_current_time (&tv0);
      coverage = pango_font_get_coverage (font, lang);
      g_get_current_time (&tv1);
      g_print ("\tpango_font_get_coverage took %.3f sec\n", calc_duration (&tv1, &tv0));

      /* ... */
      context = pango_win32_get_context ();
      pango_context_set_language (context, lang);
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_font_description (context, desc);

      glyphs = pango_glyph_string_new ();
      item = pango_item_new ();

      item->analysis.shape_engine = pango_font_find_shaper (font, lang, s[0]);
      item->analysis.font = g_object_ref (G_OBJECT (font));
      pango_shape ( s, sizeof(s), &(item->analysis), glyphs);

      if (hdc) 
      {
	  /* the positioning isn't correct */
	  char * name = g_strdup_printf ("%s (%s%s)",
					 family_name,
					 weight == PANGO_WEIGHT_NORMAL ? "n" :
					  (weight == PANGO_WEIGHT_HEAVY ? "h" :
					  (weight > PANGO_WEIGHT_NORMAL ? "b" : "l")),
					 style == PANGO_STYLE_OBLIQUE ? "o" :
					  (style == PANGO_STYLE_ITALIC ? "i" : "n"));

	  TextOut (hdc, 0, line, name, strlen(name));
          g_get_current_time (&tv0);
          pango_win32_render (hdc, font, glyphs, 200, line);
          g_get_current_time (&tv1);
          g_print ("\tpango_win32_render took %.3f sec\n", calc_duration (&tv1, &tv0));
	  line += (3 * my_font_size / 2);
	  g_free(name);
      }

      /* free glyphs, ... */
      pango_glyph_string_free (glyphs);
      pango_item_free (item);

      pango_coverage_unref (coverage); 
      g_object_unref (G_OBJECT (font));
    }
    pango_font_description_free (desc);
  }

  if (hdc)
    post_render (hdc, "pango-fonts.bmp");

  pango_font_map_list_families (fontmap, &families, &nb);
  for (i = 0; i < nb; i++)
  {
    //g_print ("%s\n", families[i]);
  }

  //pango_font_map_free_families (families, nb);

  return 0;
}

/*
 * Real Win32 specific render support
 */
static HBITMAP hbmpold = NULL;
static HWND hwndRender = NULL;

BOOL
SaveBitmap(HBITMAP hBmp, char* pszFile);

HDC pre_render (int width, int height)
{
  
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

  hdc = CreateCompatibleDC (GetDC(hwndRender));
  hbmp = CreateCompatibleBitmap (hdc, width, height);
  hbmpold = SelectObject(hdc, hbmp);

  FillRect (hdc, &r, GetStockObject(WHITE_BRUSH));
  SetTextColor (hdc, RGB (0,0,0));
  SetBkMode (hdc, TRANSPARENT);
  return hdc;
}

void post_render (HDC hdc, char* sFile)
{
  HBITMAP hbmp = SelectObject(hdc, hbmpold);
  if (sFile)
    SaveBitmap (hbmp, sFile);
  DeleteObject (hbmp);
  DeleteDC (hdc);
  ReleaseDC(hwndRender, GetDC(hwndRender));
  DestroyWindow (hwndRender);
}

BOOL
SaveBitmap(HBITMAP hBmp, char* pszFile)
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
  if (cClrBits != 24)
    pbmi = (PBITMAPINFO) GlobalAlloc(LPTR,
				    sizeof(BITMAPINFOHEADER) +
				    sizeof(RGBQUAD) * (2^cClrBits));
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
    pbmi->bmiHeader.biClrUsed = 2^cClrBits;
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

  { // C sucks
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
  } // C sucks
  return TRUE;
} 
