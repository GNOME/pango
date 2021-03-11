/* Pango
 * pango-gravity.c: Gravity routines
 *
 * Copyright (C) 2006, 2007 Red Hat Software
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

/**
 * SECTION:vertical
 * @short_description:Laying text out in vertical directions
 * @title:Vertical Text
 * @see_also: pango_context_get_base_gravity(),
 * pango_context_set_base_gravity(),
 * pango_context_get_gravity(),
 * pango_context_get_gravity_hint(),
 * pango_context_set_gravity_hint(),
 * pango_font_description_set_gravity(),
 * pango_font_description_get_gravity(),
 * pango_attr_gravity_new(),
 * pango_attr_gravity_hint_new()
 *
 * Since 1.16, Pango is able to correctly lay vertical text out. In fact, it can
 * set layouts of mixed vertical and non-vertical text. This section describes
 * the types used for setting vertical text parameters.
 *
 * The way this is implemented is through the concept of *gravity*. Gravity of
 * normal Latin text is south. A gravity value of east means that glyphs will be
 * rotated ninety degrees counterclockwise. So, to render vertical text one needs
 * to set the gravity and rotate the layout using the matrix machinery already in
 * place. This has the huge advantage that most algorithms working on a #PangoLayout
 * do not need any change as the assumption that lines run in the X direction and
 * stack in the Y direction holds even for vertical text layouts.
 *
 * Applications should only need to set base gravity on #PangoContext in use, and
 * let Pango decide the gravity assigned to each run of text. This automatically
 * handles text with mixed scripts. A very common use is to set the context base
 * gravity to auto using pango_context_set_base_gravity() and rotate the layout
 * normally. Pango will make sure that Asian languages take the right form, while
 * other scripts are rotated normally.
 *
 * The correct way to set gravity on a layout is to set it on the context
 * associated with it using pango_context_set_base_gravity(). The context
 * of a layout can be accessed using pango_layout_get_context(). The currently
 * set base gravity of the context can be accessed using
 * pango_context_get_base_gravity() and the *resolved* gravity of it using
 * pango_context_get_gravity(). The resolved gravity is the same as the base
 * gravity for the most part, except that if the base gravity is set to
 * %PANGO_GRAVITY_AUTO, the resolved gravity will depend on the current matrix
 * set on context, and is derived using pango_gravity_get_for_matrix().
 *
 * The next thing an application may want to set on the context is the
 * *gravity hint*. A #PangoGravityHint instructs how different scripts should
 * react to the set base gravity.
 *
 * Font descriptions have a gravity property too, that can be set using
 * pango_font_description_set_gravity() and accessed using
 * pango_font_description_get_gravity(). However, those are rarely useful
 * from application code and are mainly used by #PangoLayout internally.
 *
 * Last but not least, one can create #PangoAttributes for gravity
 * and gravity hint using pango_attr_gravity_new() and
 * pango_attr_gravity_hint_new().
 */
#include "config.h"

#include "pango-gravity.h"

#include <math.h>

/**
 * pango_gravity_to_rotation:
 * @gravity: gravity to query
 *
 * Converts a #PangoGravity value to its natural rotation in radians.
 * @gravity should not be %PANGO_GRAVITY_AUTO.
 *
 * Note that pango_matrix_rotate() takes angle in degrees, not radians.
 * So, to call pango_matrix_rotate() with the output of this function
 * you should multiply it by (180. / G_PI).
 *
 * Return value: the rotation value corresponding to @gravity.
 *
 * Since: 1.16
 */
double
pango_gravity_to_rotation (PangoGravity gravity)
{
  double rotation;

  g_return_val_if_fail (gravity != PANGO_GRAVITY_AUTO, 0);

  switch (gravity)
    {
      default:
      case PANGO_GRAVITY_AUTO: /* shut gcc up */
      case PANGO_GRAVITY_SOUTH:	rotation =  0;		break;
      case PANGO_GRAVITY_NORTH:	rotation =  G_PI;	break;
      case PANGO_GRAVITY_EAST:	rotation = -G_PI_2;	break;
      case PANGO_GRAVITY_WEST:	rotation = +G_PI_2;	break;
    }

  return rotation;
}

/**
 * pango_gravity_get_for_matrix:
 * @matrix: (nullable): a #PangoMatrix
 *
 * Finds the gravity that best matches the rotation component
 * in a #PangoMatrix.
 *
 * Return value: the gravity of @matrix, which will never be
 * %PANGO_GRAVITY_AUTO, or %PANGO_GRAVITY_SOUTH if @matrix is %NULL
 *
 * Since: 1.16
 */
PangoGravity
pango_gravity_get_for_matrix (const PangoMatrix *matrix)
{
  PangoGravity gravity;
  double x;
  double y;

  if (!matrix)
    return PANGO_GRAVITY_SOUTH;

  x = matrix->xy;
  y = matrix->yy;

  if (fabs (x) > fabs (y))
    gravity = x > 0 ? PANGO_GRAVITY_WEST : PANGO_GRAVITY_EAST;
  else
    gravity = y < 0 ? PANGO_GRAVITY_NORTH : PANGO_GRAVITY_SOUTH;

  return gravity;
}



