/* Pango
 * pango-markup.c: Parse markup into attributed text
 *
 * Copyright (C) 2000 Red Hat Software
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include <pango/pango-attributes.h>
#include <pango/pango-font.h>
#include <pango/pango-utils.h>

/* FIXME */
#define _(x) x

static gboolean pango_color_parse (const char *spec,
                                   guint16    *red,
                                   guint16    *green,
                                   guint16    *blue);

/* CSS size levels */
typedef enum
{
  XXSmall = -3,
  XSmall = -2,
  Small = -1,
  Medium = 0,
  Large = 1,
  XLarge = 2,
  XXLarge = 3
} SizeLevel;

typedef struct _MarkupData MarkupData;

struct _MarkupData
{
  PangoAttrList *attr_list;
  GString *text;
  GSList *tag_stack;
  gint index;
  GSList *to_apply;
  gunichar accel_marker;
  gunichar accel_char;
};

typedef struct _OpenTag OpenTag;

struct _OpenTag
{
  GSList *attrs;
  gint start_index;
  /* Current total scale level; reset whenever
   * an absolute size is set.
   * Each "larger" ups it 1, each "smaller" decrements it 1
   */
  gint scale_level;
  /* Our impact on scale_level, so we know whether we
   * need to create an attribute ourselves on close
   */
  gint scale_level_delta;
  /* Base scale factor currently in effect
   * or size that this tag
   * forces, or parent's scale factor or size.
   */
  double base_scale_factor;
  int base_font_size;
  guint has_base_font_size : 1;
};

typedef gboolean (*TagParseFunc) (MarkupData            *md,
                                  OpenTag               *tag,
                                  const gchar          **names,
                                  const gchar          **values,
                                  GMarkupParseContext   *context,
                                  GError               **error);

