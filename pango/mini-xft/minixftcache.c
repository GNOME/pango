/*
 * $XFree86: xc/lib/MiniXft/xftcache.c,v 1.2 2001/06/11 22:53:30 keithp Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#define mkdir(path, mode) _mkdir(path)
#endif

#include "minixftint.h"

typedef struct _MiniXftFileCacheEnt {
    struct _MiniXftFileCacheEnt *next;
    unsigned int	    hash;
    char		    *file;
    int			    id;
    time_t		    time;
    char		    *name;
    Bool		    referenced;
} MiniXftFileCacheEnt;

#define HASH_SIZE   509

typedef struct _MiniXftFileCache {
    MiniXftFileCacheEnt	*ents[HASH_SIZE];
    Bool		updated;
    int			entries;
    int			referenced;
} MiniXftFileCache;

static MiniXftFileCache	_MiniXftFileCache;

static unsigned int
_MiniXftFileCacheHash (char *string)
{
    unsigned int    h = 0;
    char	    c;

    while ((c = *string++))
	h = (h << 1) ^ c;
    return h;
}

char *
MiniXftFileCacheFind (char *file, int id, int *count)
{
    MiniXftFileCache    *cache;
    unsigned int    hash;
    char	    *match;
    MiniXftFileCacheEnt *c, *name;
    int		    maxid;
    struct stat	    statb;
    
    cache = &_MiniXftFileCache;
    match = file;
    
    hash = _MiniXftFileCacheHash (match);
    name = 0;
    maxid = -1;
    for (c = cache->ents[hash % HASH_SIZE]; c; c = c->next)
    {
	if (c->hash == hash && !strcmp (match, c->file))
	{
	    if (c->id > maxid)
		maxid = c->id;
	    if (c->id == id)
	    {
		if (stat (file, &statb) < 0)
		{
		    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
			printf (" file missing\n");
		    return 0;
		}
		if (statb.st_mtime != c->time)
		{
		    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
			printf (" timestamp mismatch (was %d is %d)\n",
				(int) c->time, (int) statb.st_mtime);
		    return 0;
		}
		if (!c->referenced)
		{
		    cache->referenced++;
		    c->referenced = True;
		}
		name = c;
	    }
	}
    }
    if (!name)
	return 0;
    *count = maxid + 1;
    return name->name;
}

/*
 * Cache file syntax is quite simple:
 *
 * "file_name" id time "font_name" \n
 */
 
static Bool
_MiniXftFileCacheReadString (FILE *f, char *dest, int len)
{
    int	    c;
    Bool    escape;

    while ((c = getc (f)) != EOF)
	if (c == '"')
	    break;
    if (c == EOF)
	return False;
    if (len == 0)
	return False;
    
    escape = False;
    while ((c = getc (f)) != EOF)
    {
	if (!escape)
	{
	    switch (c) {
	    case '"':
		*dest++ = '\0';
		return True;
	    case '\\':
		escape = True;
		continue;
	    }
	}
        if (--len <= 1)
	    return False;
	*dest++ = c;
	escape = False;
    }
    return False;
}

static Bool
_MiniXftFileCacheReadUlong (FILE *f, unsigned long *dest)
{
    unsigned long   t;
    int		    c;

    while ((c = getc (f)) != EOF)
    {
	if (!isspace (c))
	    break;
    }
    if (c == EOF)
	return False;
    t = 0;
    for (;;)
    {
	if (c == EOF || isspace (c))
	    break;
	if (!isdigit (c))
	    return False;
	t = t * 10 + (c - '0');
	c = getc (f);
    }
    *dest = t;
    return True;
}

static Bool
_MiniXftFileCacheReadInt (FILE *f, int *dest)
{
    unsigned long   t;
    Bool	    ret;

    ret = _MiniXftFileCacheReadUlong (f, &t);
    if (ret)
	*dest = (int) t;
    return ret;
}

static Bool
_MiniXftFileCacheReadTime (FILE *f, time_t *dest)
{
    unsigned long   t;
    Bool	    ret;

    ret = _MiniXftFileCacheReadUlong (f, &t);
    if (ret)
	*dest = (time_t) t;
    return ret;
}

static Bool
_MiniXftFileCacheAdd (MiniXftFileCache	*cache,
		  char		*file,
		  int		id,
		  time_t	time,
		  char		*name,
		  Bool		replace)
{
    MiniXftFileCacheEnt    *c;
    MiniXftFileCacheEnt    **prev, *old;
    unsigned int    hash;

    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
    {
	printf ("%s face %s/%d as %s\n", replace ? "Replace" : "Add",
		file, id, name);
    }
    hash = _MiniXftFileCacheHash (file);
    for (prev = &cache->ents[hash % HASH_SIZE]; 
	 (old = *prev);
	 prev = &(*prev)->next)
    {
	if (old->hash == hash && old->id == id && !strcmp (old->file, file))
	    break;
    }
    if (*prev)
    {
	if (!replace)
	    return False;

	old = *prev;
	if (old->referenced)
	    cache->referenced--;
	*prev = old->next;
	free (old);
	cache->entries--;
    }
	
    c = malloc (sizeof (MiniXftFileCacheEnt) +
		strlen (file) + 1 +
		strlen (name) + 1);
    if (!c)
	return False;
    c->next = *prev;
    *prev = c;
    c->hash = hash;
    c->file = (char *) (c + 1);
    c->id = id;
    c->name = c->file + strlen (file) + 1;
    strcpy (c->file, file);
    c->time = time;
    c->referenced = replace;
    strcpy (c->name, name);
    cache->entries++;
    return True;
}

