/* argcontext.h - Simple command line argument parsing
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

#ifndef __ARG_CONTEXT_H__
#define __ARG_CONTEXT_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum 
{
  ARG_STRING,
  ARG_INT,
  ARG_BOOL,
  ARG_NOBOOL,
  ARG_CALLBACK
} ArgType;

typedef struct _ArgContext ArgContext;
typedef struct _ArgDesc ArgDesc;

#define ARG_CONTEXT_ERROR arg_context_error_quark ()

typedef enum
{
  ARG_CONTEXT_ERROR_UNKNOWN_OPTION,
  ARG_CONTEXT_ERROR_BAD_VALUE,
  ARG_CONTEXT_ERROR_FAILED
} ArgContextError;

typedef void (*ArgFunc) (ArgContext *arg_context,
			 const char *name,
			 const char *arg,
			 gpointer    data);

struct _ArgDesc
{
  const char *name;
  const char *help;
  ArgType type;
  gpointer location;
  ArgFunc callback;
};

GQuark      arg_context_error_quark (void);

ArgContext *arg_context_new        (gpointer         cb_data);
void        arg_context_free       (ArgContext      *context);
void        arg_context_print_help (ArgContext      *context);
void        arg_context_add_table  (ArgContext      *context,
				    const ArgDesc   *table);
gboolean    arg_context_parse      (ArgContext      *context,
				    gint            *argc,
				    gchar         ***argv,
				    GError         **error);

G_END_DECLS

#endif /* __ARG_CONTEXT_H__ */
