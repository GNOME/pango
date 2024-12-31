/* Pango2
 * testattributes.c: Test pango attributes
 *
 * Copyright (C) 2015 Red Hat, Inc.
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

#include <pango2/pango.h>
#include <pango2/pango-attr-private.h>

static void
test_copy (Pango2Attribute *attr)
{
  Pango2Attribute *a;

  a = pango2_attribute_copy (attr);
  g_assert_true (pango2_attribute_equal (attr, a));
  pango2_attribute_destroy (a);
  pango2_attribute_destroy (attr);
}

static void
test_attributes_basic (void)
{
  Pango2FontDescription *desc;

  test_copy (pango2_attr_language_new (pango2_language_from_string ("ja-JP")));
  test_copy (pango2_attr_family_new ("Times"));
  test_copy (pango2_attr_size_new (1024));
  test_copy (pango2_attr_size_new_absolute (1024));
  test_copy (pango2_attr_style_new (PANGO2_STYLE_ITALIC));
  test_copy (pango2_attr_weight_new (PANGO2_WEIGHT_ULTRALIGHT));
  test_copy (pango2_attr_variant_new (PANGO2_VARIANT_SMALL_CAPS));
  test_copy (pango2_attr_stretch_new (PANGO2_STRETCH_SEMI_EXPANDED));
  desc = pango2_font_description_from_string ("Computer Modern 12");
  test_copy (pango2_attr_font_desc_new (desc));
  pango2_font_description_free (desc);
  test_copy (pango2_attr_underline_new (PANGO2_LINE_STYLE_SOLID));
  test_copy (pango2_attr_underline_new (PANGO2_LINE_STYLE_DOTTED));
  test_copy (pango2_attr_underline_color_new (&(Pango2Color){100, 200, 300}));
  test_copy (pango2_attr_overline_new (PANGO2_LINE_STYLE_SOLID));
  test_copy (pango2_attr_overline_color_new (&(Pango2Color){100, 200, 300}));
  test_copy (pango2_attr_strikethrough_new (TRUE));
  test_copy (pango2_attr_strikethrough_color_new (&(Pango2Color){100, 200, 300}));
  test_copy (pango2_attr_rise_new (256));
  test_copy (pango2_attr_scale_new (2.56));
  test_copy (pango2_attr_fallback_new (FALSE));
  test_copy (pango2_attr_letter_spacing_new (1024));
  test_copy (pango2_attr_gravity_new (PANGO2_GRAVITY_SOUTH));
  test_copy (pango2_attr_gravity_hint_new (PANGO2_GRAVITY_HINT_STRONG));
  test_copy (pango2_attr_font_features_new ("csc=1"));
  test_copy (pango2_attr_allow_breaks_new (FALSE));
  test_copy (pango2_attr_show_new (PANGO2_SHOW_SPACES));
  test_copy (pango2_attr_insert_hyphens_new (FALSE));
  test_copy (pango2_attr_text_transform_new (PANGO2_TEXT_TRANSFORM_UPPERCASE));
  test_copy (pango2_attr_line_height_new (1.5));
  test_copy (pango2_attr_line_height_new_absolute (3000));
  test_copy (pango2_attr_word_new ());
  test_copy (pango2_attr_sentence_new ());
}

static void
test_attributes_equal (void)
{
  Pango2Attribute *attr1, *attr2, *attr3;

  /* check that pango2_attribute_equal compares values, but not ranges */
  attr1 = pango2_attr_size_new (10);
  attr2 = pango2_attr_size_new (20);
  attr3 = pango2_attr_size_new (20);
  attr3->start_index = 1;
  attr3->end_index = 2;

  g_assert_true (!pango2_attribute_equal (attr1, attr2));
  g_assert_true (pango2_attribute_equal (attr2, attr3));

  pango2_attribute_destroy (attr1);
  pango2_attribute_destroy (attr2);
  pango2_attribute_destroy (attr3);
}

static gpointer
copy_my_attribute_data (gconstpointer data)
{
  return (gpointer)data;
}

static void
destroy_my_attribute_data (gpointer data)
{
}

static gboolean
my_attribute_data_equal (gconstpointer data1,
                         gconstpointer data2)
{
  return data1 == data2;
}

static char *
my_attribute_data_serialize (gconstpointer data)
{
  return g_strdup_printf ("%#x", GPOINTER_TO_UINT (data));
}

