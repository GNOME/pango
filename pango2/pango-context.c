/* Pango2
 * pango-context.c: Contexts for itemization and shaping
 *
 * Copyright (C) 2000, 2006 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>

#include <gio/gio.h>

#include "pango-context.h"
#include "pango-context-private.h"
#include "pango-impl-utils.h"

#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-item-private.h"
#include "pango-fontset-private.h"
#include "pango-fontmap-private.h"
#include "pango-script-private.h"
#include "pango-emoji-private.h"

/**
 * Pango2Context:
 *
 * A `Pango2Context` stores global information used to control the
 * itemization process.
 *
 * The information stored by `Pango2Context` includes the fontmap used
 * to look up fonts, and default values such as the default language,
 * default gravity, or default font.
 *
 * To obtain a `Pango2Context`, use [ctor@Pango2.Context.new]
 * or [func@Pango2.cairo_create_context].
 */

struct _Pango2ContextClass
{
  GObjectClass parent_class;

};

static void pango2_context_finalize    (GObject       *object);
static void context_changed            (Pango2Context *context);

enum {
  PROP_FONT_MAP = 1,
  PROP_FONT_DESCRIPTION,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_BASE_GRAVITY,
  PROP_GRAVITY_HINT,
  PROP_MATRIX,
  PROP_ROUND_GLYPH_POSITIONS,
  PROP_PALETTE,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE (Pango2Context, pango2_context, G_TYPE_OBJECT)

static void
pango2_context_init (Pango2Context *context)
{
  context->base_dir = PANGO2_DIRECTION_WEAK_LTR;
  context->resolved_gravity = context->base_gravity = PANGO2_GRAVITY_SOUTH;
  context->gravity_hint = PANGO2_GRAVITY_HINT_NATURAL;

  context->serial = 1;
  context->set_language = NULL;
  context->language = pango2_language_get_default ();
  context->font_map = NULL;
  context->round_glyph_positions = TRUE;
  context->palette = g_quark_from_static_string (PANGO2_COLOR_PALETTE_DEFAULT);

  context->font_desc = pango2_font_description_new ();
  pango2_font_description_set_family_static (context->font_desc, "serif");
  pango2_font_description_set_style (context->font_desc, PANGO2_STYLE_NORMAL);
  pango2_font_description_set_variant (context->font_desc, PANGO2_VARIANT_NORMAL);
  pango2_font_description_set_weight (context->font_desc, PANGO2_WEIGHT_NORMAL);
  pango2_font_description_set_stretch (context->font_desc, PANGO2_STRETCH_NORMAL);
  pango2_font_description_set_size (context->font_desc, 12 * PANGO2_SCALE);
}

static void
pango2_context_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  Pango2Context *context = PANGO2_CONTEXT (object);

