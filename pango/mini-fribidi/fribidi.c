/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999 Dov Grobgeld
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
#include <glib.h>
#include "pango/pango-utils.h"
#include "fribidi_types.h"

/*======================================================================
// Typedef for the run-length list.
//----------------------------------------------------------------------*/
typedef struct _TypeLink TypeLink;

struct _TypeLink {
  TypeLink *prev;
  TypeLink *next;
  FriBidiCharType type;
  int pos;
  int len;
  int level;
};

typedef struct {
  FriBidiChar key;
  FriBidiChar value;
} key_value_t;

static TypeLink *free_type_links = NULL;

static TypeLink *new_type_link(void)
{
  TypeLink *link;
  
  if (free_type_links)
    {
      link = free_type_links;
      free_type_links = free_type_links->next;
    }
  else
    {
      static GMemChunk *mem_chunk = NULL;

      if (!mem_chunk)
       mem_chunk = g_mem_chunk_new ("TypeLinkList",
                                    sizeof (TypeLink),
                                    sizeof (TypeLink) * 128,
                                    G_ALLOC_ONLY);

      link = g_chunk_new (TypeLink, mem_chunk);
    }

  link->len = 0;
  link->pos = 0;
  link->level = 0;
  link->next = NULL;
  link->prev = NULL;
  return link;
}

static void free_type_link(TypeLink *link)
{
  link->next = free_type_links;
  free_type_links = link;
}

static TypeLink *run_length_encode_types(int *char_type, int type_len)
{
  TypeLink *list = NULL;
  TypeLink *last;
  TypeLink *link;
  FriBidiCharType type;
  int len, pos, i;

  /* Add the starting link */
  list = new_type_link();
  list->type = FRIBIDI_TYPE_SOT;
  list->len = 0;
  list->pos = 0;
  last = list;

  /* Sweep over the string_types */
  type = -1;
  len = 0;
  pos = -1;
  for (i=0; i<=type_len; i++)
    {
      if (i==type_len || char_type[i] != type)
	{
	  if (pos>=0)
	    {
	      link = new_type_link();
	      link->type = type;
	      link->pos = pos;
	      link->len = len;
             last->next = link;
             link->prev = last;
	      last = last->next;
	    }
	  if (i==type_len)
	    break;
	  len = 0;
	  pos = i;
	}
      type =char_type[i];
      len++;
    }

  /* Add the ending link */
  link = new_type_link();
  link->type = FRIBIDI_TYPE_EOT;
  link->len = 0;
  link->pos = type_len;
  last->next = link;
  link->prev = last;

  return list;
}

/* Some convenience macros */
#define RL_TYPE(list) (list)->type
#define RL_LEN(list) (list)->len
#define RL_POS(list) (list)->pos
#define RL_LEVEL(list) (list)->level

static void compact_list(TypeLink *list)
{
  while(list)
    {
      if (list->prev
	  && RL_TYPE(list->prev) == RL_TYPE(list))
	{
         TypeLink *next = list->next;
	  list->prev->next = list->next;
	  list->next->prev = list->prev;
	  RL_LEN(list->prev) = RL_LEN(list->prev) + RL_LEN(list);
         free_type_link(list);
	  list = next;
      }
      else
	list = list->next;
    }
}

/* Define a rule macro */

