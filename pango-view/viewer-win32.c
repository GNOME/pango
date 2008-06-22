/* Pango
 * viewer-win32.c: Example program to view a UTF-8 encoding file
 *                 using Pango to render result.
 *
 * Copyright (C) 1999 Red Hat Software
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
#include <gtk/gtk.h>
#include <gdk/win32/gdkwin32.h>

#include <pango/pango.h>
#include <pango/pangowin32.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 1024

typedef struct _Paragraph Paragraph;

/* Structure representing a paragraph
 */
struct _Paragraph {
  char *text;
  int length;
  int height;   /* Height, in pixels */
  PangoLayout *layout;
};

GList *paragraphs;

static PangoFontDescription *font_description;
static Paragraph *highlight_para;
static int highlight_offset;

GtkWidget *styles_combo;

static GtkWidget *message_label;
GtkWidget *layout;

PangoContext *context;

static void fill_styles_combo (GtkWidget *combo);

/* Read an entire file into a string
 */
static char *
read_file(char *name)
{
  GString *inbuf;
  FILE *file;
  char *text;
  char buffer[BUFSIZE];

  file = fopen (name, "r");
  if (!file)
    {
      fprintf (stderr, "%s: Cannot open %s\n", g_get_prgname (), name);
      return NULL;
    }

  inbuf = g_string_new (NULL);
  while (1)
    {
      char *bp = fgets (buffer, BUFSIZE-1, file);
      if (ferror (file))
	{
	  fprintf (stderr, "%s: Error reading %s\n", g_get_prgname (), name);
	  g_string_free (inbuf, TRUE);
	  return NULL;
	}
      else if (bp == NULL)
	break;

      g_string_append (inbuf, buffer);
    }

  fclose (file);

  text = inbuf->str;
  g_string_free (inbuf, FALSE);

  return text;
}

/* Take a UTF8 string and break it into paragraphs on \n characters
 */
static GList *
split_paragraphs (char *text)
{
  char *p = text;
  char *next;
  gunichar wc;
  GList *result = NULL;
  char *last_para = text;

  while (*p)
    {
      wc = g_utf8_get_char (p);
      next = g_utf8_next_char (p);
      if (wc == (gunichar)-1)
	{
	  fprintf (stderr, "%s: Invalid character in input\n", g_get_prgname ());
	  g_list_foreach (result, (GFunc)g_free, NULL);
	  return NULL;
	}
      if (!*p || !wc || wc == '\n')
	{
	  Paragraph *para = g_new (Paragraph, 1);
	  para->text = last_para;
	  para->length = p - last_para;
	  para->layout = pango_layout_new (context);
	  pango_layout_set_text (para->layout, para->text, para->length);
	  para->height = 0;

	  last_para = next;

	  result = g_list_prepend (result, para);
	}
      if (!wc) /* incomplete character at end */
	break;
      p = next;
    }

  return g_list_reverse (result);
}

/* Given an x-y position, return the paragraph and offset
 * within the paragraph of the click.
 */
gboolean
xy_to_cp (int width, int x, int y, Paragraph **para_return, int *index)
{
  GList *para_list;
  int height = 0;

  *para_return = NULL;

  para_list = paragraphs;
  while (para_list && height < y)
    {
      Paragraph *para = para_list->data;

      if (height + para->height >= y)
	{
	  gboolean result = pango_layout_xy_to_index (para->layout,
						      x * PANGO_SCALE,
						      (y - height) * PANGO_SCALE,
						      index, NULL);
	  if (result && para_return)
	    *para_return = para;

	  return result;
	}

      height += para->height;
      para_list = para_list->next;
    }

  return FALSE;
}

/* Given a paragraph and offset in that paragraph, find the
 * bounding rectangle for the character at the offset.
 */