void
MiniXftFileCacheDispose (void)
{
    MiniXftFileCache    *cache;
    MiniXftFileCacheEnt *c, *next;
    int		    h;

    cache = &_MiniXftFileCache;
    
    for (h = 0; h < HASH_SIZE; h++)
    {
	for (c = cache->ents[h]; c; c = next)
	{
	    next = c->next;
	    free (c);
	}
	cache->ents[h] = 0;
    }
    cache->entries = 0;
    cache->referenced = 0;
    cache->updated = False;
}

void
MiniXftFileCacheLoad (char *cache_file)
{
    MiniXftFileCache    *cache;
    FILE	    *f;
    char	    file[8192];
    int		    id;
    time_t	    time;
    char	    name[8192];

    f = fopen (cache_file, "r");
    if (!f)
	return;

    cache = &_MiniXftFileCache;

    cache->updated = False;
    while (_MiniXftFileCacheReadString (f, file, sizeof (file)) &&
	   _MiniXftFileCacheReadInt (f, &id) &&
	   _MiniXftFileCacheReadTime (f, &time) &&
	   _MiniXftFileCacheReadString (f, name, sizeof (name)))
    {
	(void) _MiniXftFileCacheAdd (cache, file, id, time, name, False);
    }
    fclose (f);
}

Bool
MiniXftFileCacheUpdate (char *file, int id, char *name)
{
    MiniXftFileCache    *cache;
    char	    *match;
    struct stat	    statb;
    Bool	    ret;

    cache = &_MiniXftFileCache;
    match = file;

    if (stat (file, &statb) < 0)
	return False;
    ret = _MiniXftFileCacheAdd (cache, match, id, 
			    statb.st_mtime, name, True);
    if (ret)
	cache->updated = True;
    return ret;
}

static Bool
_MiniXftFileCacheWriteString (FILE *f, char *string)
{
    char    c;

    if (putc ('"', f) == EOF)
	return False;
    while ((c = *string++))
    {
	switch (c) {
	case '"':
	case '\\':
	    if (putc ('\\', f) == EOF)
		return False;
	    /* fall through */
	default:
	    if (putc (c, f) == EOF)
		return False;
	}
    }
    if (putc ('"', f) == EOF)
	return False;
    return True;
}

static Bool
_MiniXftFileCacheWriteUlong (FILE *f, unsigned long t)
{
    int	    pow;
    unsigned long   temp, digit;

    temp = t;
    pow = 1;
    while (temp >= 10)
    {
	temp /= 10;
	pow *= 10;
    }
    temp = t;
    while (pow)
    {
	digit = temp / pow;
	if (putc ((char) digit + '0', f) == EOF)
	    return False;
	temp = temp - pow * digit;
	pow = pow / 10;
    }
    return True;
}

static Bool
_MiniXftFileCacheWriteInt (FILE *f, int i)
{
    return _MiniXftFileCacheWriteUlong (f, (unsigned long) i);
}

static Bool
_MiniXftFileCacheWriteTime (FILE *f, time_t t)
{
    return _MiniXftFileCacheWriteUlong (f, (unsigned long) t);
}

