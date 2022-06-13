/* Pango
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
 * PangoContext:
 *
 * A `PangoContext` stores global information used to control the
 * itemization process.
 *
 * The information stored by `PangoContext` includes the fontmap used
 * to look up fonts, and default values such as the default language,
 * default gravity, or default font.
 *
 * To obtain a `PangoContext`, use [method@Pango.FontMap.create_context]
 * or [func@Pango.cairo_create_context].
 */

struct _PangoContextClass
{
  GObjectClass parent_class;

};

static void pango_context_finalize    (GObject       *object);
static void context_changed           (PangoContext  *context);

enum {
  PROP_FONT_MAP = 1,
  PROP_FONT_DESCRIPTION,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_BASE_GRAVITY,
  PROP_GRAVITY_HINT,
  PROP_MATRIX,
  PROP_ROUND_GLYPH_POSITIONS,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE (PangoContext, pango_context, G_TYPE_OBJECT)

static void
pango_context_init (PangoContext *context)
{
  context->base_dir = PANGO_DIRECTION_WEAK_LTR;
  context->resolved_gravity = context->base_gravity = PANGO_GRAVITY_SOUTH;
  context->gravity_hint = PANGO_GRAVITY_HINT_NATURAL;

  context->serial = 1;
  context->set_language = NULL;
  context->language = pango_language_get_default ();
  context->font_map = NULL;
  context->round_glyph_positions = TRUE;

  context->font_desc = pango_font_description_new ();
  pango_font_description_set_family_static (context->font_desc, "serif");
  pango_font_description_set_style (context->font_desc, PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (context->font_desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (context->font_desc, PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (context->font_desc, PANGO_STRETCH_NORMAL);
  pango_font_description_set_size (context->font_desc, 12 * PANGO_SCALE);
}

static gboolean
pango_has_mixed_deps (void)
{
  GModule *module;
  gpointer func;
  gboolean result = FALSE;

  module = g_module_open (NULL, 0);

  if (g_module_symbol (module, "pango_ot_info_find_script", &func))
    result = TRUE;

  g_module_close (module);

  return result;
}

static void
pango_context_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  PangoContext *context = PANGO_CONTEXT (object);

  switch (property_id)
    {
    case PROP_FONT_MAP:
      pango_context_set_font_map (context, g_value_get_object (value));
      break;

    case PROP_FONT_DESCRIPTION:
      pango_context_set_font_description (context, g_value_get_boxed (value));
      break;

    case PROP_LANGUAGE:
      pango_context_set_language (context, g_value_get_boxed (value));
      break;

    case PROP_BASE_DIR:
      pango_context_set_base_dir (context, g_value_get_enum (value));
      break;

    case PROP_BASE_GRAVITY:
      pango_context_set_base_gravity (context, g_value_get_enum (value));
      break;

    case PROP_GRAVITY_HINT:
      pango_context_set_gravity_hint (context, g_value_get_enum (value));
      break;

    case PROP_MATRIX:
      pango_context_set_matrix (context, g_value_get_boxed (value));
      break;

    case PROP_ROUND_GLYPH_POSITIONS:
      pango_context_set_round_glyph_positions (context, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango_context_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  PangoContext *context = PANGO_CONTEXT (object);

  switch (property_id)
    {
    case PROP_FONT_MAP:
      g_value_set_object (value, pango_context_get_font_map (context));
      break;

    case PROP_FONT_DESCRIPTION:
      g_value_set_boxed (value, pango_context_get_font_description (context));
      break;

    case PROP_LANGUAGE:
      g_value_set_boxed (value, pango_context_get_language (context));
      break;

    case PROP_BASE_DIR:
      g_value_set_enum (value, pango_context_get_base_dir (context));
      break;

    case PROP_BASE_GRAVITY:
      g_value_set_enum (value, pango_context_get_base_gravity (context));
      break;

    case PROP_GRAVITY_HINT:
      g_value_set_enum (value, pango_context_get_gravity_hint (context));
      break;

    case PROP_MATRIX:
      g_value_set_boxed (value, pango_context_get_matrix (context));
      break;

    case PROP_ROUND_GLYPH_POSITIONS:
      g_value_set_boolean (value, pango_context_get_round_glyph_positions (context));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango_context_class_init (PangoContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* Put the check for mixed linkage here, for lack of a better place */
  if (pango_has_mixed_deps ())
    g_error ("Pango 1.x symbols detected.\n"
             "Using Pango 1.x and 2 in the same process is not supported.");

  object_class->finalize = pango_context_finalize;
  object_class->set_property = pango_context_set_property;
  object_class->get_property = pango_context_get_property;

  /**
   * PangoContext:font-map: (attributes org.gtk.Property.get=pango_context_get_font_map org.gtk.Property.set=pango_context_set_font_map)
   *
   * The font map to be searched when fonts are looked up
   * in this context.
   */
  properties[PROP_FONT_MAP] =
    g_param_spec_object ("font-map", NULL, NULL, PANGO_TYPE_FONT_MAP,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:font-description: (attributes org.gtk.Property.get=pango_context_get_font_description org.gtk.Property.set=pango_context_set_font_description)
   *
   * The default font descriptoin for the context.
   */
  properties[PROP_FONT_DESCRIPTION] =
    g_param_spec_boxed ("font-description", NULL, NULL, PANGO_TYPE_FONT_DESCRIPTION,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:language: (attributes org.gtk.Property.get=pango_context_get_language org.gtk.Property.set=pango_context_set_language)
   *
   * The global language for the context.
   */
  properties[PROP_LANGUAGE] =
    g_param_spec_boxed ("language", NULL, NULL, PANGO_TYPE_LANGUAGE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:base-direction: (attributes org.gtk.Property.get=pango_context_get_base_dir org.gtk.Property.set=pango_context_set_base_dir)
   *
   * The base direction for the context.
   *
   * The base direction is used in applying the Unicode bidirectional
   * algorithm; if the @direction is `PANGO_DIRECTION_LTR` or
   * `PANGO_DIRECTION_RTL`, then the value will be used as the paragraph
   * direction in the Unicode bidirectional algorithm. A value of
   * `PANGO_DIRECTION_WEAK_LTR` or `PANGO_DIRECTION_WEAK_RTL` is used only
   * for paragraphs that do not contain any strong characters themselves.
   */
  properties[PROP_BASE_DIR] =
    g_param_spec_enum ("base-direction", NULL, NULL, PANGO_TYPE_DIRECTION,
                       PANGO_DIRECTION_LTR,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:base-gravity: (attributes org.gtk.Property.get=pango_context_get_base_gravity org.gtk.Property.set=pango_context_set_base_gravity)
   *
   * The base gravity for the context.
   *
   * The base gravity is used in laying vertical text out.
   */
  properties[PROP_BASE_GRAVITY] =
    g_param_spec_enum ("base-gravity", NULL, NULL, PANGO_TYPE_GRAVITY,
                       PANGO_GRAVITY_SOUTH,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:gravity-hint: (attributes org.gtk.Property.get=pango_context_get_gravity_hint org.gtk.Property.set=pango_context_set_gravity_hint)
   *
   * The gravity hint for the context.
   *
   * The gravity hint is used in laying vertical text out, and
   * is only relevant if gravity of the context as returned by
   * [method@Pango.Context.get_gravity] is set to `PANGO_GRAVITY_EAST`
   * or `PANGO_GRAVITY_WEST`.
   */
  properties[PROP_GRAVITY_HINT] =
    g_param_spec_enum ("gravity-hint", NULL, NULL, PANGO_TYPE_GRAVITY_HINT,
                       PANGO_GRAVITY_HINT_NATURAL,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:matrix: (attributes org.gtk.Property.get=pango_context_get_matrix org.gtk.Property.set=pango_context_set_matrix)
   *
   * The 'user to device' transformation that will be applied when rendering
   * with this context.
   *
   * This matrix is also known as the current transformation matrix, or 'ctm'.
   *
   * The transformation is needed in cases where the font rendering applies
   * hinting that depends on knowing the position of text with respect to
   * the pixel grid. If your font rendering does not
   */
  properties[PROP_MATRIX] =
    g_param_spec_boxed ("matrix", NULL, NULL, PANGO_TYPE_MATRIX,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoContext:round-glyph-positions: (attributes org.gtk.Property.get=pango_context_get_round_glyph_positions org.gtk.Property.set=pango_context_set_round_glyph_positions)
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

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
pango_context_finalize (GObject *object)
{
  PangoContext *context;

  context = PANGO_CONTEXT (object);

  if (context->font_map)
    g_object_unref (context->font_map);

  pango_font_description_free (context->font_desc);
  if (context->matrix)
    pango_matrix_free (context->matrix);

  if (context->metrics)
    pango_font_metrics_free (context->metrics);

  G_OBJECT_CLASS (pango_context_parent_class)->finalize (object);
}

/**
 * pango_context_new:
 *
 * Creates a new `PangoContext` initialized to default values.
 *
 * This function is not particularly useful as it should always
 * be followed by a [method@Pango.Context.set_font_map] call, and the
 * function [method@Pango.FontMap.create_context] does these two steps
 * together and hence users are recommended to use that.
 *
 * If you are using Pango as part of a higher-level system,
 * that system may have it's own way of create a `PangoContext`.
 * Pango's own cairo support for instance, has [func@Pango.cairo_create_context],
 * and the GTK toolkit has, among others, gtk_widget_get_pango_context().
 * Use those instead.
 *
 * Return value: the newly allocated `PangoContext`
 */
PangoContext *
pango_context_new (void)
{
  PangoContext *context;

  context = g_object_new (PANGO_TYPE_CONTEXT, NULL);

  return context;
}

static void
update_resolved_gravity (PangoContext *context)
{
  if (context->base_gravity == PANGO_GRAVITY_AUTO)
    context->resolved_gravity = pango_gravity_get_for_matrix (context->matrix);
  else
    context->resolved_gravity = context->base_gravity;
}

/**
 * pango_context_set_matrix:
 * @context: a `PangoContext`
 * @matrix: (nullable): a `PangoMatrix`, or %NULL to unset any existing
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
pango_context_set_matrix (PangoContext      *context,
                          const PangoMatrix *matrix)
{
  g_return_if_fail (PANGO_IS_CONTEXT (context));

  if (context->matrix || matrix)
    context_changed (context);

  if (context->matrix)
    pango_matrix_free (context->matrix);
  if (matrix)
    context->matrix = pango_matrix_copy (matrix);
  else
    context->matrix = NULL;

  update_resolved_gravity (context);
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_MATRIX]);
}

/**
 * pango_context_get_matrix:
 * @context: a `PangoContext`
 *
 * Gets the transformation matrix that will be applied when
 * rendering with this context.
 *
 * See [method@Pango.Context.set_matrix].
 *
 * Return value: (nullable): the matrix, or %NULL if no matrix has
 *   been set (which is the same as the identity matrix). The returned
 *   matrix is owned by Pango and must not be modified or freed.
 */
const PangoMatrix *
pango_context_get_matrix (PangoContext *context)
{
  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  return context->matrix;
}

/**
 * pango_context_set_font_map:
 * @context: a `PangoContext`
 * @font_map: the `PangoFontMap` to set.
 *
 * Sets the font map to be searched when fonts are looked-up
 * in this context.
 *
 * This is only for internal use by Pango backends, a `PangoContext`
 * obtained via one of the recommended methods should already have a
 * suitable font map.
 */
void
pango_context_set_font_map (PangoContext *context,
                            PangoFontMap *font_map)
{
  g_return_if_fail (PANGO_IS_CONTEXT (context));
  g_return_if_fail (!font_map || PANGO_IS_FONT_MAP (font_map));

  if (font_map == context->font_map)
    return;

  context_changed (context);

  if (font_map)
    g_object_ref (font_map);

  if (context->font_map)
    g_object_unref (context->font_map);

  context->font_map = font_map;
  context->fontmap_serial = pango_font_map_get_serial (font_map);

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_FONT_MAP]);
}

/**
 * pango_context_get_font_map:
 * @context: a `PangoContext`
 *
 * Gets the `PangoFontMap` used to look up fonts for this context.
 *
 * Return value: (transfer none): the font map for the `PangoContext`.
 *   This value is owned by Pango and should not be unreferenced.
 */
PangoFontMap *
pango_context_get_font_map (PangoContext *context)
{
  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  return context->font_map;
}

/**
 * pango_context_load_font:
 * @context: a `PangoContext`
 * @desc: a `PangoFontDescription` describing the font to load
 *
 * Loads the font in one of the fontmaps in the context
 * that is the closest match for @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated `PangoFont`
 *   that was loaded, or %NULL if no font matched.
 */
PangoFont *
pango_context_load_font (PangoContext               *context,
                         const PangoFontDescription *desc)
{
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (context->font_map != NULL, NULL);

  return pango_font_map_load_font (context->font_map, context, desc);
}

/**
 * pango_context_load_fontset:
 * @context: a `PangoContext`
 * @desc: a `PangoFontDescription` describing the fonts to load
 * @language: a `PangoLanguage` the fonts will be used for
 *
 * Load a set of fonts in the context that can be used to render
 * a font matching @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated
 *   `PangoFontset` loaded, or %NULL if no font matched.
 */
PangoFontset *
pango_context_load_fontset (PangoContext               *context,
                            const PangoFontDescription *desc,
                            PangoLanguage             *language)
{
  g_return_val_if_fail (context != NULL, NULL);

  return pango_font_map_load_fontset (context->font_map, context, desc, language);
}

/**
 * pango_context_set_font_description:
 * @context: a `PangoContext`
 * @desc: the new pango font description
 *
 * Set the default font description for the context
 */
void
pango_context_set_font_description (PangoContext               *context,
                                    const PangoFontDescription *desc)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (desc != NULL);

  if (desc != context->font_desc &&
      (!desc || !context->font_desc || !pango_font_description_equal(desc, context->font_desc)))
    {
      context_changed (context);

      pango_font_description_free (context->font_desc);
      context->font_desc = pango_font_description_copy (desc);

      g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_FONT_DESCRIPTION]);
    }
}

/**
 * pango_context_get_font_description:
 * @context: a `PangoContext`
 *
 * Retrieve the default font description for the context.
 *
 * Return value: (transfer none): a pointer to the context's default font
 *   description. This value must not be modified or freed.
 */
PangoFontDescription *
pango_context_get_font_description (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->font_desc;
}

/**
 * pango_context_set_language:
 * @context: a `PangoContext`
 * @language: the new language tag.
 *
 * Sets the global language tag for the context.
 *
 * The default language for the locale of the running process
 * can be found using [func@Pango.Language.get_default].
 */
void
pango_context_set_language (PangoContext  *context,
                            PangoLanguage *language)
{
  g_return_if_fail (context != NULL);

  if (language != context->language)
    context_changed (context);

  context->set_language = language;
  if (language)
    context->language = language;
  else
    context->language = pango_language_get_default ();

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_LANGUAGE]);
}

/**
 * pango_context_get_language:
 * @context: a `PangoContext`
 *
 * Retrieves the global language tag for the context.
 *
 * Return value: the global language tag.
 */
PangoLanguage *
pango_context_get_language (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->set_language;
}

/**
 * pango_context_set_base_dir:
 * @context: a `PangoContext`
 * @direction: the new base direction
 *
 * Sets the base direction for the context.
 *
 * The base direction is used in applying the Unicode bidirectional
 * algorithm; if the @direction is %PANGO_DIRECTION_LTR or
 * %PANGO_DIRECTION_RTL, then the value will be used as the paragraph
 * direction in the Unicode bidirectional algorithm. A value of
 * %PANGO_DIRECTION_WEAK_LTR or %PANGO_DIRECTION_WEAK_RTL is used only
 * for paragraphs that do not contain any strong characters themselves.
 */
void
pango_context_set_base_dir (PangoContext   *context,
                            PangoDirection  direction)
{
  g_return_if_fail (context != NULL);

  if (direction == context->base_dir)
    return;

  context_changed (context);

  context->base_dir = direction;
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_BASE_DIR]);
}

/**
 * pango_context_get_base_dir:
 * @context: a `PangoContext`
 *
 * Retrieves the base direction for the context.
 *
 * See [method@Pango.Context.set_base_dir].
 *
 * Return value: the base direction for the context.
 */
PangoDirection
pango_context_get_base_dir (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_DIRECTION_LTR);

  return context->base_dir;
}

/**
 * pango_context_set_base_gravity:
 * @context: a `PangoContext`
 * @gravity: the new base gravity
 *
 * Sets the base gravity for the context.
 *
 * The base gravity is used in laying vertical text out.
 */
void
pango_context_set_base_gravity (PangoContext *context,
                                PangoGravity  gravity)
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
 * pango_context_get_base_gravity:
 * @context: a `PangoContext`
 *
 * Retrieves the base gravity for the context.
 *
 * See [method@Pango.Context.set_base_gravity].
 *
 * Return value: the base gravity for the context.
 */
PangoGravity
pango_context_get_base_gravity (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_GRAVITY_SOUTH);

  return context->base_gravity;
}

/**
 * pango_context_get_gravity:
 * @context: a `PangoContext`
 *
 * Retrieves the gravity for the context.
 *
 * This is similar to [method@Pango.Context.get_base_gravity],
 * except for when the base gravity is %PANGO_GRAVITY_AUTO for
 * which [func@Pango.Gravity.get_for_matrix] is used to return the
 * gravity from the current context matrix.
 *
 * Return value: the resolved gravity for the context.
 */
PangoGravity
pango_context_get_gravity (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_GRAVITY_SOUTH);

  return context->resolved_gravity;
}

/**
 * pango_context_set_gravity_hint:
 * @context: a `PangoContext`
 * @hint: the new gravity hint
 *
 * Sets the gravity hint for the context.
 *
 * The gravity hint is used in laying vertical text out, and
 * is only relevant if gravity of the context as returned by
 * [method@Pango.Context.get_gravity] is set to %PANGO_GRAVITY_EAST
 * or %PANGO_GRAVITY_WEST.
 */
void
pango_context_set_gravity_hint (PangoContext     *context,
                                PangoGravityHint  hint)
{
  g_return_if_fail (context != NULL);

  if (hint == context->gravity_hint)
    return;

  context_changed (context);

  context->gravity_hint = hint;
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_GRAVITY_HINT]);
}

/**
 * pango_context_get_gravity_hint:
 * @context: a `PangoContext`
 *
 * Retrieves the gravity hint for the context.
 *
 * See [method@Pango.Context.set_gravity_hint] for details.
 *
 * Return value: the gravity hint for the context.
 */
PangoGravityHint
pango_context_get_gravity_hint (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_GRAVITY_HINT_NATURAL);