void
char_bounds (Paragraph *para, int index, int width, PangoRectangle *rect)
{
  GList *para_list;

  int height = 0;

  para_list = paragraphs;
  while (para_list)
    {
      Paragraph *cur_para = para_list->data;

      if (cur_para == para)
	{
	  PangoRectangle pos;

	  pango_layout_index_to_pos (cur_para->layout, index, &pos);

	  rect->x = PANGO_PIXELS (MIN (pos.x, pos.x + pos.width));
	  rect->width = PANGO_PIXELS (ABS (pos.width));
	  rect->y = height + PANGO_PIXELS (pos.y);
	  rect->height = PANGO_PIXELS (pos.height);
	}

      height += cur_para->height;
      para_list = para_list->next;
    }
}

/* XOR a rectangle over a given character
 */
void
xor_char (GtkWidget *layout, GdkRectangle *clip_rect,
	  Paragraph *para, int offset)
{
  static GdkGC *gc;
  PangoRectangle rect;		/* GdkRectangle in 1.2 is too limited
				 */
  if (!gc)
    {
      GdkGCValues values;
      values.foreground = layout->style->white.pixel ?
	layout->style->white : layout->style->black;
      values.function = GDK_XOR;
      gc = gdk_gc_new_with_values (GTK_LAYOUT (layout)->bin_window,
				   &values,
				   GDK_GC_FOREGROUND | GDK_GC_FUNCTION);
    }

  gdk_gc_set_clip_rectangle (gc, clip_rect);

  char_bounds (para, offset, layout->allocation.width, &rect);

  rect.y -= GTK_LAYOUT (layout)->yoffset;

  if ((rect.y + rect.height >= 0) && (rect.y < layout->allocation.height))
    gdk_draw_rectangle (GTK_LAYOUT (layout)->bin_window, gc, TRUE,
			rect.x, rect.y, rect.width, rect.height);
}

/* Handle a size allocation by re-laying-out each paragraph to
 * the new width, setting the new size for the layout and
 * then queing a redraw
 */
