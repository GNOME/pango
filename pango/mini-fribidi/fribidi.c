/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001,2002 Behdad Esfahbod.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public  
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License  
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA
 * 
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and
 * <fwpg@sharif.edu>.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>

#include "fribidi.h"

/* Redefine FRIBIDI_CHUNK_SIZE in config.h to override this. */
#ifndef FRIBIDI_CHUNK_SIZE
#ifdef MEM_OPTIMIZED
#define FRIBIDI_CHUNK_SIZE 16
#else
#define FRIBIDI_CHUNK_SIZE 128
#endif
#endif

#define DBG(s)
#define DBG2(s, t)

/*======================================================================
 * Typedef for the run-length list.
 *----------------------------------------------------------------------*/
typedef struct _TypeLink TypeLink;

struct _TypeLink
{
  TypeLink *prev;
  TypeLink *next;

  FriBidiCharType type;
  FriBidiStrIndex pos, len;
  FriBidiLevel level;
};

#define FRIBIDI_LEVEL_START   -1
#define FRIBIDI_LEVEL_END     -1
#define FRIBIDI_LEVEL_REMOVED -2

typedef struct
{
  FriBidiCharType override;	/* only L, R and N are valid */
  FriBidiLevel level;
}
LevelInfo;

static TypeLink *
new_type_link (void)
{
  TypeLink *link;

  link = g_slice_new0 (TypeLink);

  return link;
}

static void
free_type_link (TypeLink *link)
{
  g_slice_free (TypeLink, link);
}

#define FRIBIDI_ADD_TYPE_LINK(p,q) \
	do {	\
		(p)->len = (q)->pos - (p)->pos;	\
		(p)->next = (q);	\
		(q)->prev = (p);	\
		(p) = (q);	\
	} while (0)

static TypeLink *
run_length_encode_types_utf8 (const char *s,
			      int bytelen,
			      FriBidiStrIndex *len,
			      FriBidiCharType *pored_types,
			      FriBidiCharType *panded_strongs)
{
  TypeLink *list, *last, *link;
  FriBidiCharType char_type;
  FriBidiCharType ored_types = 0;
  FriBidiCharType anded_strongs = FRIBIDI_TYPE_RLE;
  FriBidiStrIndex i;
  const char *p;

  /* Add the starting link */
  list = new_type_link ();
  list->type = FRIBIDI_TYPE_SOT;
  list->level = FRIBIDI_LEVEL_START;
  last = list;

  /* Sweep over the string s */
  i = 0;
  for (p = s; p < s + bytelen; p = g_utf8_next_char(p)) {
    char_type = fribidi_get_type (g_utf8_get_char (p));
    ored_types |= char_type;
    if (FRIBIDI_IS_STRONG (char_type))
      anded_strongs &= char_type;
    if (char_type != last->type)
      {
	link = new_type_link ();
	link->type = char_type;
	link->pos = i;
	FRIBIDI_ADD_TYPE_LINK (last, link);
      }
    i++;
  }

  /* Add the ending link */
  link = new_type_link ();
  link->type = FRIBIDI_TYPE_EOT;
  link->level = FRIBIDI_LEVEL_END;
  link->pos = i;
  FRIBIDI_ADD_TYPE_LINK (last, link);

  if (len)
    *len = i;

  if (pored_types)
    *pored_types = ored_types;
  if (panded_strongs)
    *panded_strongs = anded_strongs; 

  return list;
}

/* explicits_list is a list like type_rl_list, that holds the explicit
   codes that are removed from rl_list, to reinsert them later by calling
   the override_list.
*/
static void
init_list (TypeLink **start,
	   TypeLink **end)
{
  TypeLink *list;
  TypeLink *link;

  /* Add the starting link */
  list = new_type_link ();
  list->type = FRIBIDI_TYPE_SOT;
  list->level = FRIBIDI_LEVEL_START;
  list->len = 0;
  list->pos = 0;

  /* Add the ending link */
  link = new_type_link ();
  link->type = FRIBIDI_TYPE_EOT;
  link->level = FRIBIDI_LEVEL_END;
  link->len = 0;
  link->pos = 0;
  list->next = link;
  link->prev = list;

  *start = list;
  *end = link;
}

