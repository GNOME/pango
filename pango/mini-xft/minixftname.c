/*
 * $XFree86: xc/lib/MiniXft/xftname.c,v 1.10 2001/03/30 18:50:18 keithp Exp $
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "minixftint.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _MiniXftObjectType {
    const char	*object;
    MiniXftType	type;
} MiniXftObjectType;

static const MiniXftObjectType _MiniXftObjectTypes[] = {
    { XFT_FAMILY,	MiniXftTypeString, },
    { XFT_STYLE,	MiniXftTypeString, },
    { XFT_SLANT,	MiniXftTypeInteger, },
    { XFT_WEIGHT,	MiniXftTypeInteger, },
    { XFT_SIZE,		MiniXftTypeDouble, },
    { XFT_PIXEL_SIZE,	MiniXftTypeDouble, },
    { XFT_ENCODING,	MiniXftTypeString, },
    { XFT_SPACING,	MiniXftTypeInteger, },
    { XFT_FOUNDRY,	MiniXftTypeString, },
    { XFT_CORE,		MiniXftTypeBool, },
    { XFT_ANTIALIAS,	MiniXftTypeBool, },
    { XFT_XLFD,		MiniXftTypeString, },
    { XFT_FILE,		MiniXftTypeString, },
    { XFT_INDEX,	MiniXftTypeInteger, },
    { XFT_RASTERIZER,	MiniXftTypeString, },
    { XFT_OUTLINE,	MiniXftTypeBool, },
    { XFT_SCALABLE,	MiniXftTypeBool, },
    { XFT_RGBA,		MiniXftTypeInteger, },
    { XFT_SCALE,	MiniXftTypeDouble, },
    { XFT_RENDER,	MiniXftTypeBool, },
    { XFT_MINSPACE,	MiniXftTypeBool, },
    { XFT_CHAR_WIDTH,	MiniXftTypeInteger },
    { XFT_CHAR_HEIGHT,	MiniXftTypeInteger },
    { XFT_MATRIX,	MiniXftTypeMatrix },
};

#define NUM_OBJECT_TYPES    (sizeof _MiniXftObjectTypes / sizeof _MiniXftObjectTypes[0])

static const MiniXftObjectType *
MiniXftNameGetType (const char *object)
{
    int	    i;
    
    for (i = 0; i < NUM_OBJECT_TYPES; i++)
    {
	if (!_MiniXftStrCmpIgnoreCase (object, _MiniXftObjectTypes[i].object))
	    return &_MiniXftObjectTypes[i];
    }
    return 0;
}

typedef struct _MiniXftConstant {
    const char  *name;
    const char	*object;
    int		value;
} MiniXftConstant;

static MiniXftConstant MiniXftConstants[] = {
    { "light",		"weight",   XFT_WEIGHT_LIGHT, },
    { "medium",		"weight",   XFT_WEIGHT_MEDIUM, },
    { "demibold",	"weight",   XFT_WEIGHT_DEMIBOLD, },
    { "bold",		"weight",   XFT_WEIGHT_BOLD, },
    { "black",		"weight",   XFT_WEIGHT_BLACK, },

    { "roman",		"slant",    XFT_SLANT_ROMAN, },
    { "italic",		"slant",    XFT_SLANT_ITALIC, },
    { "oblique",	"slant",    XFT_SLANT_OBLIQUE, },

    { "proportional",	"spacing",  XFT_PROPORTIONAL, },
    { "mono",		"spacing",  XFT_MONO, },
    { "charcell",	"spacing",  XFT_CHARCELL, },

    { "rgb",		"rgba",	    XFT_RGBA_RGB, },
    { "bgr",		"rgba",	    XFT_RGBA_BGR, },
    { "vrgb",		"rgba",	    XFT_RGBA_VRGB },
    { "vbgr",		"rgba",	    XFT_RGBA_VBGR },
};

#define NUM_XFT_CONSTANTS   (sizeof MiniXftConstants/sizeof MiniXftConstants[0])

static MiniXftConstant *
_MiniXftNameConstantLookup (char *string)
{
    int	i;
    
    for (i = 0; i < NUM_XFT_CONSTANTS; i++)
	if (!_MiniXftStrCmpIgnoreCase (string, MiniXftConstants[i].name))
	    return &MiniXftConstants[i];
    return 0;
}

Bool
MiniXftNameConstant (char *string, int *result)
{
    MiniXftConstant	*c;

    if ((c = _MiniXftNameConstantLookup(string)))
    {
	*result = c->value;
	return True;
    }
    return False;
}

static MiniXftValue
_MiniXftNameConvert (MiniXftType type, char *string, MiniXftMatrix *m)
{
    MiniXftValue	v;

    v.type = type;
    switch (v.type) {
    case MiniXftTypeInteger:
	if (!MiniXftNameConstant (string, &v.u.i))
	    v.u.i = atoi (string);
	break;
    case MiniXftTypeString:
	v.u.s = string;
	break;
    case MiniXftTypeBool:
	v.u.b = MiniXftDefaultParseBool (string);
	break;
    case MiniXftTypeDouble:
	v.u.d = strtod (string, 0);
	break;
    case MiniXftTypeMatrix:
	v.u.m = m;
	sscanf (string, "%lg %lg %lg %lg", &m->xx, &m->xy, &m->yx, &m->yy);
	break;
    default:
	break;
    }
    return v;
}

static const char *
_MiniXftNameFindNext (const char *cur, const char *delim, char *save, char *last)
{
    char    c;
    
    while ((c = *cur))
    {
	if (c == '\\')
	{
	    ++cur;
	    if (!(c = *cur))
		break;
	}
	else if (strchr (delim, c))
	    break;
	++cur;
	*save++ = c;
    }
    *save = 0;
    *last = *cur;
    if (*cur)
	cur++;
    return cur;
}

MiniXftPattern *
MiniXftNameParse (const char *name)
{
    char		*save;
    MiniXftPattern		*pat;
    double		d;
    char		*e;
    char		delim;
    MiniXftValue		v;
    MiniXftMatrix		m;
    const MiniXftObjectType	*t;
    MiniXftConstant		*c;

    save = malloc (strlen (name) + 1);
    if (!save)
	goto bail0;
    pat = MiniXftPatternCreate ();
    if (!pat)
	goto bail1;

    for (;;)
    {
	name = _MiniXftNameFindNext (name, "-,:", save, &delim);
	if (save[0])
	{
	    if (!MiniXftPatternAddString (pat, XFT_FAMILY, save))
		goto bail2;
	}
	if (delim != ',')
	    break;
    }
    if (delim == '-')
    {
	for (;;)
	{
	    name = _MiniXftNameFindNext (name, "-,:", save, &delim);
	    d = strtod (save, &e);
	    if (e != save)
	    {
		if (!MiniXftPatternAddDouble (pat, XFT_SIZE, d))
		    goto bail2;
	    }
	    if (delim != ',')
		break;
	}
    }
    while (delim == ':')
    {
	name = _MiniXftNameFindNext (name, "=_:", save, &delim);
	if (save[0])
	{
	    if (delim == '=' || delim == '_')
	    {
		t = MiniXftNameGetType (save);
		for (;;)
		{
		    name = _MiniXftNameFindNext (name, ":,", save, &delim);
		    if (save[0] && t)
		    {
			v = _MiniXftNameConvert (t->type, save, &m);
			if (!MiniXftPatternAdd (pat, t->object, v, True))
			    goto bail2;
		    }
		    if (delim != ',')
			break;
		}
	    }
	    else
	    {
		if ((c = _MiniXftNameConstantLookup (save)))
		{
		    if (!MiniXftPatternAddInteger (pat, c->object, c->value))
			goto bail2;
		}
	    }
	}
    }

    free (save);
    return pat;

bail2:
    MiniXftPatternDestroy (pat);
bail1:
    free (save);
bail0:
    return 0;
}

static Bool
_MiniXftNameUnparseString (const char *string, char *escape, char **destp, int *lenp)
{
    int	    len = *lenp;
    char    *dest = *destp;
    char    c;

    while ((c = *string++))
    {
	if (escape && strchr (escape, c))
	{
	    if (len-- == 0)
		return False;
	    *dest++ = escape[0];
	}
	if (len-- == 0)
	    return False;
	*dest++ = c;
    }
    *destp = dest;
    *lenp = len;
    return True;
}

static Bool
_MiniXftNameUnparseValue (MiniXftValue v, char *escape, char **destp, int *lenp)
{
    char    temp[1024];
    
    switch (v.type) {
    case MiniXftTypeVoid:
	return True;
    case MiniXftTypeInteger:
	sprintf (temp, "%d", v.u.i);
	return _MiniXftNameUnparseString (temp, 0, destp, lenp);
    case MiniXftTypeDouble:
	sprintf (temp, "%g", v.u.d);
	return _MiniXftNameUnparseString (temp, 0, destp, lenp);
    case MiniXftTypeString:
	return _MiniXftNameUnparseString (v.u.s, escape, destp, lenp);
    case MiniXftTypeBool:
	return _MiniXftNameUnparseString (v.u.b ? "True" : "False", 0, destp, lenp);
    case MiniXftTypeMatrix:
	sprintf (temp, "%g %g %g %g", 
		 v.u.m->xx, v.u.m->xy, v.u.m->yx, v.u.m->yy);
	return _MiniXftNameUnparseString (temp, 0, destp, lenp);
    }
    return False;
}

static Bool
_MiniXftNameUnparseValueList (MiniXftValueList *v, char *escape, char **destp, int *lenp)
{
    while (v)
    {
	if (!_MiniXftNameUnparseValue (v->value, escape, destp, lenp))
	    return False;
	if ((v = v->next))
	    if (!_MiniXftNameUnparseString (",", 0, destp, lenp))
		return False;
    }
    return True;
}

#define XFT_ESCAPE_FIXED    "\\-:,"
#define XFT_ESCAPE_VARIABLE "\\=_:,"

Bool
MiniXftNameUnparse (MiniXftPattern *pat, char *dest, int len)
{
    int			i;
    MiniXftPatternElt	*e;
    const MiniXftObjectType *o;

    e = MiniXftPatternFind (pat, XFT_FAMILY, False);
    if (e)
    {
	if (!_MiniXftNameUnparseValueList (e->values, XFT_ESCAPE_FIXED,
				       &dest, &len))
	    return False;
    }
    e = MiniXftPatternFind (pat, XFT_SIZE, False);
    if (e)
    {
	if (!_MiniXftNameUnparseString ("-", 0, &dest, &len))
	    return False;
	if (!_MiniXftNameUnparseValueList (e->values, XFT_ESCAPE_FIXED, &dest, &len))
	    return False;
    }
    for (i = 0; i < NUM_OBJECT_TYPES; i++)
    {
	o = &_MiniXftObjectTypes[i];
	if (!strcmp (o->object, XFT_FAMILY) || 
	    !strcmp (o->object, XFT_SIZE) ||
	    !strcmp (o->object, XFT_FILE))
	    continue;
	
	e = MiniXftPatternFind (pat, o->object, False);
	if (e)
	{
	    if (!_MiniXftNameUnparseString (":", 0, &dest, &len))
		return False;
	    if (!_MiniXftNameUnparseString (o->object, XFT_ESCAPE_VARIABLE, 
					&dest, &len))
		return False;
	    if (!_MiniXftNameUnparseString ("=", 0, &dest, &len))
		return False;
	    if (!_MiniXftNameUnparseValueList (e->values, XFT_ESCAPE_VARIABLE, 
					   &dest, &len))
		return False;
	}
    }
    if (len == 0)
	return False;
    *dest = '\0';
    return True;
}
