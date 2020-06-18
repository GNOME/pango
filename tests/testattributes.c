/* Pango
 * testiter.c: Test pango attributes
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

#include <pango/pango.h>
#include "test-common.h"

static void
test_copy (PangoAttribute *attr)
{
  PangoAttribute *a;

  a = pango_attribute_copy (attr);
  g_assert_true (pango_attribute_equal (attr, a));
  pango_attribute_destroy (a);
  pango_attribute_destroy (attr);
}

static void
test_attributes_basic (void)
{
  PangoFontDescription *desc;
  PangoRectangle rect = { 0, 0, 10, 10 };

  test_copy (pango_attr_language_new (pango_language_from_string ("ja-JP")));
  test_copy (pango_attr_family_new ("Times"));
  test_copy (pango_attr_foreground_new (100, 200, 300));
  test_copy (pango_attr_background_new (100, 200, 300));
  test_copy (pango_attr_size_new (1024));
  test_copy (pango_attr_size_new_absolute (1024));
  test_copy (pango_attr_style_new (PANGO_STYLE_ITALIC));
  test_copy (pango_attr_weight_new (PANGO_WEIGHT_ULTRALIGHT));
  test_copy (pango_attr_variant_new (PANGO_VARIANT_SMALL_CAPS));
  test_copy (pango_attr_stretch_new (PANGO_STRETCH_SEMI_EXPANDED));
  desc = pango_font_description_from_string ("Computer Modern 12");
  test_copy (pango_attr_font_desc_new (desc));
  pango_font_description_free (desc);
  test_copy (pango_attr_underline_new (PANGO_UNDERLINE_LOW));
  test_copy (pango_attr_underline_new (PANGO_UNDERLINE_ERROR_LINE));
  test_copy (pango_attr_underline_color_new (100, 200, 300));
  test_copy (pango_attr_overline_new (PANGO_OVERLINE_SINGLE));
  test_copy (pango_attr_overline_color_new (100, 200, 300));
  test_copy (pango_attr_strikethrough_new (TRUE));
  test_copy (pango_attr_strikethrough_color_new (100, 200, 300));
  test_copy (pango_attr_rise_new (256));
  test_copy (pango_attr_scale_new (2.56));
  test_copy (pango_attr_fallback_new (FALSE));
  test_copy (pango_attr_letter_spacing_new (1024));
  test_copy (pango_attr_shape_new (&rect, &rect));
  test_copy (pango_attr_gravity_new (PANGO_GRAVITY_SOUTH));
  test_copy (pango_attr_gravity_hint_new (PANGO_GRAVITY_HINT_STRONG));
  test_copy (pango_attr_allow_breaks_new (FALSE));
  test_copy (pango_attr_show_new (PANGO_SHOW_SPACES));
  test_copy (pango_attr_insert_hyphens_new (FALSE));
}

static void
test_attributes_equal (void)
{
  PangoAttribute *attr1, *attr2, *attr3;

  /* check that pango_attribute_equal compares values, but not ranges */
  attr1 = pango_attr_size_new (10);
  attr2 = pango_attr_size_new (20);
  attr3 = pango_attr_size_new (20);
  attr3->start_index = 1;
  attr3->end_index = 2;

  g_assert_true (!pango_attribute_equal (attr1, attr2));
  g_assert_true (pango_attribute_equal (attr2, attr3));

  pango_attribute_destroy (attr1);
  pango_attribute_destroy (attr2);
  pango_attribute_destroy (attr3);
}

static void
assert_attributes (GSList     *attrs,
                   const char *expected)
{
  GString *s;

  s = g_string_new ("");
  print_attributes (attrs, s);
  if (strcmp (s->str, expected) != 0)
    {
      g_print ("-----\nattribute list mismatch\nexpected:\n%s-----\nreceived:\n%s-----\n",
               expected, s->str);
      g_assert_not_reached ();
    }
  g_string_free (s, TRUE);
}