static void
test_attributes_register (void)
{
  Pango2AttrType type;
  Pango2Attribute *attr;
  Pango2Attribute *attr2;
  Pango2AttrList *list;
  gboolean ret;
  char *str;

  type = pango2_attr_type_register ("my-attribute",
                                   PANGO2_ATTR_VALUE_POINTER,
                                   PANGO2_ATTR_AFFECTS_RENDERING,
                                   PANGO2_ATTR_MERGE_OVERRIDES,
                                   copy_my_attribute_data,
                                   destroy_my_attribute_data,
                                   my_attribute_data_equal,
                                   my_attribute_data_serialize);

  g_assert_cmpstr (pango2_attr_type_get_name (type), ==, "my-attribute");

  attr = pango2_attribute_new (type, (gpointer)0x42);

  g_assert_true (pango2_attribute_get_pointer (attr) == (gpointer)0x42);

  attr2 = pango2_attribute_new (type, (gpointer)0x43);

  ret = pango2_attribute_equal (attr, attr2);
  g_assert_false (ret);

  list = pango2_attr_list_new ();
  pango2_attr_list_insert (list, attr2);

  str = pango2_attr_list_to_string (list);
  g_assert_cmpstr (str, ==, "0 4294967295 my-attribute 0x43");
  g_free (str);

  pango2_attr_list_unref (list);

  pango2_attribute_destroy (attr);
}

static void
assert_attributes (GSList     *attrs,
                   const char *expected)
{
  Pango2AttrList *list2;
  GSList *attrs2, *l, *l2;

  list2 = pango2_attr_list_from_string (expected);
  attrs2 = pango2_attr_list_get_attributes (list2);

  if (g_slist_length (attrs) != g_slist_length (attrs2))
    g_assert_not_reached ();

  for (l = attrs, l2 = attrs2; l; l = l->next, l2 = l2->next)
    {
      Pango2Attribute *a = l->data;
      Pango2Attribute *a2 = l2->data;
      if (!pango2_attribute_equal (a, a2))
        g_assert_not_reached ();
    }

  g_slist_free_full (attrs2, (GDestroyNotify)pango2_attribute_destroy);
  pango2_attr_list_unref (list2);
}

static void
assert_attr_list_order (Pango2AttrList *list)
{
  GSList *attrs, *l;
  guint start = 0;

  attrs = pango2_attr_list_get_attributes (list);

  for (l = attrs; l; l = l->next)
    {
      Pango2Attribute *attr = l->data;
      g_assert_cmpuint (start, <=, attr->start_index);
      start = attr->start_index;
    }

  g_slist_free_full (attrs, (GDestroyNotify) pango2_attribute_destroy);
}

static void
assert_attr_list (Pango2AttrList *list,
                  const char    *expected)
{
  Pango2AttrList *list2;

  assert_attr_list_order (list);

  list2 = pango2_attr_list_from_string (expected);
  if (!pango2_attr_list_equal (list, list2))
    {
      char *s = pango2_attr_list_to_string (list);
      g_print ("-----\nattribute list mismatch\nexpected:\n%s\n-----\nreceived:\n%s\n-----\n",
               expected, s);
      g_free (s);
      g_assert_not_reached ();
    }

  pango2_attr_list_unref (list2);
}

static void
assert_attr_iterator (Pango2AttrIterator *iter,
                      const char        *expected)
{
  GSList *attrs;

  attrs = pango2_attr_iterator_get_attrs (iter);
  assert_attributes (attrs, expected);
  g_slist_free_full (attrs, (GDestroyNotify)pango2_attribute_destroy);
}

static Pango2Attribute *
attribute_from_string (const char *str)
{
  Pango2AttrList *list;
  GSList *attrs;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string (str);
  attrs = pango2_attr_list_get_attributes (list);
  g_assert_true (attrs->next == NULL);

  attr = attrs->data;

  g_slist_free (attrs);
  pango2_attr_list_unref (list);

  return attr;
}

static void
test_list (void)
{
  Pango2AttrList *list, *list2;
  Pango2Attribute *attr;

  list = pango2_attr_list_new ();

  attr = pango2_attr_size_new (10);
  pango2_attr_list_insert (list, attr);
  attr = pango2_attr_size_new (20);
  pango2_attr_list_insert (list, attr);
  attr = pango2_attr_size_new (30);
  pango2_attr_list_insert (list, attr);

  list2 = pango2_attr_list_from_string ("0 -1 size 10\n"
                                       "0 -1 size 20\n"
                                       "0 -1 size 30\n");
  g_assert_true (pango2_attr_list_equal (list, list2));
  pango2_attr_list_unref (list2);

  assert_attr_list (list, "0 -1 size 10\n"
                          "0 -1 size 20\n"
                          "0 -1 size 30");
  pango2_attr_list_unref (list);

  list = pango2_attr_list_new ();

  /* test that insertion respects start_index */
  attr = attribute_from_string ("0 -1 size 10");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 20 size 20");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("0 -1 size 30");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string (" 10 40 size 40");
  pango2_attr_list_insert_before (list, attr);

  assert_attr_list (list, "0 -1 size 10\n"
                          "0 -1 size 30\n"
                          "10 40 size 40\n"
                          "10 20 size 20");
  pango2_attr_list_unref (list);
}