/* move an element before another element in a list, the list must have a
   previous element, used to update explicits_list.
   assuming that p have both prev and next or none of them, also update
   the list that p is currently in, if any.
*/
static void
move_element_before (TypeLink *p,
		     TypeLink *list)
{
  if (p->prev)
    {
      p->prev->next = p->next;
      p->next->prev = p->prev;
    }
  p->prev = list->prev;
  list->prev->next = p;
  p->next = list;
  list->prev = p;
}

/* override the rl_list 'base', with the elements in the list 'over', to
   reinsert the previously-removed explicit codes (at X9) from
   'explicits_list' back into 'type_rl_list'. This is used at the end of I2
   to restore the explicit marks, and also to reset the character types of
   characters at L1.

   it is assumed that the 'pos' of the first element in 'base' list is not
   more than the 'pos' of the first element of the 'over' list, and the
   'pos' of the last element of the 'base' list is not less than the 'pos'
   of the last element of the 'over' list. these two conditions are always
   satisfied for the two usages mentioned above.

   TBD: use some explanatory names instead of p, q, ...
*/
static void
override_list (TypeLink *base,
	       TypeLink *over)
{
  TypeLink *p = base, *q, *r, *s, *t;
  FriBidiStrIndex pos = 0, pos2;

  if (!over)
    return;
  q = over;
  while (q)
    {
      if (!q->len || q->pos < pos)
	{
	  t = q;
	  q = q->next;
	  free_type_link (t);
	  continue;
	}
      pos = q->pos;
      while (p->next && p->next->pos <= pos)
	p = p->next;
      /* now p is the element that q must be inserted 'in'. */
      pos2 = pos + q->len;
      r = p;
      while (r->next && r->next->pos < pos2)
	r = r->next;
      /* now r is the last element that q affects. */
      if (p == r)
	{
	  /* split p into at most 3 interval, and insert q in the place of
	     the second interval, set r to be the third part. */
	  /* third part needed? */
	  if (p->next && p->next->pos == pos2)
	    r = r->next;
	  else
	    {
	      r = new_type_link ();
	      *r = *p;
	      if (r->next)
		{
		  r->next->prev = r;
		  r->len = r->next->pos - pos2;
		}
	      else
		r->len -= pos - p->pos;
	      r->pos = pos2;
	    }
	  /* first part needed? */
	  if (p->prev && p->pos == pos)
	    {
	      t = p;
	      p = p->prev;
	      free_type_link (t);
	    }
	  else
	    p->len = pos - p->pos;
	}
      else
	{
	  /* cut the end of p. */
	  p->len = pos - p->pos;
	  /* if all of p is cut, remove it. */
	  if (!p->len && p->prev)
	    p = p->prev;

	  /* cut the begining of r. */
	  r->pos = pos2;
	  if (r->next)
	    r->len = r->next->pos - pos2;
	  /* if all of r is cut, remove it. */
	  if (!r->len && r->next)
	    r = r->next;

	  /* remove the elements between p and r. */
	  for (s = p->next; s != r;)
	    {
	      t = s;
	      s = s->next;
	      free_type_link (t);
	    }
	}
      /* before updating the next and prev links to point to the inserted q,
         we must remember the next element of q in the 'over' list.
       */
      t = q;
      q = q->next;
      p->next = t;
      t->prev = p;
      t->next = r;
      r->prev = t;
    }
}

/* Some convenience macros */
#define RL_TYPE(list) ((list)->type)
#define RL_LEN(list) ((list)->len)
#define RL_POS(list) ((list)->pos)
#define RL_LEVEL(list) ((list)->level)