static void
assert_attr_list (PangoAttrList *list,
                  const char    *expected)
{
  GSList *attrs;

  attrs = pango_attr_list_get_attributes (list);
  assert_attributes (attrs, expected);
  g_slist_free_full (attrs, (GDestroyNotify)pango_attribute_destroy);
}

static void
assert_attr_iterator (PangoAttrIterator *iter,
                      const char        *expected)
{
  GSList *attrs;

  attrs = pango_attr_iterator_get_attrs (iter);
  assert_attributes (attrs, expected);
  g_slist_free_full (attrs, (GDestroyNotify)pango_attribute_destroy);
}

static void
test_list (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;

  list = pango_attr_list_new ();

  attr = pango_attr_size_new (10);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_size_new (20);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_size_new (30);
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[0,-1]size=20\n"
                          "[0,-1]size=30\n");
  pango_attr_list_unref (list);

  list = pango_attr_list_new ();

  /* test that insertion respects start_index */
  attr = pango_attr_size_new (10);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_size_new (20);
  attr->start_index = 10;
  attr->end_index = 20;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_size_new (30);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_size_new (40);
  attr->start_index = 10;
  attr->end_index = 40;
  pango_attr_list_insert_before (list, attr);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[0,-1]size=30\n"
                          "[10,40]size=40\n"
                          "[10,20]size=20\n");
  pango_attr_list_unref (list);
}

static void
test_list_change (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;

  list = pango_attr_list_new ();

  attr = pango_attr_size_new (10);
  attr->start_index = 0;
  attr->end_index = 10;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_size_new (20);
  attr->start_index = 20;
  attr->end_index = 30;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index = 30;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,10]size=10\n"
                          "[0,30]weight=700\n"
                          "[20,30]size=20\n");

  /* simple insertion with pango_attr_list_change */
  attr = pango_attr_variant_new (PANGO_VARIANT_SMALL_CAPS);
  attr->start_index = 10;
  attr->end_index = 20;
  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]size=10\n"
                          "[0,30]weight=700\n"
                          "[10,20]variant=1\n"
                          "[20,30]size=20\n");

  /* insertion with splitting */
  attr = pango_attr_weight_new (PANGO_WEIGHT_LIGHT);
  attr->start_index = 15;
  attr->end_index = 20;
  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]size=10\n"
                          "[0,15]weight=700\n"
                          "[10,20]variant=1\n"
                          "[15,20]weight=300\n"
                          "[20,30]size=20\n"
                          "[20,30]weight=700\n");

  /* insertion with joining */
  attr = pango_attr_size_new (20);
  attr->start_index = 5;
  attr->end_index = 20;
  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,5]size=10\n"
                          "[0,15]weight=700\n"
                          "[5,30]size=20\n"
                          "[10,20]variant=1\n"
                          "[15,20]weight=300\n"
                          "[20,30]weight=700\n");

  pango_attr_list_unref (list);
}

static void
test_list_splice (void)
{
  PangoAttrList *base;
  PangoAttrList *list;
  PangoAttrList *other;
  PangoAttribute *attr;

  base = pango_attr_list_new ();
  attr = pango_attr_size_new (10);
  attr->start_index = 0;
  attr->end_index = -1;
  pango_attr_list_insert (base, attr);
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 10;
  attr->end_index = 15;
  pango_attr_list_insert (base, attr);
  attr = pango_attr_variant_new (PANGO_VARIANT_SMALL_CAPS);
  attr->start_index = 20;
  attr->end_index = 30;
  pango_attr_list_insert (base, attr);

  assert_attr_list (base, "[0,-1]size=10\n"
                          "[10,15]weight=700\n"
                          "[20,30]variant=1\n");

  /* splice in an empty list */
  list = pango_attr_list_copy (base);
  other = pango_attr_list_new ();
  pango_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[10,20]weight=700\n"
                          "[25,35]variant=1\n");

  pango_attr_list_unref (list);
  pango_attr_list_unref (other);

  /* splice in some attributes */
  list = pango_attr_list_copy (base);
  other = pango_attr_list_new ();
  attr = pango_attr_size_new (20);
  attr->start_index = 0;
  attr->end_index = 3;
  pango_attr_list_insert (other, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 2;
  attr->end_index = 4;
  pango_attr_list_insert (other, attr);

  pango_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "[0,11]size=10\n"
                          "[10,20]weight=700\n"
                          "[11,14]size=20\n"
                          "[13,15]stretch=2\n"
                          "[14,-1]size=10\n"
                          "[25,35]variant=1\n");

  pango_attr_list_unref (list);
  pango_attr_list_unref (other);

  pango_attr_list_unref (base);
}