/* Rules for overriding current type */
#define TYPE_RULE1(old_this,            \
		   new_this)             \
     if (this_type == TYPE_ ## old_this)      \
         RL_TYPE(pp) =       FRIBIDI_TYPE_ ## new_this; \

/* Rules for current and previous type */
#define TYPE_RULE2(old_prev, old_this,            \
		  new_prev, new_this)             \
     if (    prev_type == FRIBIDI_TYPE_ ## old_prev       \
	  && this_type == FRIBIDI_TYPE_ ## old_this)      \
       {                                          \
	   RL_TYPE(pp->prev) = FRIBIDI_TYPE_ ## new_prev; \
	   RL_TYPE(pp) =       FRIBIDI_TYPE_ ## new_this; \
           continue;                              \
       }

/* A full rule that assigns all three types */
#define TYPE_RULE(old_prev, old_this, old_next,   \
		  new_prev, new_this, new_next)   \
     if (    prev_type == FRIBIDI_TYPE_ ## old_prev       \
	  && this_type == FRIBIDI_TYPE_ ## old_this       \
	  && next_type == FRIBIDI_TYPE_ ## old_next)      \
       {                                          \
	   RL_TYPE(pp->prev) = FRIBIDI_TYPE_ ## new_prev; \
	   RL_TYPE(pp) =       FRIBIDI_TYPE_ ## new_this; \
	   RL_TYPE(pp->next) = FRIBIDI_TYPE_ ## new_next; \
           continue;                              \
       }

/* For optimization the following macro only assigns the center type */
#define TYPE_RULE_C(old_prev, old_this, old_next,   \
		    new_this)   \
     if (    prev_type == FRIBIDI_TYPE_ ## old_prev       \
	  && this_type == FRIBIDI_TYPE_ ## old_this       \
	  && next_type == FRIBIDI_TYPE_ ## old_next)      \
       {                                          \
	   RL_TYPE(pp) =       FRIBIDI_TYPE_ ## new_this; \
           continue;                              \
       }

/*======================================================================
//  This function should follow the Unicode specification closely!
//
//  It is still lacking the support for <RLO> and <LRO>.
//----------------------------------------------------------------------*/
static void
fribidi_analyse_string(/* input */
		       FriBidiChar *str,
		       int len,
		       FriBidiCharType *pbase_dir,
		       /* output */
                      TypeLink **ptype_rl_list,
		       gint *pmax_level)
{
  int base_level, base_dir;
  int max_level;
  int i;
  int *char_type;
  int prev_last_strong, last_strong;
  TypeLink *type_rl_list, *pp;

  /* Determinate character types */
  char_type = g_new(gint, len);
  for (i=0; i<len; i++)
    char_type[i] = _pango_fribidi_get_type (str[i]);

  /* Run length encode the character types */
  type_rl_list = run_length_encode_types(char_type, len);
  g_free(char_type);

  /* Find the base level */
  if (*pbase_dir == FRIBIDI_TYPE_L)
    {
      base_dir = FRIBIDI_TYPE_L;
      base_level = 0;
    }
  else if (*pbase_dir == FRIBIDI_TYPE_R)
    {
      base_dir = FRIBIDI_TYPE_R;
      base_level = 1;
    }

  /* Search for first strong character and use its direction as base
     direciton */
  else
    {
      base_level = 0; /* Default */
      base_dir = FRIBIDI_TYPE_N;
      for (pp = type_rl_list; pp; pp = pp->next)
	{
	  if (RL_TYPE(pp) == FRIBIDI_TYPE_R)
	    {
	      base_level = 1;
	      base_dir = FRIBIDI_TYPE_R;
	      break;
	    }
	  else if (RL_TYPE(pp) == FRIBIDI_TYPE_L)
	    {
	      base_level = 0;
	      base_dir = FRIBIDI_TYPE_L;
	      break;
	    }
	}
    
      /* If no strong base_dir was found, resort to the weak direction
       * that was passed on input.
       */
      if (base_dir == FRIBIDI_TYPE_N)
	{
	  if (*pbase_dir == FRIBIDI_TYPE_WR)
	    {
	      base_dir = FRIBIDI_TYPE_RTL;
	      base_level = 1;
	    }
	  else if (*pbase_dir == FRIBIDI_TYPE_WL)
	    {
	      base_dir = FRIBIDI_TYPE_LTR;
	      base_level = 0;
	    }
	}
    }
  
  /* 1. Explicit Levels and Directions. TBD! */
  compact_list(type_rl_list);
  
  /* 2. Explicit Overrides. TBD! */
  compact_list(type_rl_list);
  
  /* 3. Terminating Embeddings and overrides. TBD! */
  compact_list(type_rl_list);
  
  /* 4. Resolving weak types */
  last_strong = base_dir;
  for (pp = type_rl_list->next; pp->next; pp = pp->next)
    {
      int prev_type = RL_TYPE(pp->prev);
      int this_type = RL_TYPE(pp);
      int next_type = RL_TYPE(pp->next);

      /* Remember the last strong character */
      if (prev_type == FRIBIDI_TYPE_AL
	  || prev_type == FRIBIDI_TYPE_R
	  || prev_type == FRIBIDI_TYPE_L)
	  last_strong = prev_type;
      
      /* W1. NSM */
      if (this_type == FRIBIDI_TYPE_NSM)
	{
	  if (prev_type == FRIBIDI_TYPE_SOT)
	    RL_TYPE(pp) = FRIBIDI_TYPE_N;       /* Will be resolved to base dir */
	  else
	    RL_TYPE(pp) = prev_type;
	}

      /* W2: European numbers */
      if (this_type == FRIBIDI_TYPE_N
	  && last_strong == FRIBIDI_TYPE_AL)
	RL_TYPE(pp) = FRIBIDI_TYPE_AN;

      /* W3: Change ALs to R
	 We have to do this for prev character as we would otherwise
	 interfer with the next last_strong which is FRIBIDI_TYPE_AL.
       */
      if (prev_type == FRIBIDI_TYPE_AL)
	RL_TYPE(pp->prev) = FRIBIDI_TYPE_R;

      /* W4. A single european separator changes to a european number.
	 A single common separator between two numbers of the same type
	 changes to that type.
       */
      if (RL_LEN(pp) == 1) 
	{
	  TYPE_RULE_C(EN,ES,EN,   EN);
	  TYPE_RULE_C(EN,CS,EN,   EN);
	  TYPE_RULE_C(AN,CS,AN,   AN);
	}

      /* W5. A sequence of European terminators adjacent to European
	 numbers changes to All European numbers.
       */
      if (this_type == FRIBIDI_TYPE_ET)
	{
	  if (next_type == FRIBIDI_TYPE_EN
	      || prev_type == FRIBIDI_TYPE_EN) {
	    RL_TYPE(pp) = FRIBIDI_TYPE_EN;
	  }
	}

      /* This type may have been overriden */
      this_type = RL_TYPE(pp);
      
      /* W6. Otherwise change separators and terminators to other neutral */
      if (this_type == FRIBIDI_TYPE_ET
	  || this_type == FRIBIDI_TYPE_CS
	  || this_type == FRIBIDI_TYPE_ES)
	RL_TYPE(pp) = FRIBIDI_TYPE_ON;

      /* W7. Change european numbers to L. */
      if (prev_type == FRIBIDI_TYPE_EN
	  && last_strong == FRIBIDI_TYPE_L)
	RL_TYPE(pp->prev) = FRIBIDI_TYPE_L;
    }

  /* Handle the two rules that effect pp->prev for the last element */

  if (RL_TYPE (pp->prev) == FRIBIDI_TYPE_AL) /* W3 */
    RL_TYPE(pp->prev) = FRIBIDI_TYPE_R;
  if (RL_TYPE (pp->prev) == FRIBIDI_TYPE_EN  /* W7 */
      && last_strong == FRIBIDI_TYPE_L)	     
    RL_TYPE(pp->prev) = FRIBIDI_TYPE_L;
  

  compact_list(type_rl_list);
  
  /* 5. Resolving Neutral Types */

  /* We can now collapse all separators and other neutral types to
     plain neutrals */
  for (pp = type_rl_list->next; pp->next; pp = pp->next)
    {
      int this_type = RL_TYPE(pp);

      if (   this_type == FRIBIDI_TYPE_WS
	  || this_type == FRIBIDI_TYPE_ON
	  || this_type == FRIBIDI_TYPE_ES
	  || this_type == FRIBIDI_TYPE_ET
	  || this_type == FRIBIDI_TYPE_CS
	  || this_type == FRIBIDI_TYPE_BN)
	RL_TYPE(pp) = FRIBIDI_TYPE_N;
    }
    
  compact_list(type_rl_list);
  
  for (pp = type_rl_list->next; pp->next; pp = pp->next)
    {
      int prev_type = RL_TYPE(pp->prev);
      int this_type = RL_TYPE(pp);
      int next_type = RL_TYPE(pp->next);

      if (this_type == FRIBIDI_TYPE_N)   /* optimization! */
	{
	  /* "European and arabic numbers are treated
	     as though they were R" */

	  if (prev_type == FRIBIDI_TYPE_EN || prev_type == FRIBIDI_TYPE_AN)
	    prev_type = FRIBIDI_TYPE_R;

	  if (next_type == FRIBIDI_TYPE_EN || next_type == FRIBIDI_TYPE_AN)
	    next_type = FRIBIDI_TYPE_R;

	  /* N1. */
	  TYPE_RULE_C(R,N,R,   R);
	  TYPE_RULE_C(L,N,L,   L);

	  /* N2. Any remaining neutrals takes the embedding direction */
	  if (RL_TYPE(pp) == FRIBIDI_TYPE_N)
	    RL_TYPE(pp) = FRIBIDI_TYPE_E;
	}
    }

  compact_list(type_rl_list);
  
  /* 6. Resolving Implicit levels */
  {
    int level = base_level;
    max_level = base_level;
    
    for (pp = type_rl_list->next; pp->next; pp = pp->next)
      {
	int this_type = RL_TYPE(pp);

	/* This code should be expanded to handle explicit directions! */

	/* Even */
	if (level % 2 == 0)
	  {
	    if (this_type == FRIBIDI_TYPE_R)
	      RL_LEVEL(pp) = level + 1;
	    else if (this_type == FRIBIDI_TYPE_AN)
	      RL_LEVEL(pp) = level + 2;
	    else if (RL_TYPE(pp->prev) != FRIBIDI_TYPE_L && this_type == FRIBIDI_TYPE_EN)
	      RL_LEVEL(pp) = level + 2;
	    else
	      RL_LEVEL(pp) = level;
	  }
	/* Odd */
	else
	  {
	    if (   this_type == FRIBIDI_TYPE_L
		|| this_type == FRIBIDI_TYPE_AN
		|| this_type == FRIBIDI_TYPE_EN)
	      RL_LEVEL(pp) = level+1;
	    else
	      RL_LEVEL(pp) = level;
	  }

	if (RL_LEVEL(pp) > max_level)
	  max_level = RL_LEVEL(pp);
      }
  }
  
  compact_list(type_rl_list);

  *ptype_rl_list = type_rl_list;
  *pmax_level = max_level;
  *pbase_dir = base_dir;
}

/*======================================================================
//  Here starts the exposed front end functions.
//----------------------------------------------------------------------*/

/*======================================================================
//  fribidi_embedding_levels() is used in order to just get the
//  embedding levels.
//----------------------------------------------------------------------*/
void 
pango_log2vis_get_embedding_levels(
                     /* input */
		     gunichar *str,
		     int len,
		     PangoDirection *pbase_dir,
		     /* output */
		     guint8 *embedding_level_list
		     )
{
  TypeLink *type_rl_list, *pp;
  int max_level;
  FriBidiCharType fribidi_base_dir;

  fribidi_base_dir = (*pbase_dir == PANGO_DIRECTION_LTR) ? FRIBIDI_TYPE_L : FRIBIDI_TYPE_R;
  
  fribidi_analyse_string(str, len, &fribidi_base_dir,
			 /* output */
			 &type_rl_list,
			 &max_level);

  for (pp = type_rl_list->next; pp->next; pp = pp->next)
    {
      int i;
      int pos = RL_POS(pp);
      int len = RL_LEN(pp);
      int level = RL_LEVEL(pp);
      for (i=0; i<len; i++)
	embedding_level_list[pos + i] = level;
    }
  
  /* Free up the rl_list */
  pp->next = free_type_links;
  free_type_links = type_rl_list;
  
  *pbase_dir = (fribidi_base_dir == FRIBIDI_TYPE_L) ?  PANGO_DIRECTION_LTR : PANGO_DIRECTION_RTL;
}