static void
test_list_change (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string (" 0 10 size 10\n"
                                      " 0 30 weight 700\n"
                                      " 20 30 size 20\n");

  /* no-op */
  attr = attribute_from_string ("10 10 variant small-caps");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 size 10\n"
                          "0 30 weight bold\n"
                          "20 30 size 20");

  /* simple insertion with pango2_attr_list_change */
  attr = attribute_from_string ("10 20 variant small-caps");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 size 10\n"
                          "0 30 weight bold\n"
                          "10 20 variant small-caps\n"
                          "20 30 size 20");

  /* insertion with splitting */
  attr = attribute_from_string ("15 20 weight light");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 size 10\n"
                          "0 15 weight bold\n"
                          "10 20 variant small-caps\n"
                          "15 20 weight light\n"
                          "20 30 size 20\n"
                          "20 30 weight bold");

  /* insertion with joining */
  attr = attribute_from_string ("5 20 size 20");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 5 size 10\n"
                          "0 15 weight bold\n"
                          "5 30 size 20\n"
                          "10 20 variant small-caps\n"
                          "15 20 weight light\n"
                          "20 30 weight bold");

  pango2_attr_list_unref (list);
}

/* See https://gitlab.gnome.org/GNOME/pango/-/issues/564 */
static void
test_list_change2 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 style italic\n"
                                      "3 4 style normal\n"
                                      "4 18 style italic\n");

  /* insertion with joining */
  attr = attribute_from_string ("0 18 style normal");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 18 style normal");

  pango2_attr_list_unref (list);
}

static void
test_list_change3 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 style italic\n"
                                      "3 4 style normal\n"
                                      "4 18 style italic\n");

  /* insertion with joining */
  attr = attribute_from_string ("1 1 style normal");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 style italic\n"
                          "3 4 style normal\n"
                          "4 18 style italic");

  pango2_attr_list_unref (list);
}

static void
test_list_change4 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_new ();

  /* insertion with joining */
  attr = attribute_from_string ("0 10 style normal");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 style normal");

  pango2_attr_list_unref (list);
}

static void
test_list_change5 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n");

  /* insertion with joining */
  attr = attribute_from_string ("5 15 style italic");
  g_assert_true (attr->start_index == 5);
  g_assert_true (attr->end_index == 15);
  g_assert_true (attr->int_value == PANGO2_STYLE_ITALIC);
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 4 style normal\n"
                          "5 15 style italic\n"
                          "15 20 style normal\n"
                          "15 18 fallback false");

  pango2_attr_list_unref (list);
}

static void
test_list_change6 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n");

  /* insertion with joining */
  attr = attribute_from_string ("3 10 style normal");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 20 style normal\n"
                          "15 18 fallback false");

  pango2_attr_list_unref (list);
}

static void
test_list_change7 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n");

  /* insertion with joining */
  attr = attribute_from_string ("3 4 style normal");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 4 style normal\n"
                          "10 20 style normal\n"
                          "15 18 fallback false");

  pango2_attr_list_unref (list);
}

static void
test_list_change8 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n");

  /* insertion with joining */
  attr = attribute_from_string ("3 11 style italic");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 3 style normal\n"
                          "3 11 style italic\n"
                          "11 20 style normal\n"
                          "15 18 fallback false");

  pango2_attr_list_unref (list);
}

static void
test_list_change9 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n");

  /* insertion with joining */
  attr = attribute_from_string ("2 3 style italic");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 3 style italic\n"
                          "3 4 style normal\n"
                          "10 20 style normal\n"
                          "15 18 fallback false");

  pango2_attr_list_unref (list);
}

static void
test_list_change10 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n");

  /* insertion with joining */
  attr = attribute_from_string ("3 4 style italic");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 3 style normal\n"
                          "3 4 style italic\n"
                          "10 20 style normal\n"
                          "15 18 fallback false");

  pango2_attr_list_unref (list);
}

static void
test_list_change11 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "3 5 fallback false\n"
                                      "10 20 style italic\n"
                                      "15 18 fallback false\n"
                                      "22 30 style italic\n");

  /* insertion with joining */
  attr = attribute_from_string ("3 22 style italic");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 3 style normal\n"
                          "3 30 style italic\n"
                          "3 5 fallback false\n"
                          "15 18 fallback false\n");

  pango2_attr_list_unref (list);
}

static void
test_list_change12 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 3 weight 800\n"
                                      "2 4 style normal\n"
                                      "3 5 fallback false\n"
                                      "10 20 style normal\n"
                                      "15 18 fallback false\n"
                                      "20 30 style oblique\n"
                                      "21 22 fallback false");

  /* insertion with joining */
  attr = attribute_from_string ("3 22 style italic");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 3 weight ultrabold\n"
                          "2 3 style normal\n"
                          "3 22 style italic\n"
                          "3 5 fallback false\n"
                          "15 18 fallback false\n"
                          "21 22 fallback false\n"
                          "22 30 style oblique");

  pango2_attr_list_unref (list);
}