static TypeLink *
merge_with_prev (TypeLink *second)
{
  TypeLink *first = second->prev;
  first->next = second->next;
  first->next->prev = first;
  RL_LEN (first) += RL_LEN (second);
  free_type_link (second);
  return first;
}

static void
compact_list (TypeLink *list)
{
  if (list->next)
    for (list = list->next; list; list = list->next)
      if (RL_TYPE (list->prev) == RL_TYPE (list)
	  && RL_LEVEL (list->prev) == RL_LEVEL (list))
	list = merge_with_prev (list);
}

static void
compact_neutrals (TypeLink *list)
{
  if (list->next)
    {
      for (list = list->next; list; list = list->next)
	{
	  if (RL_LEVEL (list->prev) == RL_LEVEL (list)
	      &&
	      ((RL_TYPE
		(list->prev) == RL_TYPE (list)
		|| (FRIBIDI_IS_NEUTRAL (RL_TYPE (list->prev))
		    && FRIBIDI_IS_NEUTRAL (RL_TYPE (list))))))
	    list = merge_with_prev (list);
	}
    }
}

/*======================================================================
 *  Frees up the rl_list, must be called after each call to
 *  fribidi_analyse_string(), after the list is not needed anymore.
 *----------------------------------------------------------------------*/
static void
free_rl_list (TypeLink *type_rl_list)
{
  DBG ("Entering free_rl_list()\n");

  if (!type_rl_list)
    {
      DBG ("Leaving free_rl_list()\n");
      return;
    }

  g_slice_free_chain (TypeLink, type_rl_list, next);

  DBG ("Leaving free_rl_list()\n");
  return;
}

/*=========================================================================
 * define macros for push and pop the status in to / out of the stack
 *-------------------------------------------------------------------------*/

/* There's some little points in pushing and poping into the status stack:
   1. when the embedding level is not valid (more than UNI_MAX_BIDI_LEVEL=61),
   you must reject it, and not to push into the stack, but when you see a
   PDF, you must find the matching code, and if it was pushed in the stack,
   pop it, it means you must pop if and only if you have pushed the
   matching code, the over_pushed var counts the number of rejected codes yet.
   2. there's a more confusing point too, when the embedding level is exactly
   UNI_MAX_BIDI_LEVEL-1=60, an LRO or LRE must be rejected because the new
   level would be UNI_MAX_BIDI_LEVEL+1=62, that is invalid, but an RLO or RLE
   must be accepted because the new level is UNI_MAX_BIDI_LEVEL=61, that is
   valid, so the rejected codes may be not continuous in the logical order,
   in fact there is at most two continuous intervals of codes, with a RLO or
   RLE between them.  To support this case, the first_interval var counts the
   number of rejected codes in the first interval, when it is 0, means that
   there is only one interval yet.
*/

/* a. If this new level would be valid, then this embedding code is valid.
   Remember (push) the current embedding level and override status.
   Reset current level to this new level, and reset the override status to
   new_override.
   b. If the new level would not be valid, then this code is invalid. Don't
   change the current level or override status.
*/
#define PUSH_STATUS \
    do { \
      if (new_level <= UNI_MAX_BIDI_LEVEL) \
        { \
          if (level == UNI_MAX_BIDI_LEVEL - 1) \
            first_interval = over_pushed; \
          status_stack[stack_size].level = level; \
          status_stack[stack_size].override = override; \
          stack_size++; \
          level = new_level; \
          override = new_override; \
        } else \
	  over_pushed++; \
    } while (0)

/* If there was a valid matching code, restore (pop) the last remembered
   (pushed) embedding level and directional override.
*/
#define POP_STATUS \
    do { \
      if (over_pushed || stack_size) \
      { \
        if (over_pushed > first_interval) \
          over_pushed--; \
        else \
          { \
            if (over_pushed == first_interval) \
              first_interval = 0; \
            stack_size--; \
            level = status_stack[stack_size].level; \
            override = status_stack[stack_size].override; \
          } \
      } \
    } while (0)