  return context->gravity_hint;
}


static gboolean
get_first_metrics_foreach (PangoFontset  *fontset,
                           PangoFont     *font,
                           gpointer       data)
{
  PangoFontMetrics **fontset_metrics = data;
  PangoLanguage *language = pango_fontset_get_language (fontset);

  *fontset_metrics = pango_font_get_metrics (font, language);

  return TRUE; /* Stops iteration */
}

static PangoFontMetrics *
get_base_metrics (PangoFontset *fontset)
{
  PangoFontMetrics *metrics = NULL;

  /* Initialize the metrics from the first font in the fontset */
  pango_fontset_foreach (fontset, get_first_metrics_foreach, &metrics);

  return metrics;
}

static void
update_metrics_from_items (PangoFontMetrics *metrics,
                           PangoLanguage    *language,
                           const char       *text,
                           unsigned int      text_len,
                           GList            *items)

{
  GHashTable *fonts_seen = g_hash_table_new (NULL, NULL);
  PangoGlyphString *glyphs = pango_glyph_string_new ();
  GList *l;
  glong text_width;

  /* This should typically be called with a sample text string. */
  g_return_if_fail (text_len > 0);

  metrics->approximate_char_width = 0;

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      PangoFont *font = item->analysis.font;

      if (font != NULL && g_hash_table_lookup (fonts_seen, font) == NULL)
        {
          PangoFontMetrics *raw_metrics = pango_font_get_metrics (font, language);
          g_hash_table_insert (fonts_seen, font, font);

          /* metrics will already be initialized from the first font in the fontset */
          metrics->ascent = MAX (metrics->ascent, raw_metrics->ascent);
          metrics->descent = MAX (metrics->descent, raw_metrics->descent);
          metrics->height = MAX (metrics->height, raw_metrics->height);
          pango_font_metrics_free (raw_metrics);
        }

      pango_shape (text + item->offset, item->length,
                   text, text_len,
                   &item->analysis, glyphs,
                   PANGO_SHAPE_NONE);

      metrics->approximate_char_width += pango_glyph_string_get_width (glyphs);
    }

  pango_glyph_string_free (glyphs);
  g_hash_table_destroy (fonts_seen);

  text_width = pango_utf8_strwidth (text);
  g_assert (text_width > 0);
  metrics->approximate_char_width /= text_width;
}