void
size_allocate (GtkWidget *layout, GtkAllocation *allocation)
{
  GList *tmp_list;
  guint height = 0;
  PangoDirection base_dir = pango_context_get_base_dir (context);

  tmp_list = paragraphs;
  while (tmp_list)
    {
      Paragraph *para = tmp_list->data;
      PangoRectangle logical_rect;

      tmp_list = tmp_list->next;

      pango_layout_set_alignment (para->layout,
				  base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
      pango_layout_set_width (para->layout, layout->allocation.width * PANGO_SCALE);

      pango_layout_get_extents (para->layout, NULL, &logical_rect);
      para->height = PANGO_PIXELS (logical_rect.height);

      height += para->height;
    }

  gtk_layout_set_size (GTK_LAYOUT (layout), allocation->width, height);

  if (GTK_LAYOUT (layout)->yoffset + allocation->height > height)
    gtk_adjustment_set_value (GTK_LAYOUT (layout)->vadjustment, (float)(height - allocation->height));
}

/* Handle a draw/expose by finding the paragraphs that intersect
 * the region and reexposing them.
 */
void
draw (GtkWidget *layout, GdkRectangle *area)
{
  GList *tmp_list;
  guint height = 0;
  HDC hdc;
  const GdkGCValuesMask mask = GDK_GC_FOREGROUND|GDK_GC_BACKGROUND|GDK_GC_FONT;

  gdk_draw_rectangle (GTK_LAYOUT (layout)->bin_window,
		      layout->style->base_gc[layout->state],
		      TRUE,
		      area->x, area->y,
		      area->width, area->height);

  gdk_gc_set_clip_rectangle (layout->style->text_gc[layout->state], area);

  hdc = gdk_win32_hdc_get (GTK_LAYOUT (layout)->bin_window,
			   layout->style->text_gc[GTK_STATE_NORMAL],
			   mask);
  tmp_list = paragraphs;
  while (tmp_list &&
	 height < area->y + area->height + GTK_LAYOUT (layout)->yoffset)
    {
      Paragraph *para = tmp_list->data;
      tmp_list = tmp_list->next;

      if (height + para->height >= GTK_LAYOUT (layout)->yoffset + area->y)
	pango_win32_render_layout (hdc, para->layout,
				   0, height - GTK_LAYOUT (layout)->yoffset);
      height += para->height;
    }

  gdk_win32_hdc_release (GTK_LAYOUT (layout)->bin_window,
			 layout->style->text_gc[GTK_STATE_NORMAL],
			 mask);
  gdk_gc_set_clip_rectangle (layout->style->text_gc[layout->state], NULL);

  if (highlight_para)
    xor_char (layout, area, highlight_para, highlight_offset);
}

gboolean
expose (GtkWidget *layout, GdkEventExpose *event)
{
  if (event->window == GTK_LAYOUT (layout)->bin_window)
    draw (layout, &event->area);

  return TRUE;
}

void
button_press (GtkWidget *layout, GdkEventButton *event)
{
  Paragraph *para = NULL;
  int offset;
  gchar *message;

  xy_to_cp (layout->allocation.width,
	    event->x, event->y + GTK_LAYOUT (layout)->yoffset,
	    &para, &offset);

  if (highlight_para)
    xor_char (layout, NULL, highlight_para, highlight_offset);

  highlight_para = para;
  highlight_offset = offset;

  if (para)
    {
      gunichar wc;

      wc = g_utf8_get_char (para->text + offset);
      message = g_strdup_printf ("Current char: U%04x", wc);

      xor_char (layout, NULL, highlight_para, highlight_offset);
    }
  else
    message = g_strdup_printf ("Current char:");

  gtk_label_set_text (GTK_LABEL (message_label), message);
  g_free (message);
}

static void
checkbutton_toggled (GtkWidget *widget, gpointer data)
{
  GList *para_list;

  pango_context_set_base_dir (context, GTK_TOGGLE_BUTTON (widget)->active ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);

  para_list = paragraphs;
  while (para_list)
    {
      Paragraph *para = para_list->data;

      pango_layout_context_changed (para->layout);
      para_list = para_list->next;
    }

  gtk_widget_queue_resize (layout);
}

static void
reload_font ()
{
  GList *para_list;

  pango_context_set_font_description (context, font_description);

  para_list = paragraphs;
  while (para_list)
    {
      Paragraph *para = para_list->data;

      pango_layout_context_changed (para->layout);
      para_list = para_list->next;
    }

  if (layout)
    gtk_widget_queue_resize (layout);
}

void
set_family (GtkWidget *entry, gpointer data)
{
  gchar *family_name = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  if (!family_name || family_name[0] == '\0')
    return;
  pango_font_description_set_family(font_description, family_name);
  fill_styles_combo (styles_combo);
}

void
set_style (GtkWidget *entry, gpointer data)
{
  char *str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  PangoFontDescription *tmp_desc;

  tmp_desc = pango_font_description_from_string (str);

  pango_font_description_set_style(font_description, pango_font_description_get_style(tmp_desc));
  pango_font_description_set_variant(font_description, pango_font_description_get_variant(tmp_desc));
  pango_font_description_set_weight(font_description, pango_font_description_get_weight(tmp_desc));
  pango_font_description_set_stretch(font_description, pango_font_description_get_stretch(tmp_desc));

  pango_font_description_free (tmp_desc);
  g_free (str);

  reload_font ();
}

void
font_size_changed (GtkAdjustment *adj)
{
  pango_font_description_set_size(font_description, (int)(adj->value * PANGO_SCALE + 0.5));
  reload_font();
}

static int
compare_font_descriptions (const PangoFontDescription *a, const PangoFontDescription *b)
{
  int val = strcmp (pango_font_description_get_family(a), pango_font_description_get_family(b));
  if (val != 0)
    return val;

  if (pango_font_description_get_weight(a) != pango_font_description_get_weight(b))
    return pango_font_description_get_weight(a) - pango_font_description_get_weight(b);

  if (pango_font_description_get_style(a) != pango_font_description_get_style(b))
    return pango_font_description_get_style(a) - pango_font_description_get_style(b);

  if (pango_font_description_get_stretch(a) != pango_font_description_get_stretch(b))
    return pango_font_description_get_stretch(a) - pango_font_description_get_stretch(b);

  if (pango_font_description_get_stretch(a) != pango_font_description_get_stretch(b))
    return pango_font_description_get_stretch(a) - pango_font_description_get_stretch(b);

  return 0;
}

static int
font_description_sort_func (const void *a, const void *b)
{
  return compare_font_descriptions (*(PangoFontDescription **)a, *(PangoFontDescription **)b);
}

typedef struct
{
  PangoFontDescription **descs;
  int n_descs;
} FontDescInfo;

static void
free_info (FontDescInfo *info)
{
  pango_font_descriptions_free (info->descs, info->n_descs);
}

static void
fill_styles_combo (GtkWidget *combo)
{
  int i;
  GList *style_list = NULL;
  PangoFontFace **faces;
  PangoFontFamily *family, **families;
  FontDescInfo *info;
  int n_families;
  const char *family_name = pango_font_description_get_family(font_description);

  /*
   * Now map back the given family name to the family. There are more efficient
   * ways to handle this but it should not matter much ...
   */
  pango_context_list_families (context, &families, &n_families);
  g_assert(n_families > 0);
  family = families[0]; /* just in case */
  for (i = 0; i < n_families; i++)
    {
      if (0 == g_strcasecmp(pango_font_family_get_name(families[i]), family_name))
	{
	  family = families[i];
	  break;
	}
    }
  g_free (families);

  info = g_new (FontDescInfo, 1);
  pango_font_family_list_faces (family, &faces, &info->n_descs);

  info->descs = g_new0 (PangoFontDescription*, info->n_descs);
  for (i = 0; i < info->n_descs; i++)
    {
      info->descs[i] = pango_font_face_describe(faces[i]);
    }
  g_free (faces);

  gtk_object_set_data_full (GTK_OBJECT (combo), "descs", info, (GtkDestroyNotify)free_info);

  qsort (info->descs, info->n_descs, sizeof(PangoFontDescription *), font_description_sort_func);

  for (i=0; i<info->n_descs; i++)
    {
      char *str;

      PangoFontDescription *tmp_desc;

      tmp_desc = info->descs[i];
      pango_font_description_set_family(tmp_desc, NULL);
      pango_font_description_unset_fields(tmp_desc, PANGO_FONT_MASK_SIZE);

      str = pango_font_description_to_string (tmp_desc);
      style_list = g_list_prepend (style_list, str);
    }

  style_list = g_list_reverse (style_list);

  gtk_combo_set_popdown_strings (GTK_COMBO (combo), style_list);
  g_list_foreach (style_list, (GFunc)g_free, NULL);
}

static GtkWidget *
make_styles_combo ()
{
  GtkWidget *combo;

  combo = gtk_combo_new ();
  gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (combo)->entry), FALSE);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
		      GTK_SIGNAL_FUNC (set_style), NULL);

  styles_combo = combo;
  fill_styles_combo (combo);

  return combo;
}