  switch (property_id)
    {
    case PROP_FONT_MAP:
      pango2_context_set_font_map (context, g_value_get_object (value));
      break;

    case PROP_FONT_DESCRIPTION:
      pango2_context_set_font_description (context, g_value_get_boxed (value));
      break;

    case PROP_LANGUAGE:
      pango2_context_set_language (context, g_value_get_boxed (value));
      break;

    case PROP_BASE_DIR:
      pango2_context_set_base_dir (context, g_value_get_enum (value));
      break;

    case PROP_BASE_GRAVITY:
      pango2_context_set_base_gravity (context, g_value_get_enum (value));
      break;

    case PROP_GRAVITY_HINT:
      pango2_context_set_gravity_hint (context, g_value_get_enum (value));
      break;

    case PROP_MATRIX:
      pango2_context_set_matrix (context, g_value_get_boxed (value));
      break;

    case PROP_ROUND_GLYPH_POSITIONS:
      pango2_context_set_round_glyph_positions (context, g_value_get_boolean (value));
      break;

    case PROP_PALETTE:
      pango2_context_set_palette (context, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango2_context_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  Pango2Context *context = PANGO2_CONTEXT (object);

  switch (property_id)
    {
    case PROP_FONT_MAP:
      g_value_set_object (value, pango2_context_get_font_map (context));
      break;

    case PROP_FONT_DESCRIPTION:
      g_value_set_boxed (value, pango2_context_get_font_description (context));
      break;

    case PROP_LANGUAGE:
      g_value_set_boxed (value, pango2_context_get_language (context));
      break;

    case PROP_BASE_DIR:
      g_value_set_enum (value, pango2_context_get_base_dir (context));
      break;

    case PROP_BASE_GRAVITY:
      g_value_set_enum (value, pango2_context_get_base_gravity (context));
      break;

    case PROP_GRAVITY_HINT:
      g_value_set_enum (value, pango2_context_get_gravity_hint (context));
      break;

    case PROP_MATRIX:
      g_value_set_boxed (value, pango2_context_get_matrix (context));
      break;

    case PROP_ROUND_GLYPH_POSITIONS:
      g_value_set_boolean (value, pango2_context_get_round_glyph_positions (context));
      break;

    case PROP_PALETTE:
      g_value_set_string (value, pango2_context_get_palette (context));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango2_context_class_init (Pango2ContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_intern_static_string (PANGO2_COLOR_PALETTE_DEFAULT);
  g_intern_static_string (PANGO2_COLOR_PALETTE_LIGHT);
  g_intern_static_string (PANGO2_COLOR_PALETTE_DARK);

  object_class->finalize = pango2_context_finalize;
  object_class->set_property = pango2_context_set_property;
  object_class->get_property = pango2_context_get_property;

  /**
   * Pango2Context:font-map: (attributes org.gtk.Property.get=pango2_context_get_font_map org.gtk.Property.set=pango2_context_set_font_map)
   *
   * The font map to be searched when fonts are looked up
   * in this context.
   */
  properties[PROP_FONT_MAP] =
    g_param_spec_object ("font-map", NULL, NULL, PANGO2_TYPE_FONT_MAP,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:font-description: (attributes org.gtk.Property.get=pango2_context_get_font_description org.gtk.Property.set=pango2_context_set_font_description)
   *
   * The default font description for the context.
   */
  properties[PROP_FONT_DESCRIPTION] =
    g_param_spec_boxed ("font-description", NULL, NULL, PANGO2_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:language: (attributes org.gtk.Property.get=pango2_context_get_language org.gtk.Property.set=pango2_context_set_language)
   *
   * The global language for the context.
   */
  properties[PROP_LANGUAGE] =
    g_param_spec_boxed ("language", NULL, NULL, PANGO2_TYPE_LANGUAGE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:base-direction: (attributes org.gtk.Property.get=pango2_context_get_base_dir org.gtk.Property.set=pango2_context_set_base_dir)
   *
   * The base direction for the context.
   *
   * The base direction is used in applying the Unicode bidirectional
   * algorithm; if the @direction is `PANGO2_DIRECTION_LTR` or
   * `PANGO2_DIRECTION_RTL`, then the value will be used as the paragraph
   * direction in the Unicode bidirectional algorithm. A value of
   * `PANGO2_DIRECTION_WEAK_LTR` or `PANGO2_DIRECTION_WEAK_RTL` is used only
   * for paragraphs that do not contain any strong characters themselves.
   */
  properties[PROP_BASE_DIR] =
    g_param_spec_enum ("base-direction", NULL, NULL, PANGO2_TYPE_DIRECTION,
                       PANGO2_DIRECTION_LTR,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:base-gravity: (attributes org.gtk.Property.get=pango2_context_get_base_gravity org.gtk.Property.set=pango2_context_set_base_gravity)
   *
   * The base gravity for the context.
   *
   * The base gravity is used in laying vertical text out.
   */
  properties[PROP_BASE_GRAVITY] =
    g_param_spec_enum ("base-gravity", NULL, NULL, PANGO2_TYPE_GRAVITY,
                       PANGO2_GRAVITY_SOUTH,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:gravity-hint: (attributes org.gtk.Property.get=pango2_context_get_gravity_hint org.gtk.Property.set=pango2_context_set_gravity_hint)
   *
   * The gravity hint for the context.
   *
   * The gravity hint is used in laying vertical text out, and
   * is only relevant if gravity of the context as returned by
   * [method@Pango2.Context.get_gravity] is set to `PANGO2_GRAVITY_EAST`
   * or `PANGO2_GRAVITY_WEST`.
   */
  properties[PROP_GRAVITY_HINT] =
    g_param_spec_enum ("gravity-hint", NULL, NULL, PANGO2_TYPE_GRAVITY_HINT,
                       PANGO2_GRAVITY_HINT_NATURAL,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:matrix: (attributes org.gtk.Property.get=pango2_context_get_matrix org.gtk.Property.set=pango2_context_set_matrix)
   *
   * The 'user to device' transformation that will be applied when rendering
   * with this context.
   *
   * This matrix is also known as the current transformation matrix, or 'ctm'.
   *
   * The transformation is needed in cases where the font rendering applies
   * hinting that depends on knowing the position of text with respect to
   * the pixel grid.
   */
  properties[PROP_MATRIX] =
    g_param_spec_boxed ("matrix", NULL, NULL, PANGO2_TYPE_MATRIX,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:round-glyph-positions: (attributes org.gtk.Property.get=pango2_context_get_round_glyph_positions org.gtk.Property.set=pango2_context_set_round_glyph_positions)
   *
   * Determines whether font rendering with this context should
   * round glyph positions and widths to integral positions,
   * in device units.
   *
   * This is useful when the renderer can't handle subpixel
   * positioning of glyphs.
   */
  properties[PROP_ROUND_GLYPH_POSITIONS] =
    g_param_spec_boolean ("round-glyph-positions", NULL, NULL, TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Context:palette: (attributes org.gtk.Property.get=pango2_context_get_palette org.gtk.Property.set=pango2_context_set_palette)
   *
   * The name of the color palette to use for color fonts.
   */
  properties[PROP_PALETTE] =
    g_param_spec_string ("palette", NULL, NULL, "default",
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
pango2_context_finalize (GObject *object)
{
  Pango2Context *context = PANGO2_CONTEXT (object);

  if (context->font_map)
    g_object_unref (context->font_map);

  pango2_font_description_free (context->font_desc);
  if (context->matrix)
    pango2_matrix_free (context->matrix);

  if (context->metrics)
    pango2_font_metrics_free (context->metrics);

  G_OBJECT_CLASS (pango2_context_parent_class)->finalize (object);
}

/**
 * pango2_context_new:
 *
 * Creates a new `Pango2Context` initialized to default values.
 *
 * If you want to use a specific [class@Pango2.FontMap] other than
 * the default one, you should use [ctor@Pango2.Context.new_with_font_map]
 * instead.
 *
 * If you are using Pango as part of a higher-level system,
 * that system may have it's own way of create a `Pango2Context`.
 * Pango2's own cairo support for instance, has [func@Pango2.cairo_create_context],
 * and the GTK toolkit has, among others, gtk_widget_get_pango2_context().
 * Use those instead.
 *
 * Return value: the newly allocated `Pango2Context`
 */
Pango2Context *
pango2_context_new (void)
{
  return g_object_new (PANGO2_TYPE_CONTEXT,
                       "font-map", pango2_font_map_get_default (),
                       NULL);
}

/**
 * pango2_context_new_with_font_map:
 * @font_map: the `Pango2FontMap` to use
 *
 * Creates a new `Pango2Context` with the given font map.
 *
 * If you are using Pango as part of a higher-level system,
 * that system may have it's own way of create a `Pango2Context`.
 * Pango2's own cairo support for instance, has [func@Pango2.cairo_create_context],
 * and the GTK toolkit has, among others, gtk_widget_get_pango2_context().
 * Use those instead.
 *
 * Return value: the newly allocated `Pango2Context`
 */
Pango2Context *
pango2_context_new_with_font_map (Pango2FontMap *font_map)
{
  return g_object_new (PANGO2_TYPE_CONTEXT,
                       "font-map", font_map,
                       NULL);
}

static void
update_resolved_gravity (Pango2Context *context)
{
  if (context->base_gravity == PANGO2_GRAVITY_AUTO)
    context->resolved_gravity = pango2_gravity_get_for_matrix (context->matrix);
  else
    context->resolved_gravity = context->base_gravity;
}

/**
 * pango2_context_set_matrix:
 * @context: a `Pango2Context`
 * @matrix: (nullable): a `Pango2Matrix`, or %NULL to unset any existing
 * matrix. (No matrix set is the same as setting the identity matrix.)
 *
 * Sets the 'user to device' transformation that will be applied when rendering
 * with this context.
 *
 * This matrix is also known as the current transformation matrix, or 'ctm'.
 *
 * The transformation is needed in cases where the font rendering applies
 * hinting that depends on knowing the position of text with respect to
 * the pixel grid.
 *
 * Note that reported metrics are in the user space coordinates before
 * the application of the matrix, not device-space coordinates after the
 * application of the matrix. So, they don't scale with the matrix, though
 * they may change slightly for different matrices, depending on how the
 * text is fit to the pixel grid.
 */
void
pango2_context_set_matrix (Pango2Context      *context,
                           const Pango2Matrix *matrix)
{
  g_return_if_fail (PANGO2_IS_CONTEXT (context));

  if (context->matrix || matrix)
    context_changed (context);

  if (context->matrix)
    pango2_matrix_free (context->matrix);
  if (matrix)
    context->matrix = pango2_matrix_copy (matrix);
  else
    context->matrix = NULL;

  update_resolved_gravity (context);
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_MATRIX]);
}

/**
 * pango2_context_get_matrix:
 * @context: a `Pango2Context`
 *
 * Gets the transformation matrix that will be applied when
 * rendering with this context.
 *
 * See [method@Pango2.Context.set_matrix].
 *
 * Return value: (nullable): the matrix, or %NULL if no matrix has
 *   been set (which is the same as the identity matrix). The returned
 *   matrix is owned by Pango and must not be modified or freed.
 */
const Pango2Matrix *
pango2_context_get_matrix (Pango2Context *context)
{
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);

  return context->matrix;
}

/**
 * pango2_context_set_font_map:
 * @context: a `Pango2Context`
 * @font_map: the `Pango2FontMap` to set.
 *
 * Sets the font map to be searched when fonts are looked-up
 * in this context.
 *
 * This is only for internal use by Pango backends, a `Pango2Context`
 * obtained via one of the recommended methods should already have a
 * suitable font map.
 */
void
pango2_context_set_font_map (Pango2Context *context,
                             Pango2FontMap *font_map)
{
  g_return_if_fail (PANGO2_IS_CONTEXT (context));
  g_return_if_fail (!font_map || PANGO2_IS_FONT_MAP (font_map));

  if (font_map == context->font_map)
    return;

  context_changed (context);

  if (font_map)
    g_object_ref (font_map);

  if (context->font_map)
    g_object_unref (context->font_map);

  context->font_map = font_map;
  context->fontmap_serial = pango2_font_map_get_serial (font_map);

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_FONT_MAP]);
}

/**
 * pango2_context_get_font_map:
 * @context: a `Pango2Context`
 *
 * Gets the `Pango2FontMap` used to look up fonts for this context.
 *
 * Return value: (transfer none): the font map for the `Pango2Context`.
 *   This value is owned by Pango and should not be unreferenced.
 */
Pango2FontMap *
pango2_context_get_font_map (Pango2Context *context)
{
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);

  return context->font_map;
}

/**
 * pango2_context_load_font:
 * @context: a `Pango2Context`
 * @desc: a `Pango2FontDescription` describing the font to load
 *
 * Loads the font in one of the fontmaps in the context
 * that is the closest match for @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated `Pango2Font`
 *   that was loaded, or %NULL if no font matched.
 */
Pango2Font *
pango2_context_load_font (Pango2Context               *context,
                         const Pango2FontDescription *desc)
{
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (context->font_map != NULL, NULL);

  return pango2_font_map_load_font (context->font_map, context, desc);
}

/**
 * pango2_context_load_fontset:
 * @context: a `Pango2Context`
 * @desc: a `Pango2FontDescription` describing the fonts to load
 * @language: a `Pango2Language` the fonts will be used for
 *
 * Load a set of fonts in the context that can be used to render
 * a font matching @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated
 *   `Pango2Fontset` loaded, or %NULL if no font matched.
 */
Pango2Fontset *
pango2_context_load_fontset (Pango2Context               *context,
                             const Pango2FontDescription *desc,
                             Pango2Language              *language)
{
  g_return_val_if_fail (context != NULL, NULL);

  return pango2_font_map_load_fontset (context->font_map, context, desc, language);
}

/**
 * pango2_context_set_font_description:
 * @context: a `Pango2Context`
 * @desc: the new pango font description
 *
 * Set the default font description for the context
 */
void
pango2_context_set_font_description (Pango2Context               *context,
                                     const Pango2FontDescription *desc)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (desc != NULL);

  if (desc != context->font_desc &&
      (!desc || !context->font_desc || !pango2_font_description_equal(desc, context->font_desc)))
    {
      context_changed (context);

      pango2_font_description_free (context->font_desc);
      context->font_desc = pango2_font_description_copy (desc);

      g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_FONT_DESCRIPTION]);
    }
}

/**
 * pango2_context_get_font_description:
 * @context: a `Pango2Context`
 *
 * Retrieve the default font description for the context.
 *
 * Return value: (transfer none): a pointer to the context's default font
 *   description. This value must not be modified or freed.
 */
Pango2FontDescription *
pango2_context_get_font_description (Pango2Context *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->font_desc;
}

/**
 * pango2_context_set_language:
 * @context: a `Pango2Context`
 * @language: the new language tag.
 *
 * Sets the global language tag for the context.
 *
 * The default language for the locale of the running process
 * can be found using [func@Pango2.Language.get_default].
 */
void
pango2_context_set_language (Pango2Context  *context,
                             Pango2Language *language)
{
  g_return_if_fail (context != NULL);

  if (language != context->language)
    context_changed (context);

  context->set_language = language;
  if (language)
    context->language = language;
  else
    context->language = pango2_language_get_default ();

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_LANGUAGE]);
}

/**
 * pango2_context_get_language:
 * @context: a `Pango2Context`
 *
 * Retrieves the global language tag for the context.
 *
 * Return value: the global language tag.
 */
Pango2Language *
pango2_context_get_language (Pango2Context *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->set_language;
}

/**
 * pango2_context_set_base_dir:
 * @context: a `Pango2Context`
 * @direction: the new base direction
 *
 * Sets the base direction for the context.
 *
 * The base direction is used in applying the Unicode bidirectional
 * algorithm; if the @direction is %PANGO2_DIRECTION_LTR or
 * %PANGO2_DIRECTION_RTL, then the value will be used as the paragraph
 * direction in the Unicode bidirectional algorithm. A value of
 * %PANGO2_DIRECTION_WEAK_LTR or %PANGO2_DIRECTION_WEAK_RTL is used only
 * for paragraphs that do not contain any strong characters themselves.
 */
void
pango2_context_set_base_dir (Pango2Context   *context,
                             Pango2Direction  direction)
{
  g_return_if_fail (context != NULL);

  if (direction == context->base_dir)
    return;

  context_changed (context);

  context->base_dir = direction;
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_BASE_DIR]);
}

/**
 * pango2_context_get_base_dir:
 * @context: a `Pango2Context`
 *
 * Retrieves the base direction for the context.
 *
 * See [method@Pango2.Context.set_base_dir].
 *
 * Return value: the base direction for the context.
 */
Pango2Direction
pango2_context_get_base_dir (Pango2Context *context)
{
  g_return_val_if_fail (context != NULL, PANGO2_DIRECTION_LTR);

  return context->base_dir;
}

/**
 * pango2_context_set_base_gravity:
 * @context: a `Pango2Context`
 * @gravity: the new base gravity
 *
 * Sets the base gravity for the context.
 *
 * The base gravity is used in laying vertical text out.
 */
void
pango2_context_set_base_gravity (Pango2Context *context,
                                 Pango2Gravity  gravity)
{
  g_return_if_fail (context != NULL);

  if (gravity == context->base_gravity)
    return;

  context_changed (context);

  context->base_gravity = gravity;

  update_resolved_gravity (context);
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_BASE_GRAVITY]);
}