/*==========================================================================
 * There was no support for sor and eor in the absence of Explicit Embedding
 * Levels, so define macros, to support them, with as less change as needed.
 *--------------------------------------------------------------------------*/

/* Return the type of previous char or the sor, if already at the start of
   a run level. */
#define PREV_TYPE_OR_SOR(pp) \
    ( \
     RL_LEVEL(pp->prev) == RL_LEVEL(pp) ? \
      RL_TYPE(pp->prev) : \
      FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(pp->prev), RL_LEVEL(pp))) \
    )

/* Return the type of next char or the eor, if already at the end of
   a run level. */
#define NEXT_TYPE_OR_EOR(pp) \
    ( \
     !pp->next ? \
      FRIBIDI_LEVEL_TO_DIR(RL_LEVEL(pp)) : \
      (RL_LEVEL(pp->next) == RL_LEVEL(pp) ? \
        RL_TYPE(pp->next) : \
        FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(pp->next), RL_LEVEL(pp))) \
      ) \
    )


/* Return the embedding direction of a link. */
#define FRIBIDI_EMBEDDING_DIRECTION(list) \
    FRIBIDI_LEVEL_TO_DIR(RL_LEVEL(list))


/*======================================================================
 *  This function should follow the Unicode specification closely!
 *----------------------------------------------------------------------*/