static int
cmp_families (const PangoFontFamily** a, const PangoFontFamily** b)
{
  return strcmp (pango_font_family_get_name (*a), pango_font_family_get_name (*b));
}

GtkWidget *
make_families_menu ()
{
  GtkWidget *combo;
  int n_families;
  PangoFontFamily **families;
  GList *family_list = NULL;
  int i;

  pango_context_list_families (context, &families, &n_families);
  qsort (families, n_families, sizeof(char *), cmp_families);

  for (i=0; i<n_families; i++)
    family_list = g_list_prepend (family_list, pango_font_family_get_name (families[i]));

  family_list = g_list_reverse (family_list);

  combo = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), family_list);
  gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (combo)->entry), FALSE);

  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), pango_font_description_get_family(font_description));

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
		      GTK_SIGNAL_FUNC (set_family), NULL);

  g_list_free (family_list);

  return combo;
}


GtkWidget *
make_font_selector (void)
{
  GtkWidget *hbox;
  GtkWidget *util_hbox;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *spin_button;
  GtkAdjustment *adj;

  hbox = gtk_hbox_new (FALSE, 4);

  util_hbox = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new ("Family:");
  gtk_box_pack_start (GTK_BOX (util_hbox), label, FALSE, FALSE, 0);
  option_menu = make_families_menu ();
  gtk_box_pack_start (GTK_BOX (util_hbox), option_menu, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), util_hbox, FALSE, FALSE, 0);

  util_hbox = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new ("Style:");
  gtk_box_pack_start (GTK_BOX (util_hbox), label, FALSE, FALSE, 0);
  option_menu = make_styles_combo ();
  gtk_box_pack_start (GTK_BOX (util_hbox), option_menu, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), util_hbox, FALSE, FALSE, 0);

  util_hbox = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new ("Size:");
  gtk_box_pack_start (GTK_BOX (util_hbox), label, FALSE, FALSE, 0);
  spin_button = gtk_spin_button_new (NULL, 1., 0);
  gtk_box_pack_start (GTK_BOX (util_hbox), spin_button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), util_hbox, FALSE, FALSE, 0);

  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin_button));
  adj->value = PANGO_PIXELS (pango_font_description_get_size(font_description));
  adj->lower = 0;
  adj->upper = 1024;
  adj->step_increment = 1;
  adj->page_size = 10;
  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);

  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (font_size_changed), NULL);

  return hbox;
}