static gboolean
never_true (PangoAttribute *attribute, gpointer user_data)
{
  return FALSE;
}

static gboolean
just_weight (PangoAttribute *attribute, gpointer user_data)
{
  if (attribute->klass->type == PANGO_ATTR_WEIGHT)
    return TRUE;
  else
    return FALSE;
}

static void
test_list_filter (void)
{
  PangoAttrList *list;
  PangoAttrList *out;
  PangoAttribute *attr;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 20;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[10,20]stretch=2\n"
                          "[20,-1]weight=700\n");

  out = pango_attr_list_filter (list, never_true, NULL);
  g_assert_null (out);

  out = pango_attr_list_filter (list, just_weight, NULL);
  g_assert_nonnull (out);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[10,20]stretch=2\n");
  assert_attr_list (out, "[20,-1]weight=700\n");

  pango_attr_list_unref (list);
  pango_attr_list_unref (out);
}

static void
test_iter (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;
  PangoAttrIterator *iter;
  PangoAttrIterator *copy;
  gint start, end;

  /* Empty list */
  list = pango_attr_list_new ();
  iter = pango_attr_list_get_iterator (list);

  g_assert_false (pango_attr_iterator_next (iter));
  g_assert_null (pango_attr_iterator_get_attrs (iter));
  pango_attr_iterator_destroy (iter);
  pango_attr_list_unref (list);

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);

  iter = pango_attr_list_get_iterator (list);
  copy = pango_attr_iterator_copy (iter);
  pango_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 10);
  g_assert_true (pango_attr_iterator_next (iter));
  pango_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 10);
  g_assert_cmpint (end, ==, 20);
  g_assert_true (pango_attr_iterator_next (iter));
  pango_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 20);
  g_assert_cmpint (end, ==, 30);
  g_assert_true (pango_attr_iterator_next (iter));
  pango_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 30);
  g_assert_cmpint (end, ==, G_MAXINT);
  g_assert_true (pango_attr_iterator_next (iter));
  pango_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, G_MAXINT);
  g_assert_cmpint (end, ==, G_MAXINT);
  g_assert_true (!pango_attr_iterator_next (iter));

  pango_attr_iterator_destroy (iter);

  pango_attr_iterator_range (copy, &start, &end);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 10);
  pango_attr_iterator_destroy (copy);

  pango_attr_list_unref (list);
}

static void
test_iter_get (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;
  PangoAttrIterator *iter;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);

  iter = pango_attr_list_get_iterator (list);
  pango_attr_iterator_next (iter);
  attr = pango_attr_iterator_get (iter, PANGO_ATTR_SIZE);
  g_assert_nonnull (attr);
  g_assert_cmpuint (attr->start_index, ==, 0);
  g_assert_cmpuint (attr->end_index, ==, G_MAXUINT);
  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRETCH);
  g_assert_nonnull (attr);
  g_assert_cmpuint (attr->start_index, ==, 10);
  g_assert_cmpuint (attr->end_index, ==, 30);
  attr = pango_attr_iterator_get (iter, PANGO_ATTR_WEIGHT);
  g_assert_null (attr);
  attr = pango_attr_iterator_get (iter, PANGO_ATTR_GRAVITY);
  g_assert_null (attr);

  pango_attr_iterator_destroy (iter);
  pango_attr_list_unref (list);
}