/**
 * pango2_context_get_base_gravity:
 * @context: a `Pango2Context`
 *
 * Retrieves the base gravity for the context.
 *
 * See [method@Pango2.Context.set_base_gravity].
 *
 * Return value: the base gravity for the context.
 */
Pango2Gravity
pango2_context_get_base_gravity (Pango2Context *context)
{
  g_return_val_if_fail (context != NULL, PANGO2_GRAVITY_SOUTH);

  return context->base_gravity;
}

/**
 * pango2_context_get_gravity:
 * @context: a `Pango2Context`
 *
 * Retrieves the gravity for the context.
 *
 * This is similar to [method@Pango2.Context.get_base_gravity],
 * except for when the base gravity is %PANGO2_GRAVITY_AUTO for
 * which [func@Pango2.Gravity.get_for_matrix] is used to return the
 * gravity from the current context matrix.
 *
 * Return value: the resolved gravity for the context.
 */
Pango2Gravity
pango2_context_get_gravity (Pango2Context *context)
{
  g_return_val_if_fail (context != NULL, PANGO2_GRAVITY_SOUTH);

  return context->resolved_gravity;
}

/**
 * pango2_context_set_gravity_hint:
 * @context: a `Pango2Context`
 * @hint: the new gravity hint
 *
 * Sets the gravity hint for the context.
 *
 * The gravity hint is used in laying vertical text out, and
 * is only relevant if gravity of the context as returned by
 * [method@Pango2.Context.get_gravity] is set to %PANGO2_GRAVITY_EAST
 * or %PANGO2_GRAVITY_WEST.
 */