typedef enum
{
  PANGO_VERTICAL_DIRECTION_NONE,
  PANGO_VERTICAL_DIRECTION_TTB,
  PANGO_VERTICAL_DIRECTION_BTT
} PangoVerticalDirection;

typedef struct {
  /* PangoDirection */
  guint8 horiz_dir;		/* Orientation in horizontal context */

  /* PangoVerticalDirection */
  guint8 vert_dir;		/* Orientation in vertical context */

  /* PangoGravity */
  guint8 preferred_gravity;	/* Preferred context gravity */

  /* gboolean */
  guint8 wide;			/* Whether script is mostly wide.
				 * Wide characters are upright (ie.
				 * not rotated) in foreign context */
} PangoScriptProperties;

#define NONE PANGO_VERTICAL_DIRECTION_NONE
#define TTB  PANGO_VERTICAL_DIRECTION_TTB
#define BTT  PANGO_VERTICAL_DIRECTION_BTT

#define LTR  PANGO_DIRECTION_LTR
#define RTL  PANGO_DIRECTION_RTL
#define WEAK PANGO_DIRECTION_WEAK_LTR

#define S PANGO_GRAVITY_SOUTH
#define E PANGO_GRAVITY_EAST
#define W PANGO_GRAVITY_WEST

const PangoScriptProperties script_properties[] =
  {				/* ISO 15924 code */
      {LTR, NONE, S, FALSE},	/* Zyyy */
      {LTR, NONE, S, FALSE},	/* Qaai */
      {RTL, NONE, S, FALSE},	/* Arab */
      {LTR, NONE, S, FALSE},	/* Armn */
      {LTR, NONE, S, FALSE},	/* Beng */
      {LTR, TTB,  E, TRUE },	/* Bopo */
      {LTR, NONE, S, FALSE},	/* Cher */
      {LTR, NONE, S, FALSE},	/* Qaac */
      {LTR, NONE, S, FALSE},	/* Cyrl (Cyrs) */
      {LTR, NONE, S, FALSE},	/* Dsrt */
      {LTR, NONE, S, FALSE},	/* Deva */
      {LTR, NONE, S, FALSE},	/* Ethi */
      {LTR, NONE, S, FALSE},	/* Geor (Geon, Geoa) */
      {LTR, NONE, S, FALSE},	/* Goth */
      {LTR, NONE, S, FALSE},	/* Grek */
      {LTR, NONE, S, FALSE},	/* Gujr */
      {LTR, NONE, S, FALSE},	/* Guru */
      {LTR, TTB,  E, TRUE },	/* Hani */
      {LTR, TTB,  E, TRUE },	/* Hang */
      {RTL, NONE, S, FALSE},	/* Hebr */
      {LTR, TTB,  E, TRUE },	/* Hira */
      {LTR, NONE, S, FALSE},	/* Knda */
      {LTR, TTB,  E, TRUE },	/* Kana */
      {LTR, NONE, S, FALSE},	/* Khmr */
      {LTR, NONE, S, FALSE},	/* Laoo */
      {LTR, NONE, S, FALSE},	/* Latn (Latf, Latg) */
      {LTR, NONE, S, FALSE},	/* Mlym */
      {WEAK,TTB,  W, FALSE},	/* Mong */
      {LTR, NONE, S, FALSE},	/* Mymr */
      {LTR, BTT,  W, FALSE},	/* Ogam */
      {LTR, NONE, S, FALSE},	/* Ital */
      {LTR, NONE, S, FALSE},	/* Orya */
      {LTR, NONE, S, FALSE},	/* Runr */
      {LTR, NONE, S, FALSE},	/* Sinh */
      {RTL, NONE, S, FALSE},	/* Syrc (Syrj, Syrn, Syre) */
      {LTR, NONE, S, FALSE},	/* Taml */
      {LTR, NONE, S, FALSE},	/* Telu */
      {RTL, NONE, S, FALSE},	/* Thaa */
      {LTR, NONE, S, FALSE},	/* Thai */
      {LTR, NONE, S, FALSE},	/* Tibt */
      {LTR, NONE, S, FALSE},	/* Cans */
      {LTR, TTB,  S, TRUE },	/* Yiii */
      {LTR, NONE, S, FALSE},	/* Tglg */
      {LTR, NONE, S, FALSE},	/* Hano */
      {LTR, NONE, S, FALSE},	/* Buhd */
      {LTR, NONE, S, FALSE},	/* Tagb */

      /* Unicode-4.0 additions */
      {LTR, NONE, S, FALSE},	/* Brai */
      {RTL, NONE, S, FALSE},	/* Cprt */
      {LTR, NONE, S, FALSE},	/* Limb */
      {LTR, NONE, S, FALSE},	/* Osma */
      {LTR, NONE, S, FALSE},	/* Shaw */
      {LTR, NONE, S, FALSE},	/* Linb */
      {LTR, NONE, S, FALSE},	/* Tale */
      {LTR, NONE, S, FALSE},	/* Ugar */

      /* Unicode-4.1 additions */
      {LTR, NONE, S, FALSE},	/* Talu */
      {LTR, NONE, S, FALSE},	/* Bugi */
      {LTR, NONE, S, FALSE},	/* Glag */
      {LTR, NONE, S, FALSE},	/* Tfng */
      {LTR, NONE, S, FALSE},	/* Sylo */
      {LTR, NONE, S, FALSE},	/* Xpeo */
      {LTR, NONE, S, FALSE},	/* Khar */

      /* Unicode-5.0 additions */
      {LTR, NONE, S, FALSE},	/* Zzzz */
      {LTR, NONE, S, FALSE},	/* Bali */
      {LTR, NONE, S, FALSE},	/* Xsux */
      {RTL, NONE, S, FALSE},	/* Phnx */
      {LTR, NONE, S, FALSE},	/* Phag */
      {RTL, NONE, S, FALSE}	/* Nkoo */
};