static void
test_iter_get_font (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;
  PangoAttrIterator *iter;
  PangoFontDescription *desc;
  PangoFontDescription *desc2;
  PangoLanguage *lang;
  GSList *attrs;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10 * PANGO_SCALE);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_family_new ("Times");
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_language_new (pango_language_from_string ("ja-JP"));
  attr->start_index = 10;
  attr->end_index = 20;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_rise_new (100);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_fallback_new (FALSE);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);

  iter = pango_attr_list_get_iterator (list);
  desc = pango_font_description_new ();
  pango_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = pango_font_description_from_string ("Times 10");
  g_assert_true (pango_font_description_equal (desc, desc2));
  g_assert_null (lang);
  g_assert_null (attrs);
  pango_font_description_free (desc);
  pango_font_description_free (desc2);

  pango_attr_iterator_next (iter);
  desc = pango_font_description_new ();
  pango_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = pango_font_description_from_string ("Times Condensed 10");
  g_assert_true (pango_font_description_equal (desc, desc2));
  g_assert_nonnull (lang);
  g_assert_cmpstr (pango_language_to_string (lang), ==, "ja-jp");
  g_assert_null (attrs);
  pango_font_description_free (desc);
  pango_font_description_free (desc2);

  pango_attr_iterator_next (iter);
  desc = pango_font_description_new ();
  pango_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = pango_font_description_from_string ("Times Condensed 10");
  g_assert_true (pango_font_description_equal (desc, desc2));
  g_assert_null (lang);
  assert_attributes (attrs, "[20,-1]rise=100\n"
                            "[20,-1]fallback=0\n");
  g_slist_free_full (attrs, (GDestroyNotify)pango_attribute_destroy);

  pango_font_description_free (desc);
  pango_font_description_free (desc2);

  pango_attr_iterator_destroy (iter);
  pango_attr_list_unref (list);
}

static void
test_iter_get_attrs (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;
  PangoAttrIterator *iter;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10 * PANGO_SCALE);
  pango_attr_list_insert (list, attr);
  attr = pango_attr_family_new ("Times");
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_language_new (pango_language_from_string ("ja-JP"));
  attr->start_index = 10;
  attr->end_index = 20;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_rise_new (100);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_fallback_new (FALSE);
  attr->start_index = 20;
  pango_attr_list_insert (list, attr);

  iter = pango_attr_list_get_iterator (list);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n");

  pango_attr_iterator_next (iter);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n"
                              "[10,30]stretch=2\n"
                              "[10,20]language=ja-jp\n");

  pango_attr_iterator_next (iter);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n"
                              "[10,30]stretch=2\n"
                              "[20,-1]rise=100\n"
                              "[20,-1]fallback=0\n");

  pango_attr_iterator_next (iter);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n"
                              "[20,-1]rise=100\n"
                              "[20,-1]fallback=0\n");

  pango_attr_iterator_next (iter);
  g_assert_null (pango_attr_iterator_get_attrs (iter));

  pango_attr_iterator_destroy (iter);
  pango_attr_list_unref (list);
}

static void
test_list_update (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10 * PANGO_SCALE);
  attr->start_index = 10;
  attr->end_index = 11;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_rise_new (100);
  attr->start_index = 0;
  attr->end_index = 200;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_family_new ("Times");
  attr->start_index = 5;
  attr->end_index = 15;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_fallback_new (FALSE);
  attr->start_index = 11;
  attr->end_index = 100;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 30;
  attr->end_index = 60;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,15]family=Times\n"
                          "[10,11]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[30,60]stretch=2\n");

  pango_attr_list_update (list, 8, 10, 20);

  assert_attr_list (list, "[0,210]rise=100\n"
                          "[5,8]family=Times\n"
                          "[28,110]fallback=0\n"
                          "[40,70]stretch=2\n");

  pango_attr_list_unref (list);
}