/**
 * pango_context_get_metrics:
 * @context: a `PangoContext`
 * @desc: (nullable): a `PangoFontDescription` structure. %NULL means that the
 *   font description from the context will be used.
 * @language: (nullable): language tag used to determine which script to get
 *   the metrics for. %NULL means that the language tag from the context
 *   will be used. If no language tag is set on the context, metrics
 *   for the default language (as determined by [func@Pango.Language.get_default]
 *   will be returned.
 *
 * Get overall metric information for a particular font description.
 *
 * Since the metrics may be substantially different for different scripts,
 * a language tag can be provided to indicate that the metrics should be
 * retrieved that correspond to the script(s) used by that language.
 *
 * The `PangoFontDescription` is interpreted in the same way as by [func@itemize],
 * and the family name may be a comma separated list of names. If characters
 * from multiple of these families would be used to render the string, then
 * the returned fonts would be a composite of the metrics for the fonts loaded
 * for the individual families.
 *
 * Return value: a `PangoFontMetrics` object. The caller must call
 *   [method@Pango.FontMetrics.free] when finished using the object.
 */
PangoFontMetrics *
pango_context_get_metrics (PangoContext               *context,
                           const PangoFontDescription *desc,
                           PangoLanguage              *language)
{
  PangoFontset *current_fonts = NULL;
  PangoFontMetrics *metrics;
  const char *sample_str;
  unsigned int text_len;
  GList *items;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  if (!desc)
    desc = context->font_desc;

  if (!language)
    language = context->language;

  if (desc == context->font_desc &&
      language == context->language &&
      context->metrics != NULL)
    return pango_font_metrics_copy (context->metrics);

  current_fonts = pango_font_map_load_fontset (context->font_map, context, desc, language);
  metrics = get_base_metrics (current_fonts);

  sample_str = pango_language_get_sample_string (language);
  text_len = strlen (sample_str);
  items = pango_itemize_with_font (context, context->base_dir,
                                   sample_str, 0, text_len,
                                   NULL, NULL,
                                   desc);

  update_metrics_from_items (metrics, language, sample_str, text_len, items);

  g_list_foreach (items, (GFunc)pango_item_free, NULL);
  g_list_free (items);

  g_object_unref (current_fonts);

  if (desc == context->font_desc &&
      language == context->language)
    {
      pango_font_metrics_free (context->metrics);
      context->metrics = pango_font_metrics_copy (metrics);
    }

  return metrics;
}