static gboolean b_parse_func        (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean big_parse_func      (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean span_parse_func     (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean i_parse_func        (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean markup_parse_func   (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean s_parse_func        (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean sub_parse_func      (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean sup_parse_func      (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean small_parse_func    (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean tt_parse_func       (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);
static gboolean u_parse_func        (MarkupData           *md,
                                     OpenTag              *tag,
                                     const gchar         **names,
                                     const gchar         **values,
                                     GMarkupParseContext  *context,
                                     GError              **error);

static double
scale_factor (int scale_level, double base)
{
  double factor = base;
  int i;

  /* 1.2 is the CSS scale factor between sizes */
  
  if (scale_level > 0)
    {
      i = 0;
      while (i < scale_level)
        {
          factor *= 1.2;

          ++i;
        }
    }
  else if (scale_level < 0)
    {
      i = scale_level;
      while (i < 0)
        {
          factor /= 1.2;
                  
          ++i;
        }
    }

  return factor;
}

static void
open_tag_free (OpenTag *ot)
{
  g_slist_foreach (ot->attrs, (GFunc) pango_attribute_destroy, NULL);
  g_slist_free (ot->attrs);
  g_free (ot);
}

static void
open_tag_set_absolute_font_size (OpenTag *ot,
                                 int      font_size)
{
  ot->base_font_size = font_size;
  ot->has_base_font_size = TRUE;
  ot->scale_level = 0;
  ot->scale_level_delta = 0;
}

static void
open_tag_set_absolute_font_scale (OpenTag *ot,
                                  double   scale)
{
  ot->base_scale_factor = scale;
  ot->has_base_font_size = FALSE;
  ot->scale_level = 0;
  ot->scale_level_delta = 0;
}
     
static OpenTag*
markup_data_open_tag (MarkupData   *md)
{
  OpenTag *ot;
  OpenTag *parent = NULL;
  
  if (md->attr_list == NULL)
    return NULL;

  if (md->tag_stack)
    parent = md->tag_stack->data;
  
  ot = g_new (OpenTag, 1);
  ot->attrs = NULL;
  ot->start_index = md->index;
  ot->scale_level_delta = 0;
  
  if (parent == NULL)
    {
      ot->base_scale_factor = 1.0;
      ot->base_font_size = 0;
      ot->has_base_font_size = FALSE;
      ot->scale_level = 0;
    }
  else
    {
      ot->base_scale_factor = parent->base_scale_factor;
      ot->base_font_size = parent->base_font_size;
      ot->has_base_font_size = parent->has_base_font_size;
      ot->scale_level = parent->scale_level;
    }
  
  md->tag_stack = g_slist_prepend (md->tag_stack, ot);

  return ot;
}

static void
markup_data_close_tag (MarkupData *md)
{
  OpenTag *ot;
  GSList *tmp_list;

  if (md->attr_list == NULL)
    return;

  /* pop the stack */
  ot = md->tag_stack->data;
  md->tag_stack = g_slist_delete_link (md->tag_stack,
                                       md->tag_stack);

  /* Adjust end indexes, and push each attr onto the front of the
   * to_apply list. This means that outermost tags are on the front of
   * that list; if we apply the list in order, then the innermost
   * tags will "win" which is correct.
   */
  tmp_list = ot->attrs;
  while (tmp_list != NULL)
    {
      PangoAttribute *a = tmp_list->data;

      a->start_index = ot->start_index;
      a->end_index = md->index;
      
      md->to_apply = g_slist_prepend (md->to_apply, a);

      tmp_list = g_slist_next (tmp_list);
    }

  if (ot->scale_level_delta != 0)
    {
      /* We affected relative font size; create an appropriate
       * attribute and reverse our effects on the current level
       */
      PangoAttribute *a;

      if (ot->has_base_font_size)
        {
          /* Create a font using the absolute point size
           * as the base size to be scaled from
           */
          a = pango_attr_size_new (scale_factor (ot->scale_level,
                                                 1.0) *
                                   ot->base_font_size);
        }
      else
        {
          /* Create a font using the current scale factor
           * as the base size to be scaled from
           */
          a = pango_attr_scale_new (scale_factor (ot->scale_level,
                                                  ot->base_scale_factor));
        }

      a->start_index = ot->start_index;
      a->end_index = md->index;
          
      md->to_apply = g_slist_prepend (md->to_apply, a);
    }
  
  g_slist_free (ot->attrs);
  g_free (ot);  
}

static void
start_element_handler  (GMarkupParseContext *context,
                        const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values,
                        gpointer             user_data,
                        GError             **error)
{
  TagParseFunc parse_func = NULL;
  OpenTag *ot;

  switch (*element_name)
    {
    case 'b':
      if (strcmp ("b", element_name) == 0)
        parse_func = b_parse_func;
      else if (strcmp ("big", element_name) == 0)
        parse_func = big_parse_func;
      break;

    case 'i':
      if (strcmp ("i", element_name) == 0)
        parse_func = i_parse_func;
      break;

    case 'm':
      if (strcmp ("markup", element_name) == 0)
        parse_func = markup_parse_func;
      break;

    case 's':
      if (strcmp ("span", element_name) == 0)
        parse_func = span_parse_func;
      else if (strcmp ("s", element_name) == 0)
        parse_func = s_parse_func;
      else if (strcmp ("sub", element_name) == 0)
        parse_func = sub_parse_func;
      else if (strcmp ("sup", element_name) == 0)
        parse_func = sup_parse_func;
      else if (strcmp ("small", element_name) == 0)
        parse_func = small_parse_func;
      break;

    case 't':
      if (strcmp ("tt", element_name) == 0)
        parse_func = tt_parse_func;
      break;

    case 'u':
      if (strcmp ("u", element_name) == 0)
        parse_func = u_parse_func;
      break;
    }

  if (parse_func == NULL)
    {
      gint line_number, char_number;

      g_markup_parse_context_get_position (context,
                                           &line_number, &char_number);

      g_set_error (error,
                   G_MARKUP_ERROR,
                   G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                   _("Unknown tag '%s' on line %d char %d"),
                   element_name,
                   line_number, char_number);

      return;
    }

  ot = markup_data_open_tag (user_data);

  /* note ot may be NULL if the user didn't want the attribute list */

  if (!(*parse_func) (user_data, ot,
                      attribute_names, attribute_values,
                      context, error))
    {
      /* there's nothing to do; we return an error, and end up
       * freeing ot off the tag stack later.
       */
    }
}

static void
end_element_handler    (GMarkupParseContext *context,
                        const gchar         *element_name,
                        gpointer             user_data,
                        GError             **error)
{
  markup_data_close_tag (user_data);
}

static void
text_handler           (GMarkupParseContext *context,
                        const gchar         *text,
                        gint                 text_len,
                        gpointer             user_data,
                        GError             **error)
{
  MarkupData *md = user_data;

  if (md->accel_marker == 0)
    {
      /* Just append all the text */
      
      md->index += text_len;
      
      if (md->text)
        g_string_append_len (md->text, text, text_len);
    }
  else
    {
      /* Parse the accelerator */
      const gchar *p;
      const gchar *end;
      const gchar *range_start;
      const gchar *range_end;
      gboolean just_saw_marker;
      gint uline_index = -1;
      gint uline_len = -1;
      
      range_end = NULL;
      range_start = text;
      p = text;
      end = text + text_len;
      just_saw_marker = FALSE;
      
      while (p != end)
        {
          gunichar c;
          
          c = g_utf8_get_char (p);

          if (range_end)
            {
              if (c == md->accel_marker)
                {
                  /* escaped accel marker; move range_end
                   * past the accel marker that came before,
                   * append the whole thing
                   */
                  range_end = g_utf8_next_char (range_end);
                  g_string_append_len (md->text,
                                       range_start,
                                       range_end - range_start);
                  md->index += range_end - range_start;

                  /* set next range_start, skipping accel marker */
                  range_start = g_utf8_next_char (p);
                }
              else
                {
                  /* Don't append the accel marker (leave range_end
                   * alone); set the accel char to c; record location for
                   * underline attribute
                   */
                  if (md->accel_char == 0)
                    md->accel_char = c;

                  g_string_append_len (md->text,
                                       range_start,
                                       range_end - range_start);
                  md->index += range_end - range_start;

                  /* The underline should go underneath the char
                   * we're setting as the next range_start
                   */
                  uline_index = md->index;
                  uline_len = g_utf8_next_char (p) - p;
                  
                  /* set next range_start to include this char */
                  range_start = p;
                }

              /* reset range_end */
              range_end = NULL;
            }          
          else if (c == md->accel_marker)
            {
              range_end = p;
            }
          
          p = g_utf8_next_char (p);
        }

      if (range_end)
        {
          g_string_append_len (md->text,
                               range_start,
                               range_end - range_start);
          md->index += range_end - range_start;
        }
      else
        {
          g_string_append_len (md->text,
                               range_start,
                               end - range_start);
          md->index += end - range_start;
        }
      
      if (md->attr_list != NULL && uline_index >= 0)
        {
          /* Add the underline indicating the accelerator */          
          PangoAttribute *attr;

          attr = pango_attr_underline_new (PANGO_UNDERLINE_LOW);

          attr->start_index = uline_index;
          attr->end_index = uline_index + uline_len;
          
          pango_attr_list_change (md->attr_list, attr);
        }
    }
}

static GMarkupParser pango_markup_parser = {
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

/**
 * pango_parse_markup:
 * @markup_text: markup to parse (see <link linkend="PangoMarkupFormat">markup format</link>)
 * @length: length of @markup_text, or -1 if nul-terminated
 * @accel_marker: character that precedes an accelerator, or 0 for none
 * @attr_list: address of return location for a #PangoAttrList, or NULL
 * @text: address of return location for text with tags stripped, or NULL
 * @accel_char: address of return location for accelerator char, or NULL
 * @error: address of return location for errors, or NULL
 * 
 *
 * Parses marked-up text (see
 * <link linkend="PangoMarkupFormat">markup format</link>) to create
 * a plaintext string and an attribute list.
 *
 * If @accel_marker is nonzero, the given character will mark the
 * character following it as an accelerator. For example, the accel
 * marker might be an ampersand or underscore. All characters marked
 * as an accelerator will receive a %PANGO_UNDERLINE_LOW attribute,
 * and the first character so marked will be returned in @accel_char.
 * Two @accel_marker characters following each other produce a single
 * literal @accel_marker character.
 * 
 * Return value: FALSE if @error is set, otherwise TRUE
 **/
gboolean
pango_parse_markup (const char                 *markup_text,
                    int                         length,
                    gunichar                    accel_marker,
                    PangoAttrList             **attr_list,
                    char                      **text,
                    gunichar                   *accel_char,
                    GError                    **error)
{
  GMarkupParseContext *context = NULL;
  MarkupData *md = NULL;
  gboolean needs_root = TRUE;
  GSList *tmp_list;
  const char *p;
  const char *end;
  
  g_return_val_if_fail (markup_text != NULL, FALSE);
  
  md = g_new (MarkupData, 1);

  /* Don't bother creating these if they weren't requested;
   * might be useful e.g. if you just want to validate
   * some markup.
   */
  if (attr_list)
    md->attr_list = pango_attr_list_new ();

  if (text)
    md->text = g_string_new ("");
  
  if (accel_char)
    *accel_char = 0;

  md->accel_marker = accel_marker;
  md->accel_char = 0;
  
  md->index = 0;
  md->tag_stack = NULL;
  md->to_apply = NULL;
  
  context = g_markup_parse_context_new (&pango_markup_parser,
                                        0, md, NULL);

  if (length < 0)
    length = strlen (markup_text);  

  p = markup_text;
  end = markup_text + length;
  while (p != end && isspace (*p))
    ++p;

  if (strncmp (p, "<markup>", end - p) == 0)
    needs_root = FALSE;

  if (needs_root)
    if (!g_markup_parse_context_parse (context,
                                       "<markup>",
                                       -1,
                                       error))
      goto error;


  if (!g_markup_parse_context_parse (context,
                                     markup_text,
                                     length,
                                     error))
    goto error;

  if (needs_root)
    if (!g_markup_parse_context_parse (context,
                                       "</markup>",
                                       -1,
                                       error))
      goto error;

  if (!g_markup_parse_context_end_parse (context, error))
    goto error;

  g_markup_parse_context_free (context);

  if (md->attr_list)
    {
      /* The apply list has the most-recently-closed tags first;
       * we want to apply the least-recently-closed tag last.
       */
      tmp_list = md->to_apply;
      while (tmp_list != NULL)
        {
          PangoAttribute *attr = tmp_list->data;
          
          /* Innermost tags before outermost */
          pango_attr_list_change (md->attr_list, attr);

          tmp_list = g_slist_next (tmp_list);
        }
      g_slist_free (md->to_apply);
      md->to_apply = NULL;
    }
  
  if (attr_list)
    *attr_list = md->attr_list;

  if (text)
    *text = g_string_free (md->text, FALSE);

  g_assert (md->tag_stack == NULL);
  
  g_free (md);

  return TRUE;

 error:
  g_slist_foreach (md->tag_stack, (GFunc) open_tag_free, NULL);
  g_slist_free (md->tag_stack);
  g_slist_foreach (md->to_apply, (GFunc) pango_attribute_destroy, NULL);
  g_slist_free (md->to_apply);

  if (md->text)
    g_string_free (md->text, TRUE);
  if (md->attr_list)
    pango_attr_list_unref (md->attr_list);

  g_free (md);

  if (context)
    g_markup_parse_context_free (context);

  return FALSE;
}

static void
set_bad_attribute (GError             **error,
                   GMarkupParseContext *context,
                   const char          *element_name,
                   const char          *attribute_name)
{
  gint line_number, char_number;

  g_markup_parse_context_get_position (context,
                                       &line_number, &char_number);

  g_set_error (error,
               G_MARKUP_ERROR,
               G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
               _("Tag '%s' does not support attribute '%s' on line %d char %d"),
               element_name,
               attribute_name,
               line_number, char_number);
}

static void
add_attribute (OpenTag        *ot,
               PangoAttribute *attr)
{
  if (ot == NULL)
    pango_attribute_destroy (attr);
  else
    ot->attrs = g_slist_prepend (ot->attrs, attr);
}

#define CHECK_NO_ATTRS(elem) G_STMT_START {                    \
         if (*names != NULL) {                                 \
           set_bad_attribute (error, context, (elem), *names); \
           return FALSE;                                       \
         } }G_STMT_END

static gboolean
b_parse_func        (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("b");
  add_attribute (tag, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  return TRUE;
}

static gboolean
big_parse_func      (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("big");

  /* Grow text one level */
  tag->scale_level_delta += 1;
  tag->scale_level += 1;

  return TRUE;
}

static gboolean
parse_absolute_size (OpenTag               *tag,
                     const char            *size)
{
  SizeLevel level = Medium;
  double factor;
  
  if (strcmp (size, "xx-small") == 0)
    {
      level = XXSmall;
    }
  else if (strcmp (size, "x-small") == 0)
    {
      level = XSmall;
    }
  else if (strcmp (size, "small") == 0)
    {
      level = Small;
    }
  else if (strcmp (size, "medium") == 0)
    {
      level = Medium;
    }
  else if (strcmp (size, "large") == 0)
    {
      level = Large;
    }
  else if (strcmp (size, "x-large") == 0)
    {
      level = XLarge;
    }
  else if (strcmp (size, "xx-large") == 0)
    {
      level = XXLarge;
    }
  else
    return FALSE;

  /* This is "absolute" in that it's relative to the base font,
   * but not to sizes created by any other tags
   */
  factor = scale_factor (level, 1.0);
  add_attribute (tag, pango_attr_scale_new (factor));
  open_tag_set_absolute_font_scale (tag, factor);
  
  return TRUE;
}

#define CHECK_DUPLICATE(var) G_STMT_START{                              \
          if ((var) != NULL) {                                          \
            g_set_error (error, G_MARKUP_ERROR,                         \
                         G_MARKUP_ERROR_INVALID_CONTENT,                \
                         _("Attribute '%s' occurs twice on <span> tag " \
                           "on line %d char %d, may only occur once"),  \
                         names[i], line_number, char_number);           \
            return FALSE;                                               \
          }}G_STMT_END

static gboolean
span_parse_func     (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  int line_number, char_number;
  int i;
  const char *family = NULL;
  const char *size = NULL;
  const char *style = NULL;
  const char *weight = NULL;
  const char *variant = NULL;
  const char *stretch = NULL;
  const char *desc = NULL;
  const char *foreground = NULL;
  const char *background = NULL;
  const char *underline = NULL;
  const char *strikethrough = NULL;
  const char *rise = NULL;
  const char *lang = NULL;
  
  g_markup_parse_context_get_position (context,
                                       &line_number, &char_number);

  i = 0;
  while (names[i])
    {
      if (strcmp (names[i], "font_family") == 0 ||
          strcmp (names[i], "face") == 0)
        {
          CHECK_DUPLICATE (family);
          family = values[i];
        }
      else if (strcmp (names[i], "size") == 0)
        {
          CHECK_DUPLICATE (size);
          size = values[i];
        }
      else if (strcmp (names[i], "style") == 0)
        {
          CHECK_DUPLICATE (style);
          style = values[i];
        }
      else if (strcmp (names[i], "weight") == 0)
        {
          CHECK_DUPLICATE (weight);
          weight = values[i];
        }
      else if (strcmp (names[i], "variant") == 0)
        {
          CHECK_DUPLICATE (variant);
          variant = values[i];
        }
      else if (strcmp (names[i], "stretch") == 0)
        {
          CHECK_DUPLICATE (stretch);
          stretch = values[i];
        }
      else if (strcmp (names[i], "font_desc") == 0)
        {
          CHECK_DUPLICATE (desc);
          desc = values[i];
        }
      else if (strcmp (names[i], "foreground") == 0 ||
               strcmp (names[i], "color") == 0)
        {
          CHECK_DUPLICATE (foreground);
          foreground = values[i];
        }
      else if (strcmp (names[i], "background") == 0)
        {
          CHECK_DUPLICATE (background);
          background = values[i];
        }
      else if (strcmp (names[i], "underline") == 0)
        {
          CHECK_DUPLICATE (underline);
          underline = values[i];
        }
      else if (strcmp (names[i], "strikethrough") == 0)
        {
          CHECK_DUPLICATE (strikethrough);
          strikethrough = values[i];
        }
      else if (strcmp (names[i], "rise") == 0)
        {
          CHECK_DUPLICATE (rise);
          rise = values[i];
        }
      else if (strcmp (names[i], "lang") == 0)
        {
          CHECK_DUPLICATE (lang);
          lang = values[i];
        }
      else
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                       _("Attribute '%s' is not allowed on the <span> tag "
                         "on line %d char %d"),
                       names[i], line_number, char_number);
          return FALSE;
        }

      ++i;
    }

  /* Parse desc first, then modify it with other font-related attributes. */
  if (desc)
    {
      PangoFontDescription *parsed;

      parsed = pango_font_description_from_string (desc);
      if (parsed)
        {
          add_attribute (tag, pango_attr_font_desc_new (parsed));
          open_tag_set_absolute_font_size (tag, parsed->size);
          pango_font_description_free (parsed);
        }
    }

  if (family)
    {
      add_attribute (tag, pango_attr_family_new (family));
    }

  if (size)
    {
      if (isdigit (*size))
        {
          char *end = NULL;
          gulong n;

          n = strtoul (size, &end, 10);

          if (*end != '\0')
            {
              g_set_error (error,
                           G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("Value of 'size' attribute on <span> tag on line %d"
                             "could not be parsed; should be an integer, or a "
                             "string such as 'small', not '%s'"),
                           line_number, size);
              goto error;
            }

          add_attribute (tag, pango_attr_size_new (n));
          open_tag_set_absolute_font_size (tag, n);
        }
      else if (strcmp (size, "smaller") == 0)
        {
          tag->scale_level_delta -= 1;
          tag->scale_level -= 1;
        }
      else if (strcmp (size, "larger") == 0)
        {
          tag->scale_level_delta += 1;
          tag->scale_level += 1;
        }
      else if (parse_absolute_size (tag, size))
        ; /* nothing */
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Value of 'size' attribute on <span> tag on line %d"
                         "could not be parsed; should be an integer, or a "
                         "string such as 'small', not '%s'"),
                       line_number, size);
          goto error;
        }
    }

  if (style)
    {
      PangoFontDescription desc;
      
      if (pango_parse_style (style, &desc, FALSE))
        add_attribute (tag, pango_attr_style_new (desc.style));
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("'%s' is not a valid value for the 'style' attribute "
                         "on <span> tag, line %d; valid values are "
                         "'normal', 'oblique', 'italic'"),
                       style, line_number);
          goto error;
        }
    }

  if (weight)
    {
      PangoFontDescription desc;
      
      if (pango_parse_weight (weight, &desc, FALSE))
        add_attribute (tag,
                       pango_attr_weight_new (desc.weight));
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("'%s' is not a valid value for the 'weight' "
                         "attribute on <span> tag, line %d; valid "
                         "values are for example 'light', 'ultrabold' or a number"),
                       weight, line_number);
          goto error;
        }
    }

  if (variant)
    {
      PangoFontDescription desc;
      
      if (pango_parse_variant (variant, &desc, FALSE))
        add_attribute (tag, pango_attr_variant_new (desc.variant));
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("'%s' is not a valid value for the 'variant' "
                         "attribute on <span> tag, line %d; valid values are "
                         "'normal', 'smallcaps'"),
                       variant, line_number);
          goto error;
        }
    }

  if (stretch)
    {
      PangoFontDescription desc;
      
      if (pango_parse_stretch (stretch, &desc, FALSE))
        add_attribute (tag, pango_attr_stretch_new (desc.stretch));
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("'%s' is not a valid value for the 'stretch' "
                         "attribute on <span> tag, line %d; valid "
                         "values are for example 'condensed', "
                         "'ultraexpanded', 'normal'"),
                       stretch, line_number);
          goto error;
        }
    }

  if (foreground)
    {
      guint16 red, green, blue;

      if (!pango_color_parse (foreground, &red, &green, &blue))
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Could not parse foreground color specification "
                         "'%s' on line %d"),
                       foreground, line_number);
          goto error;
        }

      add_attribute (tag, pango_attr_foreground_new (red, green, blue));
    }

  if (background)
    {
      guint16 red, green, blue;

      if (!pango_color_parse (background, &red, &green, &blue))
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Could not parse background color specification "
                         "'%s' on line %d"),
                       background, line_number);
          goto error;
        }

      add_attribute (tag, pango_attr_background_new (red, green, blue));
    }

  if (underline)
    {
      PangoUnderline ul = PANGO_UNDERLINE_NONE;

      if (strcmp (underline, "single") == 0)
        ul = PANGO_UNDERLINE_SINGLE;
      else if (strcmp (underline, "double") == 0)
        ul = PANGO_UNDERLINE_DOUBLE;
      else if (strcmp (underline, "low") == 0)
        ul = PANGO_UNDERLINE_LOW;
      else if (strcmp (underline, "none") == 0)
        ul = PANGO_UNDERLINE_NONE;
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("'%s' is not a valid value for the 'underline' "
                         "attribute on <span> tag, line %d; valid "
                         "values are for example 'single', "
                         "'double', 'low', 'none'"),
                       underline, line_number);
          goto error;
        }

      add_attribute (tag, pango_attr_underline_new (ul));
    }

  if (strikethrough)
    {
      if (strcmp (strikethrough, "true") == 0)
        add_attribute (tag, pango_attr_strikethrough_new (TRUE));
      else if (strcmp (strikethrough, "false") == 0)
        add_attribute (tag, pango_attr_strikethrough_new (FALSE));
      else
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("'strikethrough' attribute on <span> tag "
                         "line %d should have one of the values "
                         "'true' or 'false': '%s' is not valid"),
                       line_number, strikethrough);
          goto error;
        }
    }

  if (rise)
    {
      char *end = NULL;
      glong n;
      
      n = strtol (weight, &end, 10);

      if (*end != '\0')
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Value of 'rise' attribute on <span> tag "
                         "on line %d could not be parsed; "
                         "should be an integer, not '%s'"),
                       line_number, rise);
          goto error;
        }

      add_attribute (tag, pango_attr_rise_new (n));
    }

  if (lang)
    {
      add_attribute (tag, pango_attr_lang_new (lang));
    }
  
  return TRUE;

 error:

  return FALSE;
}