static void
test_list_equal (void)
{
  PangoAttrList *list1, *list2;
  PangoAttribute *attr;

  list1 = pango_attr_list_new ();
  list2 = pango_attr_list_new ();

  g_assert_true (pango_attr_list_equal (NULL, NULL));
  g_assert_false (pango_attr_list_equal (list1, NULL));
  g_assert_false (pango_attr_list_equal (NULL, list1));
  g_assert_true (pango_attr_list_equal (list1, list1));
  g_assert_true (pango_attr_list_equal (list1, list2));

  attr = pango_attr_size_new (10 * PANGO_SCALE);
  attr->start_index = 0;
  attr->end_index = 7;
  pango_attr_list_insert (list1, pango_attribute_copy (attr));
  pango_attr_list_insert (list2, pango_attribute_copy (attr));
  pango_attribute_destroy (attr);

  g_assert_true (pango_attr_list_equal (list1, list2));

  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 0;
  attr->end_index = 1;
  pango_attr_list_insert (list1, pango_attribute_copy (attr));
  g_assert_true (!pango_attr_list_equal (list1, list2));

  pango_attr_list_insert (list2, pango_attribute_copy (attr));
  g_assert_true (pango_attr_list_equal (list1, list2));
  pango_attribute_destroy (attr);

  attr = pango_attr_size_new (30 * PANGO_SCALE);
  /* Same range as the first attribute */
  attr->start_index = 0;
  attr->end_index = 7;
  pango_attr_list_insert (list2, pango_attribute_copy (attr));
  g_assert_true (!pango_attr_list_equal (list1, list2));
  pango_attr_list_insert (list1, pango_attribute_copy (attr));
  g_assert_true (pango_attr_list_equal (list1, list2));
  pango_attribute_destroy (attr);

  pango_attr_list_unref (list1);
  pango_attr_list_unref (list2);


  /* Same range but different order */
  {
    PangoAttrList *list1, *list2;
    PangoAttribute *attr1, *attr2;

    list1 = pango_attr_list_new ();
    list2 = pango_attr_list_new ();

    attr1 = pango_attr_size_new (10 * PANGO_SCALE);
    attr2 = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);

    pango_attr_list_insert (list1, pango_attribute_copy (attr1));
    pango_attr_list_insert (list1, pango_attribute_copy (attr2));

    pango_attr_list_insert (list2, pango_attribute_copy (attr2));
    pango_attr_list_insert (list2, pango_attribute_copy (attr1));

    pango_attribute_destroy (attr1);
    pango_attribute_destroy (attr2);

    g_assert_true (pango_attr_list_equal (list1, list2));
    g_assert_true (pango_attr_list_equal (list2, list1));

    pango_attr_list_unref (list1);
    pango_attr_list_unref (list2);
  }
}

static void
test_insert (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10 * PANGO_SCALE);
  attr->start_index = 10;
  attr->end_index = 11;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_rise_new (100);
  attr->start_index = 0;
  attr->end_index = 200;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_family_new ("Times");
  attr->start_index = 5;
  attr->end_index = 15;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_fallback_new (FALSE);
  attr->start_index = 11;
  attr->end_index = 100;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 30;
  attr->end_index = 60;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,15]family=Times\n"
                          "[10,11]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[30,60]stretch=2\n");

  attr = pango_attr_family_new ("Times");
  attr->start_index = 10;
  attr->end_index = 25;
  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,25]family=Times\n"
                          "[10,11]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[30,60]stretch=2\n");

  attr = pango_attr_family_new ("Futura");
  attr->start_index = 11;
  attr->end_index = 25;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,25]family=Times\n"
                          "[10,11]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[11,25]family=Futura\n"
                          "[30,60]stretch=2\n");

  pango_attr_list_unref (list);
}

static gboolean
attr_list_merge_filter (PangoAttribute *attribute,
                        gpointer        list)
{
  pango_attr_list_change (list, pango_attribute_copy (attribute));
  return FALSE;
}