static fribidi_boolean
fribidi_analyse_string_utf8 (	/* input */
			 const char *str,
			 int bytelen,
			 FriBidiCharType *pbase_dir,
			 /* output */
			 FriBidiStrIndex *len,
			 TypeLink **ptype_rl_list,
			 FriBidiLevel *pmax_level)
{
  FriBidiLevel base_level, max_level;
  FriBidiCharType base_dir;
  TypeLink *type_rl_list, *explicits_list, *explicits_list_end, *pp;

  DBG ("Entering fribidi_analyse_string()\n");

  /* Determinate character types */
  DBG ("  Determine character types\n");
  {
    FriBidiCharType ored_types;
    FriBidiCharType anded_strongs;

    /* Run length encode the character types */
    type_rl_list = run_length_encode_types_utf8 (str, bytelen, len,
		   				 &ored_types, &anded_strongs);

    /* The case that all resolved levels will be ltr.
     * First, all strongs should be ltr, and one of the following:
     *
     *	o *pbase_dir doesn't have an rtl taste.
     *	o there are letters, and *pbase_dir is weak.
     */
    if (!FRIBIDI_IS_RTL (ored_types) &&
	     (!FRIBIDI_IS_RTL (*pbase_dir) ||
	      (FRIBIDI_IS_WEAK (*pbase_dir) && FRIBIDI_IS_LETTER (ored_types))
	     ))
      {
	/* all ltr */
	free_rl_list (type_rl_list);

	*ptype_rl_list = NULL;
	*pmax_level = 0;
	*pbase_dir = FRIBIDI_TYPE_LTR;

	return 0;
      }
    /* The case that all resolved levels will be rtl is much more complex.
     * First, there should be no numbers, all strongs be rtl, and one of
     * the following:
     *
     *	o *pbase_dir has an rtl taste (may be weak).
     *	o there are letters, and *pbase_dir is weak.
     */
    else if (!FRIBIDI_IS_NUMBER (ored_types) && FRIBIDI_IS_RTL (anded_strongs) &&
	     (FRIBIDI_IS_RTL (*pbase_dir) ||
	      (FRIBIDI_IS_WEAK (*pbase_dir) && FRIBIDI_IS_LETTER (ored_types))
	     ))
      {
	free_rl_list (type_rl_list);

	*ptype_rl_list = NULL;
	*pmax_level = 1;
	*pbase_dir = FRIBIDI_TYPE_RTL;

	return 0;
      }
  }
  DBG ("  Determine character types, Done\n");

  init_list (&explicits_list, &explicits_list_end);

  /* Find base level */
  DBG ("  Finding the base level\n");
  if (FRIBIDI_IS_STRONG (*pbase_dir))
    base_level = FRIBIDI_DIR_TO_LEVEL (*pbase_dir);
  /* P2. P3. Search for first strong character and use its direction as
     base direction */
  else
    {
      /* If no strong base_dir was found, resort to the weak direction
         that was passed on input. */
      base_level = FRIBIDI_DIR_TO_LEVEL (*pbase_dir);
      base_dir = FRIBIDI_TYPE_ON;
      for (pp = type_rl_list; pp; pp = pp->next)
	if (FRIBIDI_IS_LETTER (RL_TYPE (pp)))
	  {
	    base_level = FRIBIDI_DIR_TO_LEVEL (RL_TYPE (pp));
	    base_dir = FRIBIDI_LEVEL_TO_DIR (base_level);
	    break;
	  }
    }
  base_dir = FRIBIDI_LEVEL_TO_DIR (base_level);
  DBG2 ("  Base level : %c\n", fribidi_char_from_level (base_level));
  DBG2 ("  Base dir   : %c\n", fribidi_char_from_type (base_dir));
  DBG ("  Finding the base level, Done\n");

  /* Explicit Levels and Directions */
  DBG ("Explicit Levels and Directions\n");
  {
    /* X1. Begin by setting the current embedding level to the paragraph
       embedding level. Set the directional override status to neutral.
       Process each character iteratively, applying rules X2 through X9.
       Only embedding levels from 0 to 61 are valid in this phase. */
    FriBidiLevel level, new_level;
    FriBidiCharType override, new_override;
    FriBidiStrIndex i;
    int stack_size, over_pushed, first_interval;
    LevelInfo *status_stack;
    TypeLink temp_link;

    level = base_level;
    override = FRIBIDI_TYPE_ON;
    /* stack */
    stack_size = 0;
    over_pushed = 0;
    first_interval = 0;
    status_stack =
      (LevelInfo *) malloc (sizeof (LevelInfo) * (UNI_MAX_BIDI_LEVEL + 2));

    for (pp = type_rl_list->next; pp->next; pp = pp->next)
      {
	FriBidiCharType this_type = RL_TYPE (pp);
	if (FRIBIDI_IS_EXPLICIT_OR_BN (this_type))
	  {
	    if (FRIBIDI_IS_STRONG (this_type))
	      {			/* LRE, RLE, LRO, RLO */
		/* 1. Explicit Embeddings */
		/*   X2. With each RLE, compute the least greater odd embedding level. */
		/*   X3. With each LRE, compute the least greater even embedding level. */
		/* 2. Explicit Overrides */
		/*   X4. With each RLO, compute the least greater odd embedding level. */
		/*   X5. With each LRO, compute the least greater even embedding level. */
		new_override = FRIBIDI_EXPLICIT_TO_OVERRIDE_DIR (this_type);
		for (i = 0; i < RL_LEN (pp); i++)
		  {
		    new_level =
		      ((level + FRIBIDI_DIR_TO_LEVEL (this_type) + 2) & ~1) -
		      FRIBIDI_DIR_TO_LEVEL (this_type);
		    PUSH_STATUS;
		  }
	      }
	    else if (this_type == FRIBIDI_TYPE_PDF)
	      {
		/* 3. Terminating Embeddings and overrides */
		/*   X7. With each PDF, determine the matching embedding or
		   override code. */
		for (i = 0; i < RL_LEN (pp); i++)
		  POP_STATUS;
	      }
	    /* X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN codes. */
	    /* Remove element and add it to explicits_list */
	    temp_link.next = pp->next;
	    pp->level = FRIBIDI_LEVEL_REMOVED;
	    move_element_before (pp, explicits_list_end);
	    pp = &temp_link;
	  }
	else
	  {
	    /* X6. For all typed besides RLE, LRE, RLO, LRO, and PDF:
	       a. Set the level of the current character to the current
	       embedding level.
	       b. Whenever the directional override status is not neutral,
	       reset the current character type to the directional override
	       status. */
	    RL_LEVEL (pp) = level;
	    if (!FRIBIDI_IS_NEUTRAL (override))
	      RL_TYPE (pp) = override;
	  }
	/* X8. All explicit directional embeddings and overrides are
	   completely terminated at the end of each paragraph. Paragraph
	   separators are not included in the embedding. */
	/* This function is running on a single paragraph, so we can do
	   X8 after all the input is processed. */
      }

    /* Implementing X8. It has no effect on a single paragraph! */
    level = base_level;
    override = FRIBIDI_TYPE_ON;
    stack_size = 0;
    over_pushed = 0;

    free (status_stack);
  }
  /* X10. The remaining rules are applied to each run of characters at the
     same level. For each run, determine the start-of-level-run (sor) and
     end-of-level-run (eor) type, either L or R. This depends on the
     higher of the two levels on either side of the boundary (at the start
     or end of the paragraph, the level of the 'other' run is the base
     embedding level). If the higher level is odd, the type is R, otherwise
     it is L. */
  /* Resolving Implicit Levels can be done out of X10 loop, so only change
     of Resolving Weak Types and Resolving Neutral Types is needed. */

  compact_list (type_rl_list);

  /* 4. Resolving weak types */
  DBG ("Resolving weak types\n");
  {
    FriBidiCharType last_strong, prev_type_org;
    fribidi_boolean w4;

    last_strong = base_dir;

    for (pp = type_rl_list->next; pp->next; pp = pp->next)
      {
	FriBidiCharType prev_type, this_type, next_type;

	prev_type = PREV_TYPE_OR_SOR (pp);
	this_type = RL_TYPE (pp);
	next_type = NEXT_TYPE_OR_EOR (pp);

	if (FRIBIDI_IS_STRONG (prev_type))
	  last_strong = prev_type;

	/* W1. NSM
	   Examine each non-spacing mark (NSM) in the level run, and change the
	   type of the NSM to the type of the previous character. If the NSM
	   is at the start of the level run, it will get the type of sor. */
	/* Implementation note: it is important that if the previous character
	   is not sor, then we should merge this run with the previous,
	   because of rules like W5, that we assume all of a sequence of
	   adjacent ETs are in one TypeLink. */
	if (this_type == FRIBIDI_TYPE_NSM)
	  {
	    if (RL_LEVEL (pp->prev) == RL_LEVEL (pp))
	      pp = merge_with_prev (pp);
	    else
	      RL_TYPE (pp) = prev_type;
	    continue;		/* As we know the next condition cannot be true. */
	  }

	/* W2: European numbers. */
	if (this_type == FRIBIDI_TYPE_EN && last_strong == FRIBIDI_TYPE_AL)
	  {
	    RL_TYPE (pp) = FRIBIDI_TYPE_AN;

	    /* Resolving dependency of loops for rules W1 and W2, so we
	       can merge them in one loop. */
	    if (next_type == FRIBIDI_TYPE_NSM)
	      RL_TYPE (pp->next) = FRIBIDI_TYPE_AN;
	  }
      }


    last_strong = base_dir;
    /* Resolving dependency of loops for rules W4 and W5, W5 may
       want to prevent W4 to take effect in the next turn, do this 
       through "w4". */
    w4 = FRIBIDI_TRUE;
    /* Resolving dependency of loops for rules W4 and W5 with W7,
       W7 may change an EN to L but it sets the prev_type_org if needed,
       so W4 and W5 in next turn can still do their works. */
    prev_type_org = FRIBIDI_TYPE_ON;

    for (pp = type_rl_list->next; pp->next; pp = pp->next)
      {
	FriBidiCharType prev_type, this_type, next_type;

	prev_type = PREV_TYPE_OR_SOR (pp);
	this_type = RL_TYPE (pp);
	next_type = NEXT_TYPE_OR_EOR (pp);

	if (FRIBIDI_IS_STRONG (prev_type))
	  last_strong = prev_type;

	/* W3: Change ALs to R. */
	if (this_type == FRIBIDI_TYPE_AL)
	  {
	    RL_TYPE (pp) = FRIBIDI_TYPE_RTL;
	    w4 = FRIBIDI_TRUE;
	    prev_type_org = FRIBIDI_TYPE_ON;
	    continue;
	  }

	/* W4. A single european separator changes to a european number.
	   A single common separator between two numbers of the same type
	   changes to that type. */
	if (w4
	    && RL_LEN (pp) == 1 && FRIBIDI_IS_ES_OR_CS (this_type)
	    && FRIBIDI_IS_NUMBER (prev_type_org) && prev_type_org == next_type
	    && (prev_type_org == FRIBIDI_TYPE_EN
		|| this_type == FRIBIDI_TYPE_CS))
	  {
	    RL_TYPE (pp) = prev_type;
	    this_type = RL_TYPE (pp);
	  }
	w4 = FRIBIDI_TRUE;

	/* W5. A sequence of European terminators adjacent to European
	   numbers changes to All European numbers. */
	if (this_type == FRIBIDI_TYPE_ET
	    && (prev_type_org == FRIBIDI_TYPE_EN
		|| next_type == FRIBIDI_TYPE_EN))
	  {
	    RL_TYPE (pp) = FRIBIDI_TYPE_EN;
	    w4 = FRIBIDI_FALSE;
	    this_type = RL_TYPE (pp);
	  }

	/* W6. Otherwise change separators and terminators to other neutral. */
	if (FRIBIDI_IS_NUMBER_SEPARATOR_OR_TERMINATOR (this_type))
	  RL_TYPE (pp) = FRIBIDI_TYPE_ON;

	/* W7. Change european numbers to L. */
	if (this_type == FRIBIDI_TYPE_EN && last_strong == FRIBIDI_TYPE_LTR)
	  {
	    RL_TYPE (pp) = FRIBIDI_TYPE_LTR;
	    prev_type_org = (RL_LEVEL (pp) == RL_LEVEL (pp->next) ?
			     FRIBIDI_TYPE_EN : FRIBIDI_TYPE_ON);
	  }
	else
	  prev_type_org = PREV_TYPE_OR_SOR (pp->next);
      }
  }

  compact_neutrals (type_rl_list);

  /* 5. Resolving Neutral Types */
  DBG ("Resolving neutral types\n");
  {
    /* N1. and N2.
       For each neutral, resolve it. */
    for (pp = type_rl_list->next; pp->next; pp = pp->next)
      {
	FriBidiCharType prev_type, this_type, next_type;

	/* "European and arabic numbers are treated as though they were R"
	   FRIBIDI_CHANGE_NUMBER_TO_RTL does this. */
	this_type = FRIBIDI_CHANGE_NUMBER_TO_RTL (RL_TYPE (pp));
	prev_type = FRIBIDI_CHANGE_NUMBER_TO_RTL (PREV_TYPE_OR_SOR (pp));
	next_type = FRIBIDI_CHANGE_NUMBER_TO_RTL (NEXT_TYPE_OR_EOR (pp));

	if (FRIBIDI_IS_NEUTRAL (this_type))
	  RL_TYPE (pp) = (prev_type == next_type) ?
	    /* N1. */ prev_type :
	    /* N2. */ FRIBIDI_EMBEDDING_DIRECTION (pp);
      }
  }

  compact_list (type_rl_list);

  /* 6. Resolving implicit levels */
  DBG ("Resolving implicit levels\n");
  {
    max_level = base_level;

    for (pp = type_rl_list->next; pp->next; pp = pp->next)
      {
	FriBidiCharType this_type;
	int level;

	this_type = RL_TYPE (pp);
	level = RL_LEVEL (pp);

	/* I1. Even */
	/* I2. Odd */
	if (FRIBIDI_IS_NUMBER (this_type))
	  RL_LEVEL (pp) = (level + 2) & ~1;
	else
	  RL_LEVEL (pp) = (level ^ FRIBIDI_DIR_TO_LEVEL (this_type)) +
	    (level & 1);

	if (RL_LEVEL (pp) > max_level)
	  max_level = RL_LEVEL (pp);
      }
  }

  compact_list (type_rl_list);

/* Reinsert the explicit codes & bn's that already removed, from the
   explicits_list to type_rl_list. */
  DBG ("Reinserting explicit codes\n");
  {
    TypeLink *p;

    override_list (type_rl_list, explicits_list);
    p = type_rl_list->next;
    if (p->level < 0)
      p->level = base_level;
    for (; p->next; p = p->next)
      if (p->level < 0)
	p->level = p->prev->level;
  }

  DBG ("Reset the embedding levels\n");
  {
    int j, k, state, pos;
    TypeLink *p, *q, *list, *list_end;

    const char *strp = str + bytelen;

    /* L1. Reset the embedding levels of some chars. */
    init_list (&list, &list_end);
    q = list_end;
    state = 1;
    pos = *len - 1;
    for (j = *len - 1; j >= -1; j--)
      {
	/* if state is on at the very first of string, do this too. */
	if (j >= 0)
	  k = fribidi_get_type (g_utf8_get_char (strp = g_utf8_prev_char (strp)));
	else
	  k = FRIBIDI_TYPE_ON;
	if (!state && FRIBIDI_IS_SEPARATOR (k))
	  {
	    state = 1;
	    pos = j;
	  }
	else if (state && !FRIBIDI_IS_EXPLICIT_OR_SEPARATOR_OR_BN_OR_WS (k))
	  {
	    state = 0;
	    p = new_type_link ();
	    p->prev = p->next = NULL;
	    p->pos = j + 1;
	    p->len = pos - j;
	    p->type = base_dir;
	    p->level = base_level;
	    move_element_before (p, q);
	    q = p;
	  }
      }
    override_list (type_rl_list, list);
  }

  *ptype_rl_list = type_rl_list;
  *pmax_level = max_level;
  *pbase_dir = base_dir;

  DBG ("Leaving fribidi_analyse_string()\n");
  return 1;
}