static void
test_list_splice (void)
{
  Pango2AttrList *base;
  Pango2AttrList *list;
  Pango2AttrList *other;

  base = pango2_attr_list_from_string ("0 -1 size 10\n"
                                      "10 15 weight 700\n"
                                      "20 30 variant 1\n");

  /* splice in an empty list */
  list = pango2_attr_list_copy (base);
  other = pango2_attr_list_new ();
  pango2_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "0 -1 size 10\n"
                          "10 20 weight bold\n"
                          "25 35 variant small-caps\n");

  pango2_attr_list_unref (list);
  pango2_attr_list_unref (other);

  /* splice in some attributes */
  list = pango2_attr_list_copy (base);

  other = pango2_attr_list_from_string ("0 3 size 20\n"
                                       "2 4 stretch condensed\n");

  pango2_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "0 11 size 10\n"
                          "10 20 weight bold\n"
                          "11 14 size 20\n"
                          "13 15 stretch condensed\n"
                          "14 -1 size 10\n"
                          "25 35 variant small-caps\n");

  pango2_attr_list_unref (list);
  pango2_attr_list_unref (other);

  pango2_attr_list_unref (base);
}

/* Test that empty lists work in pango2_attr_list_splice */
static void
test_list_splice2 (void)
{
  Pango2AttrList *list;
  Pango2AttrList *other;
  Pango2Attribute *attr;

  list = pango2_attr_list_new ();
  other = pango2_attr_list_new ();

  pango2_attr_list_splice (list, other, 11, 5);

  g_assert_null (pango2_attr_list_get_attributes (list));

  attr = attribute_from_string ("0 -1 size 10");
  pango2_attr_list_insert (other, attr);

  pango2_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "11 16 size 10\n");

  pango2_attr_list_unref (other);
  other = pango2_attr_list_new ();

  pango2_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "11 21 size 10\n");

  pango2_attr_list_unref (other);
  pango2_attr_list_unref (list);
}

/* Test that attributes in other aren't leaking out in pango2_attr_list_splice */
static void
test_list_splice3 (void)
{
  Pango2AttrList *list;
  Pango2AttrList *other;

  list = pango2_attr_list_from_string ("10 30 variant 1\n");
  other = pango2_attr_list_from_string ("0 -1 weight 700\n");

  pango2_attr_list_splice (list, other, 20, 5);

  assert_attr_list (list, "10 35 variant small-caps\n"
                          "20 25 weight bold\n");

  pango2_attr_list_unref (other);
  pango2_attr_list_unref (list);
}

static gboolean
never_true (Pango2Attribute *attribute, gpointer user_data)
{
  return FALSE;
}

static gboolean
just_weight (Pango2Attribute *attribute, gpointer user_data)
{
  if (attribute->type == PANGO2_ATTR_WEIGHT)
    return TRUE;
  else
    return FALSE;
}

static void
test_list_filter (void)
{
  Pango2AttrList *list;
  Pango2AttrList *out;

  list = pango2_attr_list_from_string ("0 -1 size 10\n"
                                      "10 20 stretch condensed\n"
                                      "20 -1 weight 700\n");

  out = pango2_attr_list_filter (list, never_true, NULL);
  g_assert_null (out);

  out = pango2_attr_list_filter (list, just_weight, NULL);
  g_assert_nonnull (out);

  assert_attr_list (list, "0 -1 size 10\n"
                          "10 20 stretch condensed\n");
  assert_attr_list (out, "20 -1 weight bold\n");

  pango2_attr_list_unref (list);
  pango2_attr_list_unref (out);
}

static void
test_iter (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;
  Pango2AttrIterator *iter;
  Pango2AttrIterator *copy;
  int start, end;

  /* Empty list */
  list = pango2_attr_list_new ();
  iter = pango2_attr_list_get_iterator (list);

  g_assert_false (pango2_attr_iterator_next (iter));
  g_assert_null (pango2_attr_iterator_get_attrs (iter));
  pango2_attr_iterator_destroy (iter);
  pango2_attr_list_unref (list);

  list = pango2_attr_list_new ();
  attr = attribute_from_string ("0 -1 size 10");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 30 stretch condensed");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("20 -1 weight bold");
  pango2_attr_list_insert (list, attr);

  iter = pango2_attr_list_get_iterator (list);
  copy = pango2_attr_iterator_copy (iter);
  pango2_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 10);
  g_assert_true (pango2_attr_iterator_next (iter));
  pango2_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 10);
  g_assert_cmpint (end, ==, 20);
  g_assert_true (pango2_attr_iterator_next (iter));
  pango2_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 20);
  g_assert_cmpint (end, ==, 30);
  g_assert_true (pango2_attr_iterator_next (iter));
  pango2_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 30);
  g_assert_cmpint (end, ==, G_MAXINT);
  g_assert_true (pango2_attr_iterator_next (iter));
  pango2_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, G_MAXINT);
  g_assert_cmpint (end, ==, G_MAXINT);
  g_assert_true (!pango2_attr_iterator_next (iter));

  pango2_attr_iterator_destroy (iter);

  pango2_attr_iterator_range (copy, &start, &end);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 10);
  pango2_attr_iterator_destroy (copy);

  pango2_attr_list_unref (list);
}