void
pango2_context_set_gravity_hint (Pango2Context     *context,
                                 Pango2GravityHint  hint)
{
  g_return_if_fail (context != NULL);

  if (hint == context->gravity_hint)
    return;

  context_changed (context);

  context->gravity_hint = hint;
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_GRAVITY_HINT]);
}

/**
 * pango2_context_get_gravity_hint:
 * @context: a `Pango2Context`
 *
 * Retrieves the gravity hint for the context.
 *
 * See [method@Pango2.Context.set_gravity_hint] for details.
 *
 * Return value: the gravity hint for the context.
 */
Pango2GravityHint
pango2_context_get_gravity_hint (Pango2Context *context)
{
  g_return_val_if_fail (context != NULL, PANGO2_GRAVITY_HINT_NATURAL);

  return context->gravity_hint;
}


static gboolean
get_first_metrics_foreach (Pango2Fontset *fontset,
                           Pango2Font    *font,
                           gpointer       data)
{
  Pango2FontMetrics **fontset_metrics = data;
  Pango2Language *language = pango2_fontset_get_language (fontset);

  *fontset_metrics = pango2_font_get_metrics (font, language);

  return TRUE; /* Stops iteration */
}

static Pango2FontMetrics *
get_base_metrics (Pango2Fontset *fontset)
{
  Pango2FontMetrics *metrics = NULL;

  /* Initialize the metrics from the first font in the fontset */
  pango2_fontset_foreach (fontset, get_first_metrics_foreach, &metrics);

  return metrics;
}