int
main (int argc, char **argv)
{
  char *text;
  GtkWidget *window;
  GtkWidget *scrollwin;
  GtkWidget *vbox, *hbox;
  GtkWidget *frame;
  GtkWidget *checkbutton;

  gtk_init (&argc, &argv);

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s FILE\n", g_get_prgname ());
      exit(1);
    }

  /* Create the list of paragraphs from the supplied file
   */
  text = read_file (argv[1]);
  if (!text)
    exit(1);

  context = pango_win32_get_context ();

  paragraphs = split_paragraphs (text);

  pango_context_set_language (context, pango_language_from_string ("en_US"));
  pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);

  font_description = pango_font_description_new ();
  pango_font_description_set_family(font_description, "sans");
  pango_font_description_set_size(font_description, 16 * PANGO_SCALE);
#if 0 /* default init ok? */
  font_description.style = PANGO_STYLE_NORMAL;
  font_description.variant = PANGO_VARIANT_NORMAL;
  font_description.weight = 500;
  font_description.stretch = PANGO_STRETCH_NORMAL;
#endif

  pango_context_set_font_description (context, font_description);

  /* Create the user interface
   */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 400);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = make_font_selector ();
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);

  layout = gtk_layout_new (NULL, NULL);
  gtk_widget_set_events (layout, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_app_paintable (layout, TRUE);

  gtk_signal_connect (GTK_OBJECT (layout), "size_allocate",
		      GTK_SIGNAL_FUNC (size_allocate), paragraphs);
  gtk_signal_connect (GTK_OBJECT (layout), "expose_event",
		      GTK_SIGNAL_FUNC (expose), paragraphs);
  gtk_signal_connect (GTK_OBJECT (layout), "draw",
		      GTK_SIGNAL_FUNC (draw), paragraphs);
  gtk_signal_connect (GTK_OBJECT (layout), "button_press_event",
		      GTK_SIGNAL_FUNC (button_press), paragraphs);
#if GTK_CHECK_VERSION (1,3,2)
  gtk_widget_set_double_buffered (layout, FALSE);
#endif
  gtk_container_add (GTK_CONTAINER (scrollwin), layout);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  message_label = gtk_label_new ("Current char:");
  gtk_misc_set_padding (GTK_MISC (message_label), 1, 1);
  gtk_misc_set_alignment (GTK_MISC (message_label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (frame), message_label);

  checkbutton = gtk_check_button_new_with_label ("Use RTL global direction");
  gtk_signal_connect (GTK_OBJECT (checkbutton), "toggled",
		      GTK_SIGNAL_FUNC (checkbutton_toggled), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), checkbutton, FALSE, FALSE, 0);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