static void
context_changed (PangoContext *context)
{
  context->serial++;
  if (context->serial == 0)
    context->serial++;

  g_clear_pointer (&context->metrics, pango_font_metrics_free);
}

/**
 * pango_context_changed:
 * @context: a `PangoContext`
 *
 * Forces a change in the context, which will cause any `PangoLayout`
 * using this context to re-layout.
 *
 * This function is only useful when implementing a new backend
 * for Pango, something applications won't do. Backends should
 * call this function if they have attached extra data to the context
 * and such data is changed.
 */
void
pango_context_changed (PangoContext *context)
{
  context_changed (context);
}

static void
check_fontmap_changed (PangoContext *context)
{
  guint old_serial = context->fontmap_serial;

  if (!context->font_map)
    return;

  context->fontmap_serial = pango_font_map_get_serial (context->font_map);

  if (old_serial != context->fontmap_serial)
    context_changed (context);
}

/**
 * pango_context_get_serial:
 * @context: a `PangoContext`
 *
 * Returns the current serial number of @context.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new context is created and is increased whenever the context
 * is changed using any of the setter functions, or the `PangoFontMap` it
 * uses to find fonts has changed. The serial may wrap, but will never
 * have the value 0. Since it can wrap, never compare it with "less than",
 * always use "not equals".
 *
 * This can be used to automatically detect changes to a `PangoContext`,
 * and is only useful when implementing objects that need update when their
 * `PangoContext` changes, like `PangoLayout`.
 *
 * Return value: The current serial number of @context.
 */
guint
pango_context_get_serial (PangoContext *context)
{
  check_fontmap_changed (context);
  return context->serial;
}

/**
 * pango_context_set_round_glyph_positions:
 * @context: a `PangoContext`
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
pango_context_set_round_glyph_positions (PangoContext *context,
                                         gboolean      round_positions)
{
  if (context->round_glyph_positions == round_positions)
    return;

  context->round_glyph_positions = round_positions;
  context_changed (context);
  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_ROUND_GLYPH_POSITIONS]);
}

/**
 * pango_context_get_round_glyph_positions:
 * @context: a `PangoContext`
 *
 * Returns whether font rendering with this context should
 * round glyph positions and widths.
 */
gboolean
pango_context_get_round_glyph_positions (PangoContext *context)
{
  return context->round_glyph_positions;
}