static void
update_metrics_from_items (Pango2FontMetrics *metrics,
                           Pango2Language    *language,
                           const char        *text,
                           unsigned int       text_len,
                           GList             *items)

{
  GHashTable *fonts_seen = g_hash_table_new (NULL, NULL);
  Pango2GlyphString *glyphs = pango2_glyph_string_new ();
  GList *l;
  glong text_width;

  /* This should typically be called with a sample text string. */
  g_return_if_fail (text_len > 0);

  metrics->approximate_char_width = 0;

  for (l = items; l; l = l->next)
    {
      Pango2Item *item = l->data;
      Pango2Font *font = item->analysis.font;

      if (font != NULL && g_hash_table_lookup (fonts_seen, font) == NULL)
        {
          Pango2FontMetrics *raw_metrics = pango2_font_get_metrics (font, language);
          g_hash_table_insert (fonts_seen, font, font);

          /* metrics will already be initialized from the first font in the fontset */
          metrics->ascent = MAX (metrics->ascent, raw_metrics->ascent);
          metrics->descent = MAX (metrics->descent, raw_metrics->descent);
          metrics->height = MAX (metrics->height, raw_metrics->height);
          pango2_font_metrics_free (raw_metrics);
        }

      pango2_shape (text + item->offset, item->length,
                   text, text_len,
                   &item->analysis, glyphs,
                   PANGO2_SHAPE_NONE);

      metrics->approximate_char_width += pango2_glyph_string_get_width (glyphs);
    }

  pango2_glyph_string_free (glyphs);
  g_hash_table_destroy (fonts_seen);

  text_width = pango2_utf8_strwidth (text);
  g_assert (text_width > 0);
  metrics->approximate_char_width /= text_width;
}