static gboolean
i_parse_func        (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("i");
  add_attribute (tag, pango_attr_style_new (PANGO_STYLE_ITALIC));

  return TRUE;
}

static gboolean
markup_parse_func (MarkupData            *md,
                   OpenTag               *tag,
                   const gchar          **names,
                   const gchar          **values,
                   GMarkupParseContext   *context,
                   GError               **error)
{
  /* We don't do anything with this tag at the moment. */
  
  return TRUE;
}

static gboolean
s_parse_func        (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("s");
  add_attribute (tag, pango_attr_strikethrough_new (TRUE));

  return TRUE;
}

#define SUPERSUB_RISE 5000

static gboolean
sub_parse_func      (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("sub");

  /* Shrink font, and set a negative rise */
  tag->scale_level_delta -= 1;
  tag->scale_level -= 1;

  add_attribute (tag, pango_attr_rise_new (-SUPERSUB_RISE));

  return TRUE;
}

static gboolean
sup_parse_func      (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("sup");

  /* Shrink font, and set a positive rise */
  tag->scale_level_delta -= 1;
  tag->scale_level -= 1;

  add_attribute (tag, pango_attr_rise_new (SUPERSUB_RISE));

  return TRUE;
}

static gboolean
small_parse_func    (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("small");

  /* Shrink text one level */
  tag->scale_level_delta -= 1;
  tag->scale_level -= 1;

  return TRUE;
}