static void
test_iter_get (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;
  Pango2AttrIterator *iter;

  list = pango2_attr_list_new ();
  attr = pango2_attr_size_new (10);
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 30 stretch condensed");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("20 -1 weight bold");
  pango2_attr_list_insert (list, attr);

  iter = pango2_attr_list_get_iterator (list);
  pango2_attr_iterator_next (iter);
  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_SIZE);
  g_assert_nonnull (attr);
  g_assert_cmpuint (attr->start_index, ==, 0);
  g_assert_cmpuint (attr->end_index, ==, G_MAXUINT);
  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_STRETCH);
  g_assert_nonnull (attr);
  g_assert_cmpuint (attr->start_index, ==, 10);
  g_assert_cmpuint (attr->end_index, ==, 30);
  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_WEIGHT);
  g_assert_null (attr);
  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_GRAVITY);
  g_assert_null (attr);

  pango2_attr_iterator_destroy (iter);
  pango2_attr_list_unref (list);
}

static void
test_iter_get_font (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;
  Pango2AttrIterator *iter;
  Pango2FontDescription *desc;
  Pango2FontDescription *desc2;
  Pango2Language *lang;
  GSList *attrs;

  list = pango2_attr_list_new ();
  attr = pango2_attr_size_new (10 * PANGO2_SCALE);
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("0 -1 family \"Times\"");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 30 stretch condensed");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 20 absolute-size 10240");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 20 language ja-JP");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("20 -1 rise 100");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("20 -1 fallback false");
  pango2_attr_list_insert (list, attr);

  iter = pango2_attr_list_get_iterator (list);
  desc = pango2_font_description_new ();
  pango2_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = pango2_font_description_from_string ("Times 10");
  g_assert_true (pango2_font_description_equal (desc, desc2));
  g_assert_null (lang);
  g_assert_null (attrs);
  pango2_font_description_free (desc);
  pango2_font_description_free (desc2);

  pango2_attr_iterator_next (iter);
  desc = pango2_font_description_new ();
  pango2_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = pango2_font_description_from_string ("Times Condensed 10px");
  g_assert_true (pango2_font_description_equal (desc, desc2));
  g_assert_nonnull (lang);
  g_assert_cmpstr (pango2_language_to_string (lang), ==, "ja-jp");
  g_assert_null (attrs);
  pango2_font_description_free (desc);
  pango2_font_description_free (desc2);

  pango2_attr_iterator_next (iter);
  desc = pango2_font_description_new ();
  pango2_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = pango2_font_description_from_string ("Times Condensed 10");
  g_assert_true (pango2_font_description_equal (desc, desc2));
  g_assert_null (lang);
  assert_attributes (attrs, "20 -1 rise 100\n"
                            "20 -1 fallback false\n");
  g_slist_free_full (attrs, (GDestroyNotify)pango2_attribute_destroy);

  pango2_font_description_free (desc);
  pango2_font_description_free (desc2);

  pango2_attr_iterator_destroy (iter);
  pango2_attr_list_unref (list);
}

static void
test_iter_get_attrs (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;
  Pango2AttrIterator *iter;

  list = pango2_attr_list_new ();
  attr = pango2_attr_size_new (10 * PANGO2_SCALE);
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("0 -1 family \"Times\"");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 30 stretch condensed");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("10 20 language ja-JP");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("20 -1 rise 100");
  pango2_attr_list_insert (list, attr);
  attr = attribute_from_string ("20 -1 fallback false");
  pango2_attr_list_insert (list, attr);

  iter = pango2_attr_list_get_iterator (list);
  assert_attr_iterator (iter, "0 -1 size 10240\n"
                              "0 -1 family \"Times\"\n");

  pango2_attr_iterator_next (iter);
  assert_attr_iterator (iter, "0 -1 size 10240\n"
                              "0 -1 family \"Times\"\n"
                              "10 30 stretch condensed\n"
                              "10 20 language ja-jp\n");

  pango2_attr_iterator_next (iter);
  assert_attr_iterator (iter, "0 -1 size 10240\n"
                              "0 -1 family \"Times\"\n"
                              "10 30 stretch condensed\n"
                              "20 -1 rise 100\n"
                              "20 -1 fallback false\n");

  pango2_attr_iterator_next (iter);
  assert_attr_iterator (iter, "0 -1 size 10240\n"
                              "0 -1 family \"Times\"\n"
                              "20 -1 rise 100\n"
                              "20 -1 fallback false\n");

  pango2_attr_iterator_next (iter);
  g_assert_null (pango2_attr_iterator_get_attrs (iter));

  pango2_attr_iterator_destroy (iter);
  pango2_attr_list_unref (list);
}

static void
test_list_update (void)
{
  Pango2AttrList *list;

  list = pango2_attr_list_from_string ("0 200 rise 100\n"
                                      "5 15 family \"Times\"\n"
                                      "10 11 size 10240\n"
                                      "11 100 fallback false\n"
                                      "30 60 stretch condensed\n");

  pango2_attr_list_update (list, 8, 10, 20);

  assert_attr_list (list, "0 210 rise 100\n"
                          "5 8 family \"Times\"\n"
                          "28 110 fallback false\n"
                          "40 70 stretch condensed\n");

  pango2_attr_list_unref (list);
}