#undef NONE
#undef TTB
#undef BTT

#undef LTR
#undef RTL
#undef WEAK

#undef S
#undef E
#undef N
#undef W

static PangoScriptProperties
get_script_properties (PangoScript script)
{
  g_return_val_if_fail (script >= 0, script_properties[0]);

  if ((guint)script >= G_N_ELEMENTS (script_properties))
    return script_properties[0];

  return script_properties[script];
}

/**
 * pango_gravity_get_for_script:
 * @script: #PangoScript to query
 * @base_gravity: base gravity of the paragraph
 * @hint: orientation hint
 *
 * Based on the script, base gravity, and hint, returns actual gravity
 * to use in laying out a single #PangoItem.
 *
 * If @base_gravity is %PANGO_GRAVITY_AUTO, it is first replaced with the
 * preferred gravity of @script.  To get the preferred gravity of a script,
 * pass %PANGO_GRAVITY_AUTO and %PANGO_GRAVITY_HINT_STRONG in.
 *
 * Return value: resolved gravity suitable to use for a run of text
 * with @script.
 *
 * Since: 1.16
 */
PangoGravity
pango_gravity_get_for_script (PangoScript      script,
			      PangoGravity     base_gravity,
			      PangoGravityHint hint)
{
  PangoScriptProperties props = get_script_properties (script);

  if (G_UNLIKELY (base_gravity == PANGO_GRAVITY_AUTO))
    base_gravity = props.preferred_gravity;

  return pango_gravity_get_for_script_and_width (script, props.wide,
						 base_gravity, hint);
}

/**
 * pango_gravity_get_for_script_and_width:
 * @script: #PangoScript to query
 * @wide: %TRUE for wide characters as returned by g_unichar_iswide()
 * @base_gravity: base gravity of the paragraph
 * @hint: orientation hint
 *
 * Based on the script, East Asian width, base gravity, and hint,
 * returns actual gravity to use in laying out a single character
 * or #PangoItem.
 *
 * This function is similar to pango_gravity_get_for_script() except
 * that this function makes a distinction between narrow/half-width and
 * wide/full-width characters also.  Wide/full-width characters always
 * stand *upright*, that is, they always take the base gravity,
 * whereas narrow/full-width characters are always rotated in vertical
 * context.
 *
 * If @base_gravity is %PANGO_GRAVITY_AUTO, it is first replaced with the
 * preferred gravity of @script.
 *
 * Return value: resolved gravity suitable to use for a run of text
 * with @script and @wide.
 *
 * Since: 1.26
 */
PangoGravity
pango_gravity_get_for_script_and_width (PangoScript        script,
					gboolean           wide,
					PangoGravity       base_gravity,
					PangoGravityHint   hint)
{
  PangoScriptProperties props = get_script_properties (script);
  gboolean vertical;


  if (G_UNLIKELY (base_gravity == PANGO_GRAVITY_AUTO))
    base_gravity = props.preferred_gravity;

  vertical = PANGO_GRAVITY_IS_VERTICAL (base_gravity);

  /* Everything is designed such that a system with no vertical support
   * renders everything correctly horizontally.  So, if not in a vertical
   * gravity, base and resolved gravities are always the same.
   *
   * Wide characters are always upright.
   */
  if (G_LIKELY (!vertical || wide))
    return base_gravity;

  /* If here, we have a narrow character in a vertical gravity setting.
   * Resolve depending on the hint.
   */
  switch (hint)
    {
    default:
    case PANGO_GRAVITY_HINT_NATURAL:
      if (props.vert_dir == PANGO_VERTICAL_DIRECTION_NONE)
	return PANGO_GRAVITY_SOUTH;
      if ((base_gravity   == PANGO_GRAVITY_EAST) ^
	  (props.vert_dir == PANGO_VERTICAL_DIRECTION_BTT))
	return PANGO_GRAVITY_SOUTH;
      else
	return PANGO_GRAVITY_NORTH;

    case PANGO_GRAVITY_HINT_STRONG:
      return base_gravity;

    case PANGO_GRAVITY_HINT_LINE:
      if ((base_gravity    == PANGO_GRAVITY_EAST) ^
	  (props.horiz_dir == PANGO_DIRECTION_RTL))
	return PANGO_GRAVITY_SOUTH;
      else
	return PANGO_GRAVITY_NORTH;
    }
}