static gboolean
tt_parse_func       (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("tt");

  add_attribute (tag, pango_attr_family_new ("Monospace"));

  return TRUE;
}

static gboolean
u_parse_func        (MarkupData            *md,
                     OpenTag               *tag,
                     const gchar          **names,
                     const gchar          **values,
                     GMarkupParseContext   *context,
                     GError               **error)
{
  CHECK_NO_ATTRS("u");
  add_attribute (tag, pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));

  return TRUE;
}

/* Color parsing
 */

/* The following 2 routines (parse_color, find_color) come from Tk, via the Win32
 * port of GDK. The licensing terms on these (longer than the functions) is:
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The following
 * terms apply to all files associated with the software unless explicitly
 * disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 * FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 * DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 * IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 * NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense, the
 * software shall be classified as "Commercial Computer Software" and the
 * Government shall have only "Restricted Rights" as defined in Clause
 * 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 * authors grant the U.S. Government and others acting in its behalf
 * permission to use and distribute the software in accordance with the
 * terms specified in this license.
 */

typedef struct {
    const char *name;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} ColorEntry;

static ColorEntry xColors[] = {
  { "alice blue", 240, 248, 255 },
  { "AliceBlue", 240, 248, 255 },
  { "antique white", 250, 235, 215 },
  { "AntiqueWhite", 250, 235, 215 },
  { "AntiqueWhite1", 255, 239, 219 },
  { "AntiqueWhite2", 238, 223, 204 },
  { "AntiqueWhite3", 205, 192, 176 },
  { "AntiqueWhite4", 139, 131, 120 },
  { "aquamarine", 127, 255, 212 },
  { "aquamarine1", 127, 255, 212 },
  { "aquamarine2", 118, 238, 198 },
  { "aquamarine3", 102, 205, 170 },
  { "aquamarine4", 69, 139, 116 },
  { "azure", 240, 255, 255 },
  { "azure1", 240, 255, 255 },
  { "azure2", 224, 238, 238 },
  { "azure3", 193, 205, 205 },
  { "azure4", 131, 139, 139 },
  { "beige", 245, 245, 220 },
  { "bisque", 255, 228, 196 },
  { "bisque1", 255, 228, 196 },
  { "bisque2", 238, 213, 183 },
  { "bisque3", 205, 183, 158 },
  { "bisque4", 139, 125, 107 },
  { "black", 0, 0, 0 },
  { "blanched almond", 255, 235, 205 },
  { "BlanchedAlmond", 255, 235, 205 },
  { "blue", 0, 0, 255 },
  { "blue violet", 138, 43, 226 },
  { "blue1", 0, 0, 255 },
  { "blue2", 0, 0, 238 },
  { "blue3", 0, 0, 205 },
  { "blue4", 0, 0, 139 },
  { "BlueViolet", 138, 43, 226 },
  { "brown", 165, 42, 42 },
  { "brown1", 255, 64, 64 },
  { "brown2", 238, 59, 59 },
  { "brown3", 205, 51, 51 },
  { "brown4", 139, 35, 35 },
  { "burlywood", 222, 184, 135 },
  { "burlywood1", 255, 211, 155 },
  { "burlywood2", 238, 197, 145 },
  { "burlywood3", 205, 170, 125 },
  { "burlywood4", 139, 115, 85 },
  { "cadet blue", 95, 158, 160 },
  { "CadetBlue", 95, 158, 160 },
  { "CadetBlue1", 152, 245, 255 },
  { "CadetBlue2", 142, 229, 238 },
  { "CadetBlue3", 122, 197, 205 },
  { "CadetBlue4", 83, 134, 139 },
  { "chartreuse", 127, 255, 0 },
  { "chartreuse1", 127, 255, 0 },
  { "chartreuse2", 118, 238, 0 },
  { "chartreuse3", 102, 205, 0 },
  { "chartreuse4", 69, 139, 0 },
  { "chocolate", 210, 105, 30 },
  { "chocolate1", 255, 127, 36 },
  { "chocolate2", 238, 118, 33 },
  { "chocolate3", 205, 102, 29 },
  { "chocolate4", 139, 69, 19 },
  { "coral", 255, 127, 80 },
  { "coral1", 255, 114, 86 },
  { "coral2", 238, 106, 80 },
  { "coral3", 205, 91, 69 },
  { "coral4", 139, 62, 47 },
  { "cornflower blue", 100, 149, 237 },
  { "CornflowerBlue", 100, 149, 237 },
  { "cornsilk", 255, 248, 220 },
  { "cornsilk1", 255, 248, 220 },
  { "cornsilk2", 238, 232, 205 },
  { "cornsilk3", 205, 200, 177 },
  { "cornsilk4", 139, 136, 120 },
  { "cyan", 0, 255, 255 },
  { "cyan1", 0, 255, 255 },
  { "cyan2", 0, 238, 238 },
  { "cyan3", 0, 205, 205 },
  { "cyan4", 0, 139, 139 },
  { "dark blue", 0, 0, 139 },
  { "dark cyan", 0, 139, 139 },
  { "dark goldenrod", 184, 134, 11 },
  { "dark gray", 169, 169, 169 },
  { "dark green", 0, 100, 0 },
  { "dark grey", 169, 169, 169 },
  { "dark khaki", 189, 183, 107 },
  { "dark magenta", 139, 0, 139 },
  { "dark olive green", 85, 107, 47 },
  { "dark orange", 255, 140, 0 },
  { "dark orchid", 153, 50, 204 },
  { "dark red", 139, 0, 0 },
  { "dark salmon", 233, 150, 122 },
  { "dark sea green", 143, 188, 143 },
  { "dark slate blue", 72, 61, 139 },
  { "dark slate gray", 47, 79, 79 },
  { "dark slate grey", 47, 79, 79 },
  { "dark turquoise", 0, 206, 209 },
  { "dark violet", 148, 0, 211 },
  { "DarkBlue", 0, 0, 139 },
  { "DarkCyan", 0, 139, 139 },
  { "DarkGoldenrod", 184, 134, 11 },
  { "DarkGoldenrod1", 255, 185, 15 },
  { "DarkGoldenrod2", 238, 173, 14 },
  { "DarkGoldenrod3", 205, 149, 12 },
  { "DarkGoldenrod4", 139, 101, 8 },
  { "DarkGray", 169, 169, 169 },
  { "DarkGreen", 0, 100, 0 },
  { "DarkGrey", 169, 169, 169 },
  { "DarkKhaki", 189, 183, 107 },
  { "DarkMagenta", 139, 0, 139 },
  { "DarkOliveGreen", 85, 107, 47 },
  { "DarkOliveGreen1", 202, 255, 112 },
  { "DarkOliveGreen2", 188, 238, 104 },
  { "DarkOliveGreen3", 162, 205, 90 },
  { "DarkOliveGreen4", 110, 139, 61 },
  { "DarkOrange", 255, 140, 0 },
  { "DarkOrange1", 255, 127, 0 },
  { "DarkOrange2", 238, 118, 0 },
  { "DarkOrange3", 205, 102, 0 },
  { "DarkOrange4", 139, 69, 0 },
  { "DarkOrchid", 153, 50, 204 },
  { "DarkOrchid1", 191, 62, 255 },
  { "DarkOrchid2", 178, 58, 238 },
  { "DarkOrchid3", 154, 50, 205 },
  { "DarkOrchid4", 104, 34, 139 },
  { "DarkRed", 139, 0, 0 },
  { "DarkSalmon", 233, 150, 122 },
  { "DarkSeaGreen", 143, 188, 143 },
  { "DarkSeaGreen1", 193, 255, 193 },
  { "DarkSeaGreen2", 180, 238, 180 },
  { "DarkSeaGreen3", 155, 205, 155 },
  { "DarkSeaGreen4", 105, 139, 105 },
  { "DarkSlateBlue", 72, 61, 139 },
  { "DarkSlateGray", 47, 79, 79 },
  { "DarkSlateGray1", 151, 255, 255 },
  { "DarkSlateGray2", 141, 238, 238 },
  { "DarkSlateGray3", 121, 205, 205 },
  { "DarkSlateGray4", 82, 139, 139 },
  { "DarkSlateGrey", 47, 79, 79 },
  { "DarkTurquoise", 0, 206, 209 },
  { "DarkViolet", 148, 0, 211 },
  { "deep pink", 255, 20, 147 },
  { "deep sky blue", 0, 191, 255 },
  { "DeepPink", 255, 20, 147 },
  { "DeepPink1", 255, 20, 147 },
  { "DeepPink2", 238, 18, 137 },
  { "DeepPink3", 205, 16, 118 },
  { "DeepPink4", 139, 10, 80 },
  { "DeepSkyBlue", 0, 191, 255 },
  { "DeepSkyBlue1", 0, 191, 255 },
  { "DeepSkyBlue2", 0, 178, 238 },
  { "DeepSkyBlue3", 0, 154, 205 },
  { "DeepSkyBlue4", 0, 104, 139 },
  { "dim gray", 105, 105, 105 },
  { "dim grey", 105, 105, 105 },
  { "DimGray", 105, 105, 105 },
  { "DimGrey", 105, 105, 105 },
  { "dodger blue", 30, 144, 255 },
  { "DodgerBlue", 30, 144, 255 },
  { "DodgerBlue1", 30, 144, 255 },
  { "DodgerBlue2", 28, 134, 238 },
  { "DodgerBlue3", 24, 116, 205 },
  { "DodgerBlue4", 16, 78, 139 },
  { "firebrick", 178, 34, 34 },
  { "firebrick1", 255, 48, 48 },
  { "firebrick2", 238, 44, 44 },
  { "firebrick3", 205, 38, 38 },
  { "firebrick4", 139, 26, 26 },
  { "floral white", 255, 250, 240 },
  { "FloralWhite", 255, 250, 240 },
  { "forest green", 34, 139, 34 },
  { "ForestGreen", 34, 139, 34 },
  { "gainsboro", 220, 220, 220 },
  { "ghost white", 248, 248, 255 },
  { "GhostWhite", 248, 248, 255 },
  { "gold", 255, 215, 0 },
  { "gold1", 255, 215, 0 },
  { "gold2", 238, 201, 0 },
  { "gold3", 205, 173, 0 },
  { "gold4", 139, 117, 0 },
  { "goldenrod", 218, 165, 32 },
  { "goldenrod1", 255, 193, 37 },
  { "goldenrod2", 238, 180, 34 },
  { "goldenrod3", 205, 155, 29 },
  { "goldenrod4", 139, 105, 20 },
  { "gray", 190, 190, 190 },
  { "gray0", 0, 0, 0 },
  { "gray1", 3, 3, 3 },
  { "gray10", 26, 26, 26 },
  { "gray100", 255, 255, 255 },
  { "gray11", 28, 28, 28 },
  { "gray12", 31, 31, 31 },
  { "gray13", 33, 33, 33 },
  { "gray14", 36, 36, 36 },
  { "gray15", 38, 38, 38 },
  { "gray16", 41, 41, 41 },
  { "gray17", 43, 43, 43 },
  { "gray18", 46, 46, 46 },
  { "gray19", 48, 48, 48 },
  { "gray2", 5, 5, 5 },
  { "gray20", 51, 51, 51 },
  { "gray21", 54, 54, 54 },
  { "gray22", 56, 56, 56 },
  { "gray23", 59, 59, 59 },
  { "gray24", 61, 61, 61 },
  { "gray25", 64, 64, 64 },
  { "gray26", 66, 66, 66 },
  { "gray27", 69, 69, 69 },
  { "gray28", 71, 71, 71 },
  { "gray29", 74, 74, 74 },
  { "gray3", 8, 8, 8 },
  { "gray30", 77, 77, 77 },
  { "gray31", 79, 79, 79 },
  { "gray32", 82, 82, 82 },
  { "gray33", 84, 84, 84 },
  { "gray34", 87, 87, 87 },
  { "gray35", 89, 89, 89 },
  { "gray36", 92, 92, 92 },
  { "gray37", 94, 94, 94 },
  { "gray38", 97, 97, 97 },
  { "gray39", 99, 99, 99 },
  { "gray4", 10, 10, 10 },
  { "gray40", 102, 102, 102 },
  { "gray41", 105, 105, 105 },
  { "gray42", 107, 107, 107 },
  { "gray43", 110, 110, 110 },
  { "gray44", 112, 112, 112 },
  { "gray45", 115, 115, 115 },
  { "gray46", 117, 117, 117 },
  { "gray47", 120, 120, 120 },
  { "gray48", 122, 122, 122 },
  { "gray49", 125, 125, 125 },
  { "gray5", 13, 13, 13 },
  { "gray50", 127, 127, 127 },
  { "gray51", 130, 130, 130 },
  { "gray52", 133, 133, 133 },
  { "gray53", 135, 135, 135 },
  { "gray54", 138, 138, 138 },
  { "gray55", 140, 140, 140 },
  { "gray56", 143, 143, 143 },
  { "gray57", 145, 145, 145 },
  { "gray58", 148, 148, 148 },
  { "gray59", 150, 150, 150 },
  { "gray6", 15, 15, 15 },
  { "gray60", 153, 153, 153 },
  { "gray61", 156, 156, 156 },
  { "gray62", 158, 158, 158 },
  { "gray63", 161, 161, 161 },
  { "gray64", 163, 163, 163 },
  { "gray65", 166, 166, 166 },
  { "gray66", 168, 168, 168 },
  { "gray67", 171, 171, 171 },
  { "gray68", 173, 173, 173 },
  { "gray69", 176, 176, 176 },
  { "gray7", 18, 18, 18 },
  { "gray70", 179, 179, 179 },
  { "gray71", 181, 181, 181 },
  { "gray72", 184, 184, 184 },
  { "gray73", 186, 186, 186 },
  { "gray74", 189, 189, 189 },
  { "gray75", 191, 191, 191 },
  { "gray76", 194, 194, 194 },
  { "gray77", 196, 196, 196 },
  { "gray78", 199, 199, 199 },
  { "gray79", 201, 201, 201 },
  { "gray8", 20, 20, 20 },
  { "gray80", 204, 204, 204 },
  { "gray81", 207, 207, 207 },
  { "gray82", 209, 209, 209 },
  { "gray83", 212, 212, 212 },
  { "gray84", 214, 214, 214 },
  { "gray85", 217, 217, 217 },
  { "gray86", 219, 219, 219 },
  { "gray87", 222, 222, 222 },
  { "gray88", 224, 224, 224 },
  { "gray89", 227, 227, 227 },
  { "gray9", 23, 23, 23 },
  { "gray90", 229, 229, 229 },
  { "gray91", 232, 232, 232 },
  { "gray92", 235, 235, 235 },
  { "gray93", 237, 237, 237 },
  { "gray94", 240, 240, 240 },
  { "gray95", 242, 242, 242 },
  { "gray96", 245, 245, 245 },
  { "gray97", 247, 247, 247 },
  { "gray98", 250, 250, 250 },
  { "gray99", 252, 252, 252 },
  { "green", 0, 255, 0 },
  { "green yellow", 173, 255, 47 },
  { "green1", 0, 255, 0 },
  { "green2", 0, 238, 0 },
  { "green3", 0, 205, 0 },
  { "green4", 0, 139, 0 },
  { "GreenYellow", 173, 255, 47 },
  { "grey", 190, 190, 190 },
  { "grey0", 0, 0, 0 },
  { "grey1", 3, 3, 3 },
  { "grey10", 26, 26, 26 },
  { "grey100", 255, 255, 255 },
  { "grey11", 28, 28, 28 },
  { "grey12", 31, 31, 31 },
  { "grey13", 33, 33, 33 },
  { "grey14", 36, 36, 36 },
  { "grey15", 38, 38, 38 },
  { "grey16", 41, 41, 41 },
  { "grey17", 43, 43, 43 },
  { "grey18", 46, 46, 46 },
  { "grey19", 48, 48, 48 },
  { "grey2", 5, 5, 5 },
  { "grey20", 51, 51, 51 },
  { "grey21", 54, 54, 54 },
  { "grey22", 56, 56, 56 },
  { "grey23", 59, 59, 59 },
  { "grey24", 61, 61, 61 },
  { "grey25", 64, 64, 64 },
  { "grey26", 66, 66, 66 },
  { "grey27", 69, 69, 69 },
  { "grey28", 71, 71, 71 },
  { "grey29", 74, 74, 74 },
  { "grey3", 8, 8, 8 },
  { "grey30", 77, 77, 77 },
  { "grey31", 79, 79, 79 },
  { "grey32", 82, 82, 82 },
  { "grey33", 84, 84, 84 },
  { "grey34", 87, 87, 87 },
  { "grey35", 89, 89, 89 },
  { "grey36", 92, 92, 92 },
  { "grey37", 94, 94, 94 },
  { "grey38", 97, 97, 97 },
  { "grey39", 99, 99, 99 },
  { "grey4", 10, 10, 10 },
  { "grey40", 102, 102, 102 },
  { "grey41", 105, 105, 105 },
  { "grey42", 107, 107, 107 },
  { "grey43", 110, 110, 110 },
  { "grey44", 112, 112, 112 },
  { "grey45", 115, 115, 115 },
  { "grey46", 117, 117, 117 },
  { "grey47", 120, 120, 120 },
  { "grey48", 122, 122, 122 },
  { "grey49", 125, 125, 125 },
  { "grey5", 13, 13, 13 },
  { "grey50", 127, 127, 127 },
  { "grey51", 130, 130, 130 },
  { "grey52", 133, 133, 133 },
  { "grey53", 135, 135, 135 },
  { "grey54", 138, 138, 138 },
  { "grey55", 140, 140, 140 },
  { "grey56", 143, 143, 143 },
  { "grey57", 145, 145, 145 },
  { "grey58", 148, 148, 148 },
  { "grey59", 150, 150, 150 },
  { "grey6", 15, 15, 15 },
  { "grey60", 153, 153, 153 },
  { "grey61", 156, 156, 156 },
  { "grey62", 158, 158, 158 },
  { "grey63", 161, 161, 161 },
  { "grey64", 163, 163, 163 },
  { "grey65", 166, 166, 166 },
  { "grey66", 168, 168, 168 },
  { "grey67", 171, 171, 171 },
  { "grey68", 173, 173, 173 },
  { "grey69", 176, 176, 176 },
  { "grey7", 18, 18, 18 },
  { "grey70", 179, 179, 179 },
  { "grey71", 181, 181, 181 },
  { "grey72", 184, 184, 184 },
  { "grey73", 186, 186, 186 },
  { "grey74", 189, 189, 189 },
  { "grey75", 191, 191, 191 },
  { "grey76", 194, 194, 194 },
  { "grey77", 196, 196, 196 },
  { "grey78", 199, 199, 199 },
  { "grey79", 201, 201, 201 },
  { "grey8", 20, 20, 20 },
  { "grey80", 204, 204, 204 },
  { "grey81", 207, 207, 207 },
  { "grey82", 209, 209, 209 },
  { "grey83", 212, 212, 212 },
  { "grey84", 214, 214, 214 },
  { "grey85", 217, 217, 217 },
  { "grey86", 219, 219, 219 },
  { "grey87", 222, 222, 222 },
  { "grey88", 224, 224, 224 },
  { "grey89", 227, 227, 227 },
  { "grey9", 23, 23, 23 },
  { "grey90", 229, 229, 229 },
  { "grey91", 232, 232, 232 },
  { "grey92", 235, 235, 235 },
  { "grey93", 237, 237, 237 },
  { "grey94", 240, 240, 240 },
  { "grey95", 242, 242, 242 },
  { "grey96", 245, 245, 245 },
  { "grey97", 247, 247, 247 },
  { "grey98", 250, 250, 250 },
  { "grey99", 252, 252, 252 },
  { "honeydew", 240, 255, 240 },
  { "honeydew1", 240, 255, 240 },
  { "honeydew2", 224, 238, 224 },
  { "honeydew3", 193, 205, 193 },
  { "honeydew4", 131, 139, 131 },
  { "hot pink", 255, 105, 180 },
  { "HotPink", 255, 105, 180 },
  { "HotPink1", 255, 110, 180 },
  { "HotPink2", 238, 106, 167 },
  { "HotPink3", 205, 96, 144 },
  { "HotPink4", 139, 58, 98 },
  { "indian red", 205, 92, 92 },
  { "IndianRed", 205, 92, 92 },
  { "IndianRed1", 255, 106, 106 },
  { "IndianRed2", 238, 99, 99 },
  { "IndianRed3", 205, 85, 85 },
  { "IndianRed4", 139, 58, 58 },
  { "ivory", 255, 255, 240 },
  { "ivory1", 255, 255, 240 },
  { "ivory2", 238, 238, 224 },
  { "ivory3", 205, 205, 193 },
  { "ivory4", 139, 139, 131 },
  { "khaki", 240, 230, 140 },
  { "khaki1", 255, 246, 143 },
  { "khaki2", 238, 230, 133 },
  { "khaki3", 205, 198, 115 },
  { "khaki4", 139, 134, 78 },
  { "lavender", 230, 230, 250 },
  { "lavender blush", 255, 240, 245 },
  { "LavenderBlush", 255, 240, 245 },
  { "LavenderBlush1", 255, 240, 245 },
  { "LavenderBlush2", 238, 224, 229 },
  { "LavenderBlush3", 205, 193, 197 },
  { "LavenderBlush4", 139, 131, 134 },
  { "lawn green", 124, 252, 0 },
  { "LawnGreen", 124, 252, 0 },
  { "lemon chiffon", 255, 250, 205 },
  { "LemonChiffon", 255, 250, 205 },
  { "LemonChiffon1", 255, 250, 205 },
  { "LemonChiffon2", 238, 233, 191 },
  { "LemonChiffon3", 205, 201, 165 },
  { "LemonChiffon4", 139, 137, 112 },
  { "light blue", 173, 216, 230 },
  { "light coral", 240, 128, 128 },
  { "light cyan", 224, 255, 255 },
  { "light goldenrod", 238, 221, 130 },
  { "light goldenrod yellow", 250, 250, 210 },
  { "light gray", 211, 211, 211 },
  { "light green", 144, 238, 144 },
  { "light grey", 211, 211, 211 },
  { "light pink", 255, 182, 193 },
  { "light salmon", 255, 160, 122 },
  { "light sea green", 32, 178, 170 },
  { "light sky blue", 135, 206, 250 },
  { "light slate blue", 132, 112, 255 },
  { "light slate gray", 119, 136, 153 },
  { "light slate grey", 119, 136, 153 },
  { "light steel blue", 176, 196, 222 },
  { "light yellow", 255, 255, 224 },
  { "LightBlue", 173, 216, 230 },
  { "LightBlue1", 191, 239, 255 },
  { "LightBlue2", 178, 223, 238 },
  { "LightBlue3", 154, 192, 205 },
  { "LightBlue4", 104, 131, 139 },
  { "LightCoral", 240, 128, 128 },
  { "LightCyan", 224, 255, 255 },
  { "LightCyan1", 224, 255, 255 },
  { "LightCyan2", 209, 238, 238 },
  { "LightCyan3", 180, 205, 205 },
  { "LightCyan4", 122, 139, 139 },
  { "LightGoldenrod", 238, 221, 130 },
  { "LightGoldenrod1", 255, 236, 139 },
  { "LightGoldenrod2", 238, 220, 130 },
  { "LightGoldenrod3", 205, 190, 112 },
  { "LightGoldenrod4", 139, 129, 76 },
  { "LightGoldenrodYellow", 250, 250, 210 },
  { "LightGray", 211, 211, 211 },
  { "LightGreen", 144, 238, 144 },
  { "LightGrey", 211, 211, 211 },
  { "LightPink", 255, 182, 193 },
  { "LightPink1", 255, 174, 185 },
  { "LightPink2", 238, 162, 173 },
  { "LightPink3", 205, 140, 149 },
  { "LightPink4", 139, 95, 101 },
  { "LightSalmon", 255, 160, 122 },
  { "LightSalmon1", 255, 160, 122 },
  { "LightSalmon2", 238, 149, 114 },
  { "LightSalmon3", 205, 129, 98 },
  { "LightSalmon4", 139, 87, 66 },
  { "LightSeaGreen", 32, 178, 170 },
  { "LightSkyBlue", 135, 206, 250 },
  { "LightSkyBlue1", 176, 226, 255 },
  { "LightSkyBlue2", 164, 211, 238 },
  { "LightSkyBlue3", 141, 182, 205 },
  { "LightSkyBlue4", 96, 123, 139 },
  { "LightSlateBlue", 132, 112, 255 },
  { "LightSlateGray", 119, 136, 153 },
  { "LightSlateGrey", 119, 136, 153 },
  { "LightSteelBlue", 176, 196, 222 },
  { "LightSteelBlue1", 202, 225, 255 },
  { "LightSteelBlue2", 188, 210, 238 },
  { "LightSteelBlue3", 162, 181, 205 },
  { "LightSteelBlue4", 110, 123, 139 },
  { "LightYellow", 255, 255, 224 },
  { "LightYellow1", 255, 255, 224 },
  { "LightYellow2", 238, 238, 209 },
  { "LightYellow3", 205, 205, 180 },
  { "LightYellow4", 139, 139, 122 },
  { "lime green", 50, 205, 50 },
  { "LimeGreen", 50, 205, 50 },
  { "linen", 250, 240, 230 },
  { "magenta", 255, 0, 255 },
  { "magenta1", 255, 0, 255 },
  { "magenta2", 238, 0, 238 },
  { "magenta3", 205, 0, 205 },
  { "magenta4", 139, 0, 139 },
  { "maroon", 176, 48, 96 },
  { "maroon1", 255, 52, 179 },
  { "maroon2", 238, 48, 167 },
  { "maroon3", 205, 41, 144 },
  { "maroon4", 139, 28, 98 },
  { "medium aquamarine", 102, 205, 170 },
  { "medium blue", 0, 0, 205 },
  { "medium orchid", 186, 85, 211 },
  { "medium purple", 147, 112, 219 },
  { "medium sea green", 60, 179, 113 },
  { "medium slate blue", 123, 104, 238 },
  { "medium spring green", 0, 250, 154 },
  { "medium turquoise", 72, 209, 204 },
  { "medium violet red", 199, 21, 133 },
  { "MediumAquamarine", 102, 205, 170 },
  { "MediumBlue", 0, 0, 205 },
  { "MediumOrchid", 186, 85, 211 },
  { "MediumOrchid1", 224, 102, 255 },
  { "MediumOrchid2", 209, 95, 238 },
  { "MediumOrchid3", 180, 82, 205 },
  { "MediumOrchid4", 122, 55, 139 },
  { "MediumPurple", 147, 112, 219 },
  { "MediumPurple1", 171, 130, 255 },
  { "MediumPurple2", 159, 121, 238 },
  { "MediumPurple3", 137, 104, 205 },
  { "MediumPurple4", 93, 71, 139 },
  { "MediumSeaGreen", 60, 179, 113 },
  { "MediumSlateBlue", 123, 104, 238 },
  { "MediumSpringGreen", 0, 250, 154 },
  { "MediumTurquoise", 72, 209, 204 },
  { "MediumVioletRed", 199, 21, 133 },
  { "midnight blue", 25, 25, 112 },
  { "MidnightBlue", 25, 25, 112 },
  { "mint cream", 245, 255, 250 },
  { "MintCream", 245, 255, 250 },
  { "misty rose", 255, 228, 225 },
  { "MistyRose", 255, 228, 225 },
  { "MistyRose1", 255, 228, 225 },
  { "MistyRose2", 238, 213, 210 },
  { "MistyRose3", 205, 183, 181 },
  { "MistyRose4", 139, 125, 123 },
  { "moccasin", 255, 228, 181 },
  { "navajo white", 255, 222, 173 },
  { "NavajoWhite", 255, 222, 173 },
  { "NavajoWhite1", 255, 222, 173 },
  { "NavajoWhite2", 238, 207, 161 },
  { "NavajoWhite3", 205, 179, 139 },
  { "NavajoWhite4", 139, 121, 94 },
  { "navy", 0, 0, 128 },
  { "navy blue", 0, 0, 128 },
  { "NavyBlue", 0, 0, 128 },
  { "old lace", 253, 245, 230 },
  { "OldLace", 253, 245, 230 },
  { "olive drab", 107, 142, 35 },
  { "OliveDrab", 107, 142, 35 },
  { "OliveDrab1", 192, 255, 62 },
  { "OliveDrab2", 179, 238, 58 },
  { "OliveDrab3", 154, 205, 50 },
  { "OliveDrab4", 105, 139, 34 },
  { "orange", 255, 165, 0 },
  { "orange red", 255, 69, 0 },
  { "orange1", 255, 165, 0 },
  { "orange2", 238, 154, 0 },
  { "orange3", 205, 133, 0 },
  { "orange4", 139, 90, 0 },
  { "OrangeRed", 255, 69, 0 },
  { "OrangeRed1", 255, 69, 0 },
  { "OrangeRed2", 238, 64, 0 },
  { "OrangeRed3", 205, 55, 0 },
  { "OrangeRed4", 139, 37, 0 },
  { "orchid", 218, 112, 214 },
  { "orchid1", 255, 131, 250 },
  { "orchid2", 238, 122, 233 },
  { "orchid3", 205, 105, 201 },
  { "orchid4", 139, 71, 137 },
  { "pale goldenrod", 238, 232, 170 },
  { "pale green", 152, 251, 152 },
  { "pale turquoise", 175, 238, 238 },
  { "pale violet red", 219, 112, 147 },
  { "PaleGoldenrod", 238, 232, 170 },
  { "PaleGreen", 152, 251, 152 },
  { "PaleGreen1", 154, 255, 154 },
  { "PaleGreen2", 144, 238, 144 },
  { "PaleGreen3", 124, 205, 124 },
  { "PaleGreen4", 84, 139, 84 },
  { "PaleTurquoise", 175, 238, 238 },
  { "PaleTurquoise1", 187, 255, 255 },
  { "PaleTurquoise2", 174, 238, 238 },
  { "PaleTurquoise3", 150, 205, 205 },
  { "PaleTurquoise4", 102, 139, 139 },
  { "PaleVioletRed", 219, 112, 147 },
  { "PaleVioletRed1", 255, 130, 171 },
  { "PaleVioletRed2", 238, 121, 159 },
  { "PaleVioletRed3", 205, 104, 137 },
  { "PaleVioletRed4", 139, 71, 93 },
  { "papaya whip", 255, 239, 213 },
  { "PapayaWhip", 255, 239, 213 },
  { "peach puff", 255, 218, 185 },
  { "PeachPuff", 255, 218, 185 },
  { "PeachPuff1", 255, 218, 185 },
  { "PeachPuff2", 238, 203, 173 },
  { "PeachPuff3", 205, 175, 149 },
  { "PeachPuff4", 139, 119, 101 },
  { "peru", 205, 133, 63 },
  { "pink", 255, 192, 203 },
  { "pink1", 255, 181, 197 },
  { "pink2", 238, 169, 184 },
  { "pink3", 205, 145, 158 },
  { "pink4", 139, 99, 108 },
  { "plum", 221, 160, 221 },
  { "plum1", 255, 187, 255 },
  { "plum2", 238, 174, 238 },
  { "plum3", 205, 150, 205 },
  { "plum4", 139, 102, 139 },
  { "powder blue", 176, 224, 230 },
  { "PowderBlue", 176, 224, 230 },
  { "purple", 160, 32, 240 },
  { "purple1", 155, 48, 255 },
  { "purple2", 145, 44, 238 },
  { "purple3", 125, 38, 205 },
  { "purple4", 85, 26, 139 },
  { "red", 255, 0, 0 },
  { "red1", 255, 0, 0 },
  { "red2", 238, 0, 0 },
  { "red3", 205, 0, 0 },
  { "red4", 139, 0, 0 },
  { "rosy brown", 188, 143, 143 },
  { "RosyBrown", 188, 143, 143 },
  { "RosyBrown1", 255, 193, 193 },
  { "RosyBrown2", 238, 180, 180 },
  { "RosyBrown3", 205, 155, 155 },
  { "RosyBrown4", 139, 105, 105 },
  { "royal blue", 65, 105, 225 },
  { "RoyalBlue", 65, 105, 225 },
  { "RoyalBlue1", 72, 118, 255 },
  { "RoyalBlue2", 67, 110, 238 },
  { "RoyalBlue3", 58, 95, 205 },
  { "RoyalBlue4", 39, 64, 139 },
  { "saddle brown", 139, 69, 19 },
  { "SaddleBrown", 139, 69, 19 },
  { "salmon", 250, 128, 114 },
  { "salmon1", 255, 140, 105 },
  { "salmon2", 238, 130, 98 },
  { "salmon3", 205, 112, 84 },
  { "salmon4", 139, 76, 57 },
  { "sandy brown", 244, 164, 96 },
  { "SandyBrown", 244, 164, 96 },
  { "sea green", 46, 139, 87 },
  { "SeaGreen", 46, 139, 87 },
  { "SeaGreen1", 84, 255, 159 },
  { "SeaGreen2", 78, 238, 148 },
  { "SeaGreen3", 67, 205, 128 },
  { "SeaGreen4", 46, 139, 87 },
  { "seashell", 255, 245, 238 },
  { "seashell1", 255, 245, 238 },
  { "seashell2", 238, 229, 222 },
  { "seashell3", 205, 197, 191 },
  { "seashell4", 139, 134, 130 },
  { "sienna", 160, 82, 45 },
  { "sienna1", 255, 130, 71 },
  { "sienna2", 238, 121, 66 },
  { "sienna3", 205, 104, 57 },
  { "sienna4", 139, 71, 38 },
  { "sky blue", 135, 206, 235 },
  { "SkyBlue", 135, 206, 235 },
  { "SkyBlue1", 135, 206, 255 },
  { "SkyBlue2", 126, 192, 238 },
  { "SkyBlue3", 108, 166, 205 },
  { "SkyBlue4", 74, 112, 139 },
  { "slate blue", 106, 90, 205 },
  { "slate gray", 112, 128, 144 },
  { "slate grey", 112, 128, 144 },
  { "SlateBlue", 106, 90, 205 },
  { "SlateBlue1", 131, 111, 255 },
  { "SlateBlue2", 122, 103, 238 },
  { "SlateBlue3", 105, 89, 205 },
  { "SlateBlue4", 71, 60, 139 },
  { "SlateGray", 112, 128, 144 },
  { "SlateGray1", 198, 226, 255 },
  { "SlateGray2", 185, 211, 238 },
  { "SlateGray3", 159, 182, 205 },
  { "SlateGray4", 108, 123, 139 },
  { "SlateGrey", 112, 128, 144 },
  { "snow", 255, 250, 250 },
  { "snow1", 255, 250, 250 },
  { "snow2", 238, 233, 233 },
  { "snow3", 205, 201, 201 },
  { "snow4", 139, 137, 137 },
  { "spring green", 0, 255, 127 },
  { "SpringGreen", 0, 255, 127 },
  { "SpringGreen1", 0, 255, 127 },
  { "SpringGreen2", 0, 238, 118 },
  { "SpringGreen3", 0, 205, 102 },
  { "SpringGreen4", 0, 139, 69 },
  { "steel blue", 70, 130, 180 },
  { "SteelBlue", 70, 130, 180 },
  { "SteelBlue1", 99, 184, 255 },
  { "SteelBlue2", 92, 172, 238 },
  { "SteelBlue3", 79, 148, 205 },
  { "SteelBlue4", 54, 100, 139 },
  { "tan", 210, 180, 140 },
  { "tan1", 255, 165, 79 },
  { "tan2", 238, 154, 73 },
  { "tan3", 205, 133, 63 },
  { "tan4", 139, 90, 43 },
  { "thistle", 216, 191, 216 },
  { "thistle1", 255, 225, 255 },
  { "thistle2", 238, 210, 238 },
  { "thistle3", 205, 181, 205 },
  { "thistle4", 139, 123, 139 },
  { "tomato", 255, 99, 71 },
  { "tomato1", 255, 99, 71 },
  { "tomato2", 238, 92, 66 },
  { "tomato3", 205, 79, 57 },
  { "tomato4", 139, 54, 38 },
  { "turquoise", 64, 224, 208 },
  { "turquoise1", 0, 245, 255 },
  { "turquoise2", 0, 229, 238 },
  { "turquoise3", 0, 197, 205 },
  { "turquoise4", 0, 134, 139 },
  { "violet", 238, 130, 238 },
  { "violet red", 208, 32, 144 },
  { "VioletRed", 208, 32, 144 },
  { "VioletRed1", 255, 62, 150 },
  { "VioletRed2", 238, 58, 140 },
  { "VioletRed3", 205, 50, 120 },
  { "VioletRed4", 139, 34, 82 },
  { "wheat", 245, 222, 179 },
  { "wheat1", 255, 231, 186 },
  { "wheat2", 238, 216, 174 },
  { "wheat3", 205, 186, 150 },
  { "wheat4", 139, 126, 102 },
  { "white", 255, 255, 255 },
  { "white smoke", 245, 245, 245 },
  { "WhiteSmoke", 245, 245, 245 },
  { "yellow", 255, 255, 0 },
  { "yellow green", 154, 205, 50 },
  { "yellow1", 255, 255, 0 },
  { "yellow2", 238, 238, 0 },
  { "yellow3", 205, 205, 0 },
  { "yellow4", 139, 139, 0 },
  { "YellowGreen", 154, 205, 50 }
};

