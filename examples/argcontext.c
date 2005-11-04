/* argcontext.c - Simple command line argument parsing
 * Copyright 1999, 2003 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "argcontext.h"

struct _ArgContext
{
  GPtrArray *tables;
  gpointer cb_data;
};

GQuark
arg_context_error_quark (void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string ("arg-context-error-quark");

  return q;
}

ArgContext *
arg_context_new (gpointer cb_data)
{
  ArgContext *result = g_new (ArgContext, 1);
  result->tables = g_ptr_array_new ();
  result->cb_data = cb_data;

  return result;
}

void
arg_context_free (ArgContext *context)
{
  g_ptr_array_free (context->tables, TRUE);
  g_free (context);
}

void
arg_context_add_table (ArgContext    *context,
		       const ArgDesc *table)
{
  g_ptr_array_add (context->tables, (ArgDesc *)table);
}

static gboolean
parse_int (const char *arg_name,
	   const char *arg,
	   gint       *result,
	   GError    **error)
{
  gchar *end;
  glong tmp = strtol (arg, &end, 0);

  errno = 0;
  
  if (*arg == '\0' || *end != '\0')
    {
      g_set_error (error,
		   ARG_CONTEXT_ERROR, ARG_CONTEXT_ERROR_BAD_VALUE,
		   "Cannot parse integer value '%s' for --%s",
		   arg, arg_name);
      return FALSE;
    }

  *result = tmp;
  if (*result != tmp || errno == ERANGE)
    {
      g_set_error (error,
		   ARG_CONTEXT_ERROR, ARG_CONTEXT_ERROR_BAD_VALUE,
		   "Integer value '%s' for %s out of range",
		   arg, arg_name);
      return FALSE;
    }

  return TRUE;
}

void
arg_context_print_help (ArgContext *context)
{
  unsigned int j, k;
  int max_name_len = 0;

  for (j = 0; j < context->tables->len; j++)
    {
      ArgDesc *table = context->tables->pdata[j];
      for (k = 0; table[k].name; k++)
	max_name_len = MAX (strlen (table[k].name), max_name_len);
    }

  for (j = 0; j < context->tables->len; j++)
    {
      ArgDesc *table = context->tables->pdata[j];
      for (k = 0; table[k].name; k++)
	g_print ("  --%-*s %s\n",
		 max_name_len, table[k].name,
		 table[k].help ? table[k].help : "");
    }
}

gboolean
arg_context_parse (ArgContext *context,
		   gint       *argc,
		   gchar    ***argv,
		   GError    **error)
{
  int i, j, k;

  if (argc && argv)
    {
      for (i = 1; i < *argc; i++)
	{
	  char *arg;

	  if ((*argv)[i][0] == '-' && (*argv)[i][1] != '-')
	    {
	      g_set_error (error,
			   ARG_CONTEXT_ERROR, ARG_CONTEXT_ERROR_UNKNOWN_OPTION,
			   "Unknown option %s", (*argv)[i]);
	      return FALSE;
	    }
	  
	  if (!((*argv)[i][0] == '-' && (*argv)[i][1] == '-'))
	    continue;
	  
	  arg = (*argv)[i] + 2;

	  /* '--' terminates list of arguments */
	  if (*arg == '\0')
	    {
	      (*argv)[i] = NULL;
	      break;
	    }
	      
	  for (j = 0; j < context->tables->len; j++)
	    {
	      ArgDesc *table = context->tables->pdata[j];
	      for (k = 0; table[k].name; k++)
		{
		  switch (table[k].type)
		    {
		    case ARG_STRING:
		    case ARG_CALLBACK:
		    case ARG_INT:
		      {
			int len = strlen (table[k].name);
			
			if (strncmp (arg, table[k].name, len) == 0 &&
			    (arg[len] == '=' || arg[len] == 0))
			  {
			    const char *value = NULL;

			    (*argv)[i] = NULL;

			    if (arg[len] == '=')
			      value = arg + len + 1;
			    else if (i < *argc - 1)
			      {
				value = (*argv)[i + 1];
				(*argv)[i+1] = NULL;
				i++;
			      }
			    else
			      value = "";

			    switch (table[k].type)
			      {
			      case ARG_STRING:
				*(const gchar **)table[k].location = value;
				break;
			      case ARG_INT:
				if (!parse_int (table[k].name, value,
						(gint *)table[k].location,
						error))
				  return FALSE;
				break;
			      case ARG_CALLBACK:
				(*table[k].callback)(context, table[k].name, value, context->cb_data);
				break;
			      default:
				;
			      }

			    goto next_arg;
			  }
			break;
		      }
		    case ARG_BOOL:
		    case ARG_NOBOOL:
		      if (strcmp (arg, table[k].name) == 0)
			{
			  (*argv)[i] = NULL;
			  
			  *(gboolean *)table[k].location = (table[k].type == ARG_BOOL) ? TRUE : FALSE;
			  goto next_arg;
			}
		    }
		}
	    }
	  g_set_error (error,
		       ARG_CONTEXT_ERROR, ARG_CONTEXT_ERROR_UNKNOWN_OPTION,
		       "Unknown option --%s",
		       arg);
	  return FALSE;
	  
	next_arg:
	  ;
	}
	  
      for (i = 1; i < *argc; i++)
	{
	  for (k = i; k < *argc; k++)
	    if ((*argv)[k] != NULL)
	      break;
	  
	  if (k > i)
	    {
	      k -= i;
	      for (j = i + k; j < *argc; j++)
		(*argv)[j-k] = (*argv)[j];
	      *argc -= k;
	    }
	}
    }

  return TRUE;
}