/* Test that empty lists work in pango2_attr_list_update */
static void
test_list_update2 (void)
{
  Pango2AttrList *list;

  list = pango2_attr_list_new ();
  pango2_attr_list_update (list, 8, 10, 20);

  g_assert_null (pango2_attr_list_get_attributes (list));

  pango2_attr_list_unref (list);
}

/* test overflow */
static void
test_list_update3 (void)
{
  Pango2AttrList *list;

  list = pango2_attr_list_from_string ("5 4294967285 family \"Times\"\n");

  pango2_attr_list_update (list, 8, 10, 30);

  assert_attr_list (list, "5 -1 family \"Times\"\n");

  pango2_attr_list_unref (list);
}
static void
test_list_equal (void)
{
  Pango2AttrList *list1, *list2;
  Pango2Attribute *attr;

  list1 = pango2_attr_list_new ();
  list2 = pango2_attr_list_new ();

  g_assert_true (pango2_attr_list_equal (NULL, NULL));
  g_assert_false (pango2_attr_list_equal (list1, NULL));
  g_assert_false (pango2_attr_list_equal (NULL, list1));
  g_assert_true (pango2_attr_list_equal (list1, list1));
  g_assert_true (pango2_attr_list_equal (list1, list2));

  attr = attribute_from_string ("0 7 size 10240");
  pango2_attr_list_insert (list1, pango2_attribute_copy (attr));
  pango2_attr_list_insert (list2, pango2_attribute_copy (attr));
  pango2_attribute_destroy (attr);

  g_assert_true (pango2_attr_list_equal (list1, list2));

  attr = attribute_from_string ("0 1 stretch condensed");
  pango2_attr_list_insert (list1, pango2_attribute_copy (attr));
  g_assert_true (!pango2_attr_list_equal (list1, list2));

  pango2_attr_list_insert (list2, pango2_attribute_copy (attr));
  g_assert_true (pango2_attr_list_equal (list1, list2));
  pango2_attribute_destroy (attr);

  /* Same range as the first attribute */
  attr = attribute_from_string ("0 7 size 30720");
  pango2_attr_list_insert (list2, pango2_attribute_copy (attr));
  g_assert_true (!pango2_attr_list_equal (list1, list2));
  pango2_attr_list_insert (list1, pango2_attribute_copy (attr));
  g_assert_true (pango2_attr_list_equal (list1, list2));
  pango2_attribute_destroy (attr);

  pango2_attr_list_unref (list1);
  pango2_attr_list_unref (list2);


  /* Same range but different order */
  {
    Pango2AttrList *list1, *list2;
    Pango2Attribute *attr1, *attr2;

    list1 = pango2_attr_list_new ();
    list2 = pango2_attr_list_new ();

    attr1 = pango2_attr_size_new (10 * PANGO2_SCALE);
    attr2 = pango2_attr_stretch_new (PANGO2_STRETCH_CONDENSED);

    pango2_attr_list_insert (list1, pango2_attribute_copy (attr1));
    pango2_attr_list_insert (list1, pango2_attribute_copy (attr2));

    pango2_attr_list_insert (list2, pango2_attribute_copy (attr2));
    pango2_attr_list_insert (list2, pango2_attribute_copy (attr1));

    pango2_attribute_destroy (attr1);
    pango2_attribute_destroy (attr2);

    g_assert_true (pango2_attr_list_equal (list1, list2));
    g_assert_true (pango2_attr_list_equal (list2, list1));

    pango2_attr_list_unref (list1);
    pango2_attr_list_unref (list2);
  }
}

static void
test_insert (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 200 rise 100\n"
                                      "5 15 family \"Times\"\n"
                                      "10 11 size 10240\n"
                                      "11 100 fallback false\n"
                                      "30 60 stretch condensed\n");

  attr = attribute_from_string ("10 25 family \"Times\"");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 200 rise 100\n"
                          "5 25 family \"Times\"\n"
                          "10 11 size 10240\n"
                          "11 100 fallback false\n"
                          "30 60 stretch condensed\n");

  attr = attribute_from_string ("11 25 family \"Futura\"");
  pango2_attr_list_insert (list, attr);

  assert_attr_list (list, "0 200 rise 100\n"
                          "5 25 family \"Times\"\n"
                          "10 11 size 10240\n"
                          "11 100 fallback false\n"
                          "11 25 family \"Futura\"\n"
                          "30 60 stretch condensed\n");

  pango2_attr_list_unref (list);
}

static void
test_insert2 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 200 rise 100\n"
                                      "5 15 family \"Times\"\n"
                                      "10 11 size 10240\n"
                                      "11 100 fallback false\n"
                                      "20 30 family \"Times\"\n"
                                      "30 40 family \"Futura\"\n"
                                      "30 60 stretch condensed\n");

  attr = attribute_from_string ("10 35 family \"Times\"");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 200 rise 100\n"
                          "5 35 family \"Times\"\n"
                          "10 11 size 10240\n"
                          "11 100 fallback false\n"
                          "35 40 family \"Futura\"\n"
                          "30 60 stretch condensed\n");

  pango2_attr_list_unref (list);
}