static int
compare_xcolor_entries (const void *a, const void *b)
{
  return strcasecmp ((const char *) a, ((const ColorEntry *) b)->name);
}

static gboolean
find_color(const char *name,
           guint16    *red,
           guint16    *green,
           guint16    *blue)
{
  ColorEntry *found;

  found = bsearch (name, xColors, G_N_ELEMENTS (xColors),
                   sizeof (ColorEntry),
                   compare_xcolor_entries);
  if (found == NULL)
    return FALSE;

  *red = (found->red * 65535) / 255;
  *green = (found->green * 65535) / 255;
  *blue = (found->blue * 65535) / 255;

  return TRUE;
}

static gboolean
pango_color_parse (const char *spec,
                   guint16    *red,
                   guint16    *green,
                   guint16    *blue)
{
  if (spec[0] == '#')
    {
      char fmt[16];
      int i, r, g, b;

      if ((i = strlen (spec+1)) % 3)
        return FALSE;

      i /= 3;

      sprintf (fmt, "%%%dx%%%dx%%%dx", i, i, i);
      if (sscanf (spec+1, fmt, &r, &g, &b) != 3)
        return FALSE;

      if (i == 4)
        {
          if (red)
            *red = r;
          if (green)
            *green = g;
          if (blue)
            *blue = b;
        }
      else if (i == 1)
        {
          if (red)
            *red = (r * 65535) / 15;
          if (green)
            *green = (g * 65535) / 15;
          if (blue)
            *blue = (b * 65535) / 15;
        }
      else if (i == 2)
        {
          if (red)
            *red = (r * 65535) / 255;
          if (green)
            *green = (g * 65535) / 255;
          if (blue)
            *blue = (b * 65535) / 255;
        }
      else /* if (i == 3) */
        {
          if (red)
            *red = (r * 65535) / 4095;
          if (green)
            *green = (g * 65535) / 4095;
          if (blue)
            *blue = (b * 65535) / 4095;
        }
    }
  else
    {
      if (!find_color(spec, red, green, blue))
        return FALSE;
    }

  return TRUE;
}