Bool
MiniXftFileCacheSave (char *cache_file)
{
    MiniXftFileCache    *cache;
    char	    *lck;
    char	    *tmp;
    FILE	    *f;
    int		    h;
    MiniXftFileCacheEnt *c;

    cache = &_MiniXftFileCache;

    if (!cache->updated && cache->referenced == cache->entries)
	return True;
    
    lck = malloc (strlen (cache_file)*2 + 4);
    if (!lck)
	goto bail0;
    tmp = lck + strlen (cache_file) + 2;
    strcpy (lck, cache_file);
    strcat (lck, "L");
    strcpy (tmp, cache_file);
    strcat (tmp, "T");
#ifndef _WIN32
    if (link (lck, cache_file) < 0 && errno != ENOENT)
	goto bail1;
#else
    if (mkdir (lck, 0666) < 0)
        goto bail1;
#endif
    if (access (tmp, F_OK) == 0)
	goto bail2;
    f = fopen (tmp, "w");
    if (!f)
	goto bail2;

    for (h = 0; h < HASH_SIZE; h++)
    {
	for (c = cache->ents[h]; c; c = c->next)
	{
	    if (!c->referenced)
		continue;
	    if (!_MiniXftFileCacheWriteString (f, c->file))
		goto bail4;
	    if (putc (' ', f) == EOF)
		goto bail4;
	    if (!_MiniXftFileCacheWriteInt (f, c->id))
		goto bail4;
	    if (putc (' ', f) == EOF)
		goto bail4;
	    if (!_MiniXftFileCacheWriteTime (f, c->time))
		goto bail4;
	    if (putc (' ', f) == EOF)
		goto bail4;
	    if (!_MiniXftFileCacheWriteString (f, c->name))
		goto bail4;
	    if (putc ('\n', f) == EOF)
		goto bail4;
	}
    }

    if (fclose (f) == EOF)
	goto bail3;
    
    if (rename (tmp, cache_file) < 0)
	goto bail3;
    
#ifndef _WIN32
    unlink (lck);
#else
    rmdir (lck);
#endif
    cache->updated = False;
    return True;

bail4:
    fclose (f);
bail3:
    unlink (tmp);
bail2:
#ifndef _WIN32
    unlink (lck);
#else
    rmdir (lck);
#endif
bail1:
    free (lck);
bail0:
    return False;
}

Bool
MiniXftFileCacheReadDir (MiniXftFontSet *set, const char *cache_file)
{
    MiniXftPattern	    *font;
    FILE	    *f;
    char	    *path;
    char	    *base;
    char	    file[8192];
    int		    id;
    char	    name[8192];
    Bool	    ret = False;

    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
    {
	printf ("MiniXftFileCacheReadDir cache_file \"%s\"\n", cache_file);
    }
    
    f = fopen (cache_file, "r");
    if (!f)
    {
	if (_MiniXftFontDebug () & XFT_DBG_CACHE)
	{
	    printf (" no cache file\n");
	}
	goto bail0;
    }

    base = strrchr (cache_file, '/');
    if (!base)
	goto bail1;
    base++;
    path = malloc (base - cache_file + 8192 + 1);
    if (!path)
	goto bail1;
    memcpy (path, cache_file, base - cache_file);
    base = path + (base - cache_file);
    
    while (_MiniXftFileCacheReadString (f, file, sizeof (file)) &&
	   _MiniXftFileCacheReadInt (f, &id) &&
	   _MiniXftFileCacheReadString (f, name, sizeof (name)))
    {
	font = MiniXftNameParse (name);
	if (font)
	{
	    strcpy (base, file);
	    if (_MiniXftFontDebug () & XFT_DBG_CACHEV)
	    {
		printf (" dir cache file \"%s\"\n", file);
	    }
	    MiniXftPatternAddString (font, XFT_FILE, path);
	    if (!MiniXftFontSetAdd (set, font))
		goto bail2;
	}
    }
    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
    {
	printf (" cache loaded\n");
    }
    
    ret = True;
bail2:
    free (path);
bail1:
    fclose (f);
bail0:
    return ret;
}

Bool
MiniXftFileCacheWriteDir (MiniXftFontSet *set, const char *cache_file)
{
    MiniXftPattern	    *font;
    FILE	    *f;
    char	    name[8192];
    char	    *file, *base;
    int		    n;
    int		    id;

    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
	printf ("MiniXftFileCacheWriteDir cache_file \"%s\"\n", cache_file);
    
    f = fopen (cache_file, "w");
    if (!f)
    {
	if (_MiniXftFontDebug () & XFT_DBG_CACHE)
	    printf (" can't create \"%s\"\n", cache_file);
	goto bail0;
    }
    for (n = 0; n < set->nfont; n++)
    {
	font = set->fonts[n];
	if (MiniXftPatternGetString (font, XFT_FILE, 0, &file) != MiniXftResultMatch)
	    goto bail1;
	base = strrchr (file, '/');
	if (base)
	    base = base + 1;
	else
	    base = file;
	if (MiniXftPatternGetInteger (font, XFT_INDEX, 0, &id) != MiniXftResultMatch)
	    goto bail1;
	if (!MiniXftNameUnparse (font, name, sizeof (name)))
	    goto bail1;
	if (_MiniXftFontDebug () & XFT_DBG_CACHEV)
	    printf (" write file \"%s\"\n", base);
	if (!_MiniXftFileCacheWriteString (f, base))
	    goto bail1;
	if (putc (' ', f) == EOF)
	    goto bail1;
	if (!_MiniXftFileCacheWriteInt (f, id))
	    goto bail1;
        if (putc (' ', f) == EOF)
	    goto bail1;
	if (!_MiniXftFileCacheWriteString (f, name))
	    goto bail1;
	if (putc ('\n', f) == EOF)
	    goto bail1;
    }
    if (fclose (f) == EOF)
	goto bail0;
    
    if (_MiniXftFontDebug () & XFT_DBG_CACHE)
	printf (" cache written\n");
    return True;
    
bail1:
    fclose (f);
bail0:
    unlink (cache_file);
    return False;
}