static gboolean
attr_list_merge_filter (Pango2Attribute *attribute,
                        gpointer        list)
{
  pango2_attr_list_change (list, pango2_attribute_copy (attribute));
  return FALSE;
}

/* test something that gtk does */
static void
test_merge (void)
{
  Pango2AttrList *list;
  Pango2AttrList *list2;

  list = pango2_attr_list_from_string ("0 200 rise 100\n"
                                      "5 15 family \"Times\"\n"
                                      "10 11 size 10240\n"
                                      "11 100 fallback false\n"
                                      "30 60 stretch condensed\n");

  list2 = pango2_attr_list_from_string ("11 13 size 10240\n"
                                       "13 15 size 11264\n"
                                       "40 50 size 12288\n");

  pango2_attr_list_filter (list2, attr_list_merge_filter, list);

  assert_attr_list (list, "0 200 rise 100\n"
                          "5 15 family \"Times\"\n"
                          "10 13 size 10240\n"
                          "11 100 fallback false\n"
                          "13 15 size 11264\n"
                          "30 60 stretch condensed\n"
                          "40 50 size 12288\n");

  pango2_attr_list_unref (list);
  pango2_attr_list_unref (list2);
}

/* reproduce what the links example in gtk4-demo does
 * with the colored Google link
 */
static void
test_merge2 (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 10 underline 1\n"
                                      "0 10 foreground #00000000ffff\n");

  attr = attribute_from_string ("2 3 foreground #ffff00000000");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 underline solid\n"
                          "0 2 foreground #00000000ffff\n"
                          "2 3 foreground #ffff00000000\n"
                          "3 10 foreground #00000000ffff");

  attr = attribute_from_string ("3 4 foreground #0000ffff0000");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 underline solid\n"
                          "0 2 foreground #00000000ffff\n"
                          "2 3 foreground #ffff00000000\n"
                          "3 4 foreground #0000ffff0000\n"
                          "4 10 foreground #00000000ffff");

  attr = attribute_from_string ("4 5 foreground #00000000ffff");
  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 10 underline solid\n"
                          "0 2 foreground #00000000ffff\n"
                          "2 3 foreground #ffff00000000\n"
                          "3 4 foreground #0000ffff0000\n"
                          "4 10 foreground #00000000ffff");

  pango2_attr_list_unref (list);
}

/* This only prints rise, size, scale, allow_breaks and line_break,
 * which are the only relevant attributes in the tests that use this
 * function.
 */
static void
print_tags_for_attributes (Pango2AttrIterator *iter,
                           GString           *s)
{
  Pango2Attribute *attr;

  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_RISE);
  if (attr)
    g_string_append_printf (s, "%d  %d rise %d\n",
                            attr->start_index, attr->end_index,
                            attr->int_value);

  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_SIZE);
  if (attr)
    g_string_append_printf (s, "%d  %d size %d\n",
                            attr->start_index, attr->end_index,
                            attr->int_value);

  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_SCALE);
  if (attr)
    g_string_append_printf (s, "%d  %d scale %f\n",
                            attr->start_index, attr->end_index,
                            attr->double_value);

  attr = pango2_attr_iterator_get (iter, PANGO2_ATTR_ALLOW_BREAKS);
  if (attr)
    g_string_append_printf (s, "%d  %d allow_breaks %d\n",
                            attr->start_index, attr->end_index,
                            attr->int_value);
}