/*======================================================================
 *  fribidi_log2vis_get_embedding_levels() is used in order to just get
 *  the embedding levels.
 *----------------------------------------------------------------------*/
FRIBIDI_API FriBidiLevel *
fribidi_log2vis_get_embedding_levels_new_utf8 (	/* input */
				       const char *str,
				       int bytelen,
				       FriBidiCharType *pbase_dir)
{
  TypeLink *type_rl_list, *pp;
  FriBidiLevel max_level, *embedding_level_list;
  FriBidiStrIndex len;

  DBG ("Entering fribidi_log2vis_get_embedding_levels()\n");

  if (bytelen == 0)
    {
      DBG ("Leaving fribidi_log2vis_get_embedding_levels()\n");
      return NULL;
    }

  if (!fribidi_analyse_string_utf8 (str, bytelen, pbase_dir,
			  /* output */
			  &len, &type_rl_list, &max_level))
    {
     /* unidirectional.  return all-zero or all-one embedding levels */

     if (max_level)
       {
         embedding_level_list = g_new (FriBidiLevel, len);
	 /* assumes sizeof(FriBidiLevel) == 1, which is true! */
	 memset (embedding_level_list, max_level, len);
	 return embedding_level_list;
       }
     else
       {
         return g_new0 (FriBidiLevel, len);
       }
    }

  embedding_level_list = g_new (FriBidiLevel, len);
  for (pp = type_rl_list->next; pp->next; pp = pp->next)
    {
      FriBidiStrIndex i, pos = RL_POS (pp), len = RL_LEN (pp);
      FriBidiLevel level = RL_LEVEL (pp);
      for (i = 0; i < len; i++)
	embedding_level_list[pos + i] = level;
    }

  free_rl_list (type_rl_list);

  DBG ("Leaving fribidi_log2vis_get_embedding_levels()\n");
  return embedding_level_list;
}