/**
 * pango2_context_get_metrics:
 * @context: a `Pango2Context`
 * @desc: (nullable): a `Pango2FontDescription` structure. %NULL means that the
 *   font description from the context will be used.
 * @language: (nullable): language tag used to determine which script to get
 *   the metrics for. %NULL means that the language tag from the context
 *   will be used. If no language tag is set on the context, metrics
 *   for the default language (as determined by [func@Pango2.Language.get_default]
 *   will be returned.
 *
 * Get overall metric information for a particular font description.
 *
 * Since the metrics may be substantially different for different scripts,
 * a language tag can be provided to indicate that the metrics should be
 * retrieved that correspond to the script(s) used by that language.
 *
 * The `Pango2FontDescription` is interpreted in the same way as by [func@itemize],
 * and the family name may be a comma separated list of names. If characters
 * from multiple of these families would be used to render the string, then
 * the returned fonts would be a composite of the metrics for the fonts loaded
 * for the individual families.
 *
 * Return value: a `Pango2FontMetrics` object. The caller must call
 *   [method@Pango2.FontMetrics.free] when finished using the object.
 */
Pango2FontMetrics *
pango2_context_get_metrics (Pango2Context               *context,
                            const Pango2FontDescription *desc,
                            Pango2Language              *language)
{
  Pango2Fontset *current_fonts = NULL;
  Pango2FontMetrics *metrics;
  const char *sample_str;
  unsigned int text_len;
  GList *items;

  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);

  if (!desc)
    desc = context->font_desc;

  if (!language)
    language = context->language;

  if (desc == context->font_desc &&
      language == context->language &&
      context->metrics != NULL)
    return pango2_font_metrics_copy (context->metrics);

  current_fonts = pango2_font_map_load_fontset (context->font_map, context, desc, language);
  metrics = get_base_metrics (current_fonts);

  sample_str = pango2_language_get_sample_string (language);
  text_len = strlen (sample_str);
  items = pango2_itemize_with_font (context, context->base_dir,
                                    sample_str, 0, text_len,
                                    NULL, NULL,
                                    desc);

  update_metrics_from_items (metrics, language, sample_str, text_len, items);

  g_list_foreach (items, (GFunc)pango2_item_free, NULL);
  g_list_free (items);

  g_object_unref (current_fonts);

  if (desc == context->font_desc &&
      language == context->language)
    {
      pango2_font_metrics_free (context->metrics);
      context->metrics = pango2_font_metrics_copy (metrics);
    }

  return metrics;
}