static void
test_iter_epsilon_zero (void)
{
  const char *markup = "𝜀<span rise=\"-6000\" size=\"x-small\" font_desc=\"italic\">0</span> = 𝜔<span rise=\"8000\" size=\"smaller\">𝜔<span rise=\"14000\" size=\"smaller\">𝜔<span rise=\"20000\">.<span rise=\"23000\">.<span rise=\"26000\">.</span></span></span></span></span>";
  Pango2AttrList *attributes;
  Pango2AttrIterator *attr;
  char *text;
  GError *error = NULL;
  GString *s;

  s = g_string_new ("");

  pango2_parse_markup (markup, -1, 0, &attributes, &text, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (text, ==, "𝜀0 = 𝜔𝜔𝜔...");

  attr = pango2_attr_list_get_iterator (attributes);
  do
    {
      int start, end;

      pango2_attr_iterator_range (attr, &start, &end);

      g_string_append_printf (s, "range: [%d, %d]\n", start, end);

      print_tags_for_attributes (attr, s);
    }
  while (pango2_attr_iterator_next (attr));

  g_assert_cmpstr (s->str, ==,
                   "range: [0, 4]\n"
                   "range: [4, 5]\n"
                   "4  5 rise -6000\n"
                   "4  5 scale 0.694444\n"
                   "range: [5, 12]\n"
                   "range: [12, 16]\n"
                   "12  23 rise 8000\n"
                   "12  23 scale 0.833333\n"
                   "range: [16, 20]\n"
                   "16  23 rise 14000\n"
                   "16  23 scale 0.694444\n"
                   "range: [20, 21]\n"
                   "20  23 rise 20000\n"
                   "16  23 scale 0.694444\n"
                   "range: [21, 22]\n"
                   "21  23 rise 23000\n"
                   "16  23 scale 0.694444\n"
                   "range: [22, 23]\n"
                   "22  23 rise 26000\n"
                   "16  23 scale 0.694444\n"
                   "range: [23, 2147483647]\n");

  g_free (text);
  pango2_attr_list_unref (attributes);
  pango2_attr_iterator_destroy (attr);

  g_string_free (s, TRUE);
}

static void
test_gnumeric_splice (void)
{
  Pango2AttrList *list, *list2;

  list = pango2_attr_list_from_string ("0 -1 font-desc \"Sans 10\"\n");
  list2 = pango2_attr_list_from_string ("1 2 weight bold\n");

  pango2_attr_list_splice (list, list2, 0, 0);

  assert_attr_list (list, "0 4294967295 font-desc \"Sans 10\"\n"
                          "1 2 weight bold\n");

  pango2_attr_list_unref (list);
  pango2_attr_list_unref (list2);
}

static void
test_change_order (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("1 -1 font-features \"tnum=1\"\n"
                                      "0 20 font-desc \"sans-serif\"\n"
                                      "0 9 size 102400\n");

  attr = pango2_attr_font_features_new ("tnum=2");
  attr->end_index = 9;

  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 9 font-features \"tnum=2\"\n"
                          "0 20 font-desc \"sans-serif\"\n"
                          "0 9 size 102400\n"
                          "9 4294967295 font-features \"tnum=1\"\n");

  pango2_attr_list_unref (list);
}

static void
test_pitivi_crash (void)
{
  Pango2AttrList *list;
  Pango2Attribute *attr;

  list = pango2_attr_list_from_string ("0 8 font-features \"tnum=1\"\n"
                                      "0 20 font-desc \"sans-serif\"\n"
                                      "0 9 size 102400\n");

  attr = pango2_attr_font_features_new ("tnum=2");
  attr->end_index = 9;

  pango2_attr_list_change (list, attr);

  assert_attr_list (list, "0 9 font-features \"tnum=2\"\n"
                          "0 20 font-desc \"sans-serif\"\n"
                          "0 9 size 102400\n");

  pango2_attr_list_unref (list);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/attributes/basic", test_attributes_basic);
  g_test_add_func ("/attributes/equal", test_attributes_equal);
  g_test_add_func ("/attributes/register", test_attributes_register);
  g_test_add_func ("/attributes/list/basic", test_list);
  g_test_add_func ("/attributes/list/change", test_list_change);
  g_test_add_func ("/attributes/list/change2", test_list_change2);
  g_test_add_func ("/attributes/list/change3", test_list_change3);
  g_test_add_func ("/attributes/list/change4", test_list_change4);
  g_test_add_func ("/attributes/list/change5", test_list_change5);
  g_test_add_func ("/attributes/list/change6", test_list_change6);
  g_test_add_func ("/attributes/list/change7", test_list_change7);
  g_test_add_func ("/attributes/list/change8", test_list_change8);
  g_test_add_func ("/attributes/list/change9", test_list_change9);
  g_test_add_func ("/attributes/list/change10", test_list_change10);
  g_test_add_func ("/attributes/list/change11", test_list_change11);
  g_test_add_func ("/attributes/list/change12", test_list_change12);
  g_test_add_func ("/attributes/list/splice", test_list_splice);
  g_test_add_func ("/attributes/list/splice2", test_list_splice2);
  g_test_add_func ("/attributes/list/splice3", test_list_splice3);
  g_test_add_func ("/attributes/list/filter", test_list_filter);
  g_test_add_func ("/attributes/list/update", test_list_update);
  g_test_add_func ("/attributes/list/update2", test_list_update2);
  g_test_add_func ("/attributes/list/update3", test_list_update3);
  g_test_add_func ("/attributes/list/equal", test_list_equal);
  g_test_add_func ("/attributes/list/insert", test_insert);
  g_test_add_func ("/attributes/list/insert2", test_insert2);
  g_test_add_func ("/attributes/list/merge", test_merge);
  g_test_add_func ("/attributes/list/merge2", test_merge2);
  g_test_add_func ("/attributes/iter/basic", test_iter);
  g_test_add_func ("/attributes/iter/get", test_iter_get);
  g_test_add_func ("/attributes/iter/get_font", test_iter_get_font);
  g_test_add_func ("/attributes/iter/get_attrs", test_iter_get_attrs);
  g_test_add_func ("/attributes/iter/epsilon_zero", test_iter_epsilon_zero);
  g_test_add_func ("/attributes/gnumeric-splice", test_gnumeric_splice);
  g_test_add_func ("/attributes/list/change_order", test_change_order);
  g_test_add_func ("/attributes/pitivi-crash", test_pitivi_crash);

  return g_test_run ();
}