/* test something that gtk does */
static void
test_merge (void)
{
  PangoAttrList *list;
  PangoAttrList *list2;
  PangoAttribute *attr;

  list = pango_attr_list_new ();
  attr = pango_attr_size_new (10 * PANGO_SCALE);
  attr->start_index = 10;
  attr->end_index = 11;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_rise_new (100);
  attr->start_index = 0;
  attr->end_index = 200;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_family_new ("Times");
  attr->start_index = 5;
  attr->end_index = 15;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_fallback_new (FALSE);
  attr->start_index = 11;
  attr->end_index = 100;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 30;
  attr->end_index = 60;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,15]family=Times\n"
                          "[10,11]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[30,60]stretch=2\n");

  list2 = pango_attr_list_new ();
  attr = pango_attr_size_new (10 * PANGO_SCALE);
  attr->start_index = 11;
  attr->end_index = 13;
  pango_attr_list_insert (list2, attr);
  attr = pango_attr_size_new (11 * PANGO_SCALE);
  attr->start_index = 13;
  attr->end_index = 15;
  pango_attr_list_insert (list2, attr);
  attr = pango_attr_size_new (12 * PANGO_SCALE);
  attr->start_index = 40;
  attr->end_index = 50;
  pango_attr_list_insert (list2, attr);

  assert_attr_list (list2, "[11,13]size=10240\n"
                           "[13,15]size=11264\n"
                           "[40,50]size=12288\n");

  pango_attr_list_filter (list2, attr_list_merge_filter, list);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,15]family=Times\n"
                          "[10,13]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[13,15]size=11264\n"
                          "[30,60]stretch=2\n"
                          "[40,50]size=12288\n");

  pango_attr_list_unref (list);
  pango_attr_list_unref (list2);
}

/* reproduce what the links example in gtk4-demo does
 * with the colored Google link
 */
static void
test_merge2 (void)
{
  PangoAttrList *list;
  PangoAttribute *attr;

  list = pango_attr_list_new ();
  attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
  attr->start_index = 0;
  attr->end_index = 10;
  pango_attr_list_insert (list, attr);
  attr = pango_attr_foreground_new (0, 0, 0xffff);
  attr->start_index = 0;
  attr->end_index = 10;
  pango_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,10]underline=1\n"
                          "[0,10]foreground=#00000000ffff\n");

  attr = pango_attr_foreground_new (0xffff, 0, 0);
  attr->start_index = 2;
  attr->end_index = 3;

  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]underline=1\n"
                          "[0,2]foreground=#00000000ffff\n"
                          "[2,3]foreground=#ffff00000000\n"
                          "[3,10]foreground=#00000000ffff\n");

  attr = pango_attr_foreground_new (0, 0xffff, 0);
  attr->start_index = 3;
  attr->end_index = 4;

  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]underline=1\n"
                          "[0,2]foreground=#00000000ffff\n"
                          "[2,3]foreground=#ffff00000000\n"
                          "[3,4]foreground=#0000ffff0000\n"
                          "[4,10]foreground=#00000000ffff\n");

  attr = pango_attr_foreground_new (0, 0, 0xffff);
  attr->start_index = 4;
  attr->end_index = 5;

  pango_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]underline=1\n"
                          "[0,2]foreground=#00000000ffff\n"
                          "[2,3]foreground=#ffff00000000\n"
                          "[3,4]foreground=#0000ffff0000\n"
                          "[4,10]foreground=#00000000ffff\n");

  pango_attr_list_unref (list);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/attributes/basic", test_attributes_basic);
  g_test_add_func ("/attributes/equal", test_attributes_equal);
  g_test_add_func ("/attributes/list/basic", test_list);
  g_test_add_func ("/attributes/list/change", test_list_change);
  g_test_add_func ("/attributes/list/splice", test_list_splice);
  g_test_add_func ("/attributes/list/filter", test_list_filter);
  g_test_add_func ("/attributes/list/update", test_list_update);
  g_test_add_func ("/attributes/list/equal", test_list_equal);
  g_test_add_func ("/attributes/list/insert", test_insert);
  g_test_add_func ("/attributes/list/merge", test_merge);
  g_test_add_func ("/attributes/list/merge2", test_merge2);
  g_test_add_func ("/attributes/iter/basic", test_iter);
  g_test_add_func ("/attributes/iter/get", test_iter_get);
  g_test_add_func ("/attributes/iter/get_font", test_iter_get_font);
  g_test_add_func ("/attributes/iter/get_attrs", test_iter_get_attrs);

  return g_test_run ();
}