static void
context_changed (Pango2Context *context)
{
  context->serial++;
  if (context->serial == 0)
    context->serial++;

  g_clear_pointer (&context->metrics, pango2_font_metrics_free);
}

/**
 * pango2_context_changed:
 * @context: a `Pango2Context`
 *
 * Forces a change in the context, which will cause any `Pango2Layout`
 * using this context to re-layout.
 *
 * This function is only useful when implementing a new backend
 * for Pango2, something applications won't do. Backends should
 * call this function if they have attached extra data to the context
 * and such data is changed.
 */
void
pango2_context_changed (Pango2Context *context)
{
  context_changed (context);
}

static void
check_fontmap_changed (Pango2Context *context)
{
  guint old_serial = context->fontmap_serial;

  if (!context->font_map)
    return;

  context->fontmap_serial = pango2_font_map_get_serial (context->font_map);

  if (old_serial != context->fontmap_serial)
    context_changed (context);
}

/**
 * pango2_context_get_serial:
 * @context: a `Pango2Context`
 *
 * Returns the current serial number of @context.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new context is created and is increased whenever the context
 * is changed using any of the setter functions, or the `Pango2FontMap` it
 * uses to find fonts has changed. The serial may wrap, but will never
 * have the value 0. Since it can wrap, never compare it with "less than",
 * always use "not equals".
 *
 * This can be used to automatically detect changes to a `Pango2Context`,
 * and is only useful when implementing objects that need update when their
 * `Pango2Context` changes, like `Pango2Layout`.
 *
 * Return value: The current serial number of @context.
 */
