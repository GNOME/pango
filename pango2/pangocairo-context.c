/* Pango2
 * pangocairo-context.c: Cairo context handling
 *
 * Copyright (C) 2000-2005 Red Hat Software
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

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-impl-utils.h"
#include "pango-context-private.h"

#include <string.h>

/**
 * PANGO2_HAS_CAIRO:
 *
 * Defined to 1 at compile time if Pango was built with cairo support.
 */

/**
 * pango2_cairo_update_context:
 * @cr: a Cairo context
 * @context: a `Pango2Context`, from a pangocairo font map
 *
 * Updates a `Pango2Context` previously created for use with Cairo to
 * match the current transformation and target surface of a Cairo
 * context.
 *
 * If any layouts have been created for the context, it's necessary
 * to call [method@Pango2.Layout.context_changed] on those layouts.
 */
void
pango2_cairo_update_context (cairo_t       *cr,
                             Pango2Context *context)
{
  cairo_matrix_t cairo_matrix;
  cairo_surface_t *target;
  Pango2Matrix pango2_matrix;
  const Pango2Matrix *current_matrix, identity_matrix = PANGO2_MATRIX_INIT;
  const cairo_font_options_t *merged_options;
  cairo_font_options_t *old_merged_options;
  gboolean changed = FALSE;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO2_IS_CONTEXT (context));

  target = cairo_get_target (cr);

  if (!context->surface_options)
    context->surface_options = cairo_font_options_create ();
  cairo_surface_get_font_options (target, context->surface_options);
  if (!context->set_options_explicit)
  {
    if (!context->set_options)
      context->set_options = cairo_font_options_create ();
    cairo_get_font_options (cr, context->set_options);
  }

  old_merged_options = context->merged_options;
  context->merged_options = NULL;

  merged_options = pango2_cairo_context_get_merged_font_options (context);

  if (old_merged_options)
    {
      if (!cairo_font_options_equal (merged_options, old_merged_options))
        changed = TRUE;
      cairo_font_options_destroy (old_merged_options);
      old_merged_options = NULL;
    }
  else
    changed = TRUE;

  cairo_get_matrix (cr, &cairo_matrix);
  pango2_matrix.xx = cairo_matrix.xx;
  pango2_matrix.yx = cairo_matrix.yx;
  pango2_matrix.xy = cairo_matrix.xy;
  pango2_matrix.yy = cairo_matrix.yy;
  pango2_matrix.x0 = 0;
  pango2_matrix.y0 = 0;

  current_matrix = pango2_context_get_matrix (context);
  if (!current_matrix)
    current_matrix = &identity_matrix;

  /* layout is matrix-independent if metrics-hinting is off.
   * also ignore matrix translation offsets
   */
  if ((cairo_font_options_get_hint_metrics (merged_options) != CAIRO_HINT_METRICS_OFF) &&
      (0 != memcmp (&pango2_matrix, current_matrix, sizeof (Pango2Matrix))))
    changed = TRUE;

  pango2_context_set_matrix (context, &pango2_matrix);

  if (changed)
    pango2_context_changed (context);
}

/**
 * pango2_cairo_context_set_font_options:
 * @context: a `Pango2Context`, from a pangocairo font map
 * @options: (nullable): a `cairo_font_options_t`, or %NULL to unset
 *   any previously set options. A copy is made.
 *
 * Sets the font options used when rendering text with this context.
 *
 * These options override any options that [func@Pango2.cairo_update_context]
 * derives from the target surface.
 */
void
pango2_cairo_context_set_font_options (Pango2Context              *context,
                                       const cairo_font_options_t *options)
{
  g_return_if_fail (PANGO2_IS_CONTEXT (context));

  if (!context->set_options && !options)
    return;

  if (context->set_options && options &&
      cairo_font_options_equal (context->set_options, options))
    return;

  if (context->set_options || options)
    pango2_context_changed (context);

 if (context->set_options)
    cairo_font_options_destroy (context->set_options);

  if (options)
  {
    context->set_options = cairo_font_options_copy (options);
    context->set_options_explicit = TRUE;
  }
  else
  {
    context->set_options = NULL;
    context->set_options_explicit = FALSE;
  }

  if (context->merged_options)
    {
      cairo_font_options_destroy (context->merged_options);
      context->merged_options = NULL;
    }
}

/**
 * pango2_cairo_context_get_font_options:
 * @context: a `Pango2Context`, from a pangocairo font map
 *
 * Retrieves any font rendering options previously set with
 * [func@Pango2.cairo_context_set_font_options].
 *
 * This function does not report options that are derived from
 * the target surface by [func@Pango2.cairo_update_context].
 *
 * Return value: (nullable): the font options previously set on the
 *   context, or %NULL if no options have been set. This value is
 *   owned by the context and must not be modified or freed.
 */
const cairo_font_options_t *
pango2_cairo_context_get_font_options (Pango2Context *context)
{
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);

  return context->set_options;
}

const cairo_font_options_t *
pango2_cairo_context_get_merged_font_options (Pango2Context *context)
{
  if (!context->merged_options)
    {
      context->merged_options = cairo_font_options_create ();

      if (context->surface_options)
        cairo_font_options_merge (context->merged_options, context->surface_options);
      if (context->set_options)
        cairo_font_options_merge (context->merged_options, context->set_options);
    }

  return context->merged_options;
}

/**
 * pango2_cairo_create_context:
 * @cr: a Cairo context
 *
 * Creates a context object set up to match the current transformation
 * and target surface of the Cairo context.
 *
 * This context can then be
 * used to create a layout using [ctor@Pango2.Layout.new].
 *
 * This function is a convenience function that creates a context
 * using the default font map, then updates it to @cr.
 *
 * Return value: (transfer full): the newly created `Pango2Context`
 */
Pango2Context *
pango2_cairo_create_context (cairo_t *cr)
{
  Pango2Context *context;

  g_return_val_if_fail (cr != NULL, NULL);

  context = pango2_context_new ();
  pango2_cairo_update_context (cr, context);

  return context;
}

/**
 * pango2_cairo_update_layout:
 * @cr: a Cairo context
 * @layout: a `Pango2Layout`
 *
 * Updates the private `Pango2Context` of a `Pango2Layout` to match
 * the current transformation and target surface of a Cairo context.
 */
void
pango2_cairo_update_layout (cairo_t     *cr,
                           Pango2Layout *layout)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  pango2_cairo_update_context (cr, pango2_layout_get_context (layout));
}

/**
 * pango2_cairo_create_layout:
 * @cr: a Cairo context
 *
 * Creates a layout object set up to match the current transformation
 * and target surface of the Cairo context.
 *
 * This layout can then be used for text measurement with functions
 * like [method@Pango2.Lines.get_size] or drawing with functions like
 * [func@Pango2.cairo_show_layout]. If you change the transformation or target
 * surface for @cr, you need to call [func@Pango2.cairo_update_layout].
 *
 * This function is the most convenient way to use Cairo with Pango2,
 * however it is slightly inefficient since it creates a separate
 * `Pango2Context` object for each layout. This might matter in an
 * application that was laying out large amounts of text.
 *
 * Return value: (transfer full): the newly created `Pango2Layout`
 */
Pango2Layout *
pango2_cairo_create_layout (cairo_t *cr)
{
  Pango2Context *context;
  Pango2Layout *layout;

  g_return_val_if_fail (cr != NULL, NULL);

  context = pango2_cairo_create_context (cr);
  layout = pango2_layout_new (context);
  g_object_unref (context);

  return layout;
}