guint
pango2_context_get_serial (Pango2Context *context)
{
  check_fontmap_changed (context);
  return context->serial;
}

/**
 * pango2_context_set_round_glyph_positions:
 * @context: a `Pango2Context`
 * @round_positions: whether to round glyph positions
 *
 * Sets whether font rendering with this context should
 * round glyph positions and widths to integral positions,
 * in device units.
 *
 * This is useful when the renderer can't handle subpixel
 * positioning of glyphs.
 *
 * The default value is to round glyph positions, to remain
 * compatible with previous Pango behavior.
 */
void
pango2_context_set_round_glyph_positions (Pango2Context *context,
                                          gboolean       round_positions)
{
  if (context->round_glyph_positions == round_positions)
    return;

  context->round_glyph_positions = round_positions;
  context_changed (context);
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_ROUND_GLYPH_POSITIONS]);
}

/**
 * pango2_context_get_round_glyph_positions:
 * @context: a `Pango2Context`
 *
 * Returns whether font rendering with this context should
 * round glyph positions and widths.
 */
gboolean
pango2_context_get_round_glyph_positions (Pango2Context *context)
{
  return context->round_glyph_positions;
}

/**
 * pango2_context_set_palette:
 * @context: a `Pango2Context`
 * @palette: the name of the palette to use
 *
 * Sets the default palette to use for color fonts.
 *
 * This can either be one of the predefined names
 * "default", "light" or "dark", a name referring
 * to a palette by index ("palette0", "palette1", …),
 * or a custom name.
 *
 * Some color fonts include metadata that indicates
 * the default palette, as well as palettes that work
 * well on light or dark backgrounds. The predefined
 * names select those.
 *
 * To associate custom names with palettes in fonts,
 * use [method@Pango2.HbFace.set_palette_name].
 */
void
pango2_context_set_palette (Pango2Context *context,
                            const char    *palette)
{
  GQuark quark;

  g_return_if_fail (PANGO2_IS_CONTEXT (context));

  quark = g_quark_from_string (palette);
  if (context->palette == quark)
    return;

  context->palette = quark;
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_PALETTE]);
}

/**
 * pango2_context_get_palette:
 * @context: a `Pango2Context`
 *
 * Returns the default palette to use for color fonts.
 *
 * See [method@Pango2.Context.set_palette].
 *
 * Return value: (nullable): the palette name
 */
const char *
pango2_context_get_palette (Pango2Context *context)
{
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), PANGO2_COLOR_PALETTE_DEFAULT);

  return g_quark_to_string (context->palette);
}
