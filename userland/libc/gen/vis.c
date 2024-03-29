/*	$NetBSD: vis.c,v 1.40 2009/02/11 13:52:28 christos Exp $	*/

/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1999, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: vis.c,v 1.40 2009/02/11 13:52:28 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>

#include <assert.h>
#include <vis.h>
#include <stdlib.h>

#ifdef __weak_alias
__weak_alias(strsvis,_strsvis)
__weak_alias(strsvisx,_strsvisx)
__weak_alias(strvis,_strvis)
__weak_alias(strvisx,_strvisx)
__weak_alias(svis,_svis)
__weak_alias(vis,_vis)
#endif

#if !HAVE_VIS || !HAVE_SVIS
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static char *do_svis(char *, int, int, int, const char *);

#undef BELL
#define BELL '\a'

#define isoctal(c)	(((u_char)(c)) >= '0' && ((u_char)(c)) <= '7')
#define iswhite(c)	(c == ' ' || c == '\t' || c == '\n')
#define issafe(c)	(c == '\b' || c == BELL || c == '\r')
#define xtoa(c)		"0123456789abcdef"[c]
#define XTOA(c)		"0123456789ABCDEF"[c]

#define MAXEXTRAS	5

#define MAKEEXTRALIST(flag, extra, orig_str)				      \
do {									      \
	const char *orig = orig_str;					      \
	const char *o = orig;						      \
	char *e;							      \
	while (*o++)							      \
		continue;						      \
	extra = malloc((size_t)((o - orig) + MAXEXTRAS));		      \
	if (!extra) break;						      \
	for (o = orig, e = extra; (*e++ = *o++) != '\0';)		      \
		continue;						      \
	e--;								      \
	if (flag & VIS_SP) *e++ = ' ';					      \
	if (flag & VIS_TAB) *e++ = '\t';				      \
	if (flag & VIS_NL) *e++ = '\n';					      \
	if ((flag & VIS_NOSLASH) == 0) *e++ = '\\';			      \
	*e = '\0';							      \
} while (/*CONSTCOND*/0)

/*
 * This is do_hvis, for HTTP style (RFC 1808)
 */
static char *
do_hvis(char *dst, int c, int flag, int nextc, const char *extra)
{
	if (!isascii(c) || !isalnum(c) || strchr("$-_.+!*'(),", c) != NULL) {
		*dst++ = '%';
		*dst++ = xtoa(((unsigned int)c >> 4) & 0xf);
		*dst++ = xtoa((unsigned int)c & 0xf);
	} else {
		dst = do_svis(dst, c, flag, nextc, extra);
	}
	return dst;
}

/*
 * This is do_mvis, for Quoted-Printable MIME (RFC 2045)
 * NB: No handling of long lines or CRLF.
 */
static char *
do_mvis(char *dst, int c, int flag, int nextc, const char *extra)
{
	if ((c != '\n') &&
	    /* Space at the end of the line */
	    ((isspace(c) && (nextc == '\r' || nextc == '\n')) ||
	    /* Out of range */
	    (!isspace(c) && (c < 33 || (c > 60 && c < 62) || c > 126)) ||
	    /* Specific char to be escaped */ 
	    strchr("#$@[\\]^`{|}~", c) != NULL)) {
		*dst++ = '=';
		*dst++ = XTOA(((unsigned int)c >> 4) & 0xf);
		*dst++ = XTOA((unsigned int)c & 0xf);
	} else {
		dst = do_svis(dst, c, flag, nextc, extra);
	}
	return dst;
}

/*
 * This is do_vis, the central code of vis.
 * dst:	      Pointer to the destination buffer
 * c:	      Character to encode
 * flag:      Flag word
 * nextc:     The character following 'c'
 * extra:     Pointer to the list of extra characters to be
 *	      backslash-protected.
 */
static char *
do_svis(char *dst, int c, int flag, int nextc, const char *extra)
{
	int isextra;
	isextra = strchr(extra, c) != NULL;
	if (!isextra && isascii(c) && (isgraph(c) || iswhite(c) ||
	    ((flag & VIS_SAFE) && issafe(c)))) {
		*dst++ = c;
		return dst;
	}
	if (flag & VIS_CSTYLE) {
		switch (c) {
		case '\n':
			*dst++ = '\\'; *dst++ = 'n';
			return dst;
		case '\r':
			*dst++ = '\\'; *dst++ = 'r';
			return dst;
		case '\b':
			*dst++ = '\\'; *dst++ = 'b';
			return dst;
		case BELL:
			*dst++ = '\\'; *dst++ = 'a';
			return dst;
		case '\v':
			*dst++ = '\\'; *dst++ = 'v';
			return dst;
		case '\t':
			*dst++ = '\\'; *dst++ = 't';
			return dst;
		case '\f':
			*dst++ = '\\'; *dst++ = 'f';
			return dst;
		case ' ':
			*dst++ = '\\'; *dst++ = 's';
			return dst;
		case '\0':
			*dst++ = '\\'; *dst++ = '0';
			if (isoctal(nextc)) {
				*dst++ = '0';
				*dst++ = '0';
			}
			return dst;
		default:
			if (isgraph(c)) {
				*dst++ = '\\'; *dst++ = c;
				return dst;
			}
		}
	}
	if (isextra || ((c & 0177) == ' ') || (flag & VIS_OCTAL)) {
		*dst++ = '\\';
		*dst++ = (u_char)(((u_int32_t)(u_char)c >> 6) & 03) + '0';
		*dst++ = (u_char)(((u_int32_t)(u_char)c >> 3) & 07) + '0';
		*dst++ =			     (c	      & 07) + '0';
	} else {
		if ((flag & VIS_NOSLASH) == 0) *dst++ = '\\';
		if (c & 0200) {
			c &= 0177; *dst++ = 'M';
		}
		if (iscntrl(c)) {
			*dst++ = '^';
			if (c == 0177)
				*dst++ = '?';
			else
				*dst++ = c + '@';
		} else {
			*dst++ = '-'; *dst++ = c;
		}
	}
	return dst;
}

typedef char *(*visfun_t)(char *, int, int, int, const char *);

/*
 * Return the appropriate encoding function depending on the flags given.
 */
static visfun_t
getvisfun(int flag)
{
	if (flag & VIS_HTTPSTYLE)
		return do_hvis;
	if (flag & VIS_MIMESTYLE)
		return do_mvis;
	return do_svis;
}

/*
 * svis - visually encode characters, also encoding the characters
 *	  pointed to by `extra'
 */
char *
svis(char *dst, int c, int flag, int nextc, const char *extra)
{
	char *nextra = NULL;
	visfun_t f;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(extra != NULL);
	MAKEEXTRALIST(flag, nextra, extra);
	if (!nextra) {
		*dst = '\0';		/* can't create nextra, return "" */
		return dst;
	}
	f = getvisfun(flag);
	dst = (*f)(dst, c, flag, nextc, nextra);
	free(nextra);
	*dst = '\0';
	return dst;
}


/*
 * strsvis, strsvisx - visually encode characters from src into dst
 *
 *	Extra is a pointer to a \0-terminated list of characters to
 *	be encoded, too. These functions are useful e. g. to
 *	encode strings in such a way so that they are not interpreted
 *	by a shell.
 *
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 *
 *	Strsvisx encodes exactly len bytes from src into dst.
 *	This is useful for encoding a block of data.
 */
int
strsvis(char *dst, const char *csrc, int flag, const char *extra)
{
	int c;
	char *start;
	char *nextra = NULL;
	const unsigned char *src = (const unsigned char *)csrc;
	visfun_t f;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
	_DIAGASSERT(extra != NULL);
	MAKEEXTRALIST(flag, nextra, extra);
	if (!nextra) {
		*dst = '\0';		/* can't create nextra, return "" */
		return 0;
	}
	f = getvisfun(flag);
	for (start = dst; (c = *src++) != '\0'; /* empty */)
		dst = (*f)(dst, c, flag, *src, nextra);
	free(nextra);
	*dst = '\0';
	return (int)(dst - start);
}


int
strsvisx(char *dst, const char *csrc, size_t len, int flag, const char *extra)
{
	unsigned char c;
	char *start;
	char *nextra = NULL;
	const unsigned char *src = (const unsigned char *)csrc;
	visfun_t f;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
	_DIAGASSERT(extra != NULL);
	MAKEEXTRALIST(flag, nextra, extra);
	if (! nextra) {
		*dst = '\0';		/* can't create nextra, return "" */
		return 0;
	}

	f = getvisfun(flag);
	for (start = dst; len > 0; len--) {
		c = *src++;
		dst = (*f)(dst, c, flag, len > 1 ? *src : '\0', nextra);
	}
	free(nextra);
	*dst = '\0';
	return (int)(dst - start);
}
#endif

#if !HAVE_VIS
/*
 * vis - visually encode characters
 */
char *
vis(char *dst, int c, int flag, int nextc)
{
	char *extra = NULL;
	unsigned char uc = (unsigned char)c;
	visfun_t f;

	_DIAGASSERT(dst != NULL);

	MAKEEXTRALIST(flag, extra, "");
	if (! extra) {
		*dst = '\0';		/* can't create extra, return "" */
		return dst;
	}
	f = getvisfun(flag);
	dst = (*f)(dst, uc, flag, nextc, extra);
	free(extra);
	*dst = '\0';
	return dst;
}


/*
 * strvis, strvisx - visually encode characters from src into dst
 *
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 *
 *	Strvisx encodes exactly len bytes from src into dst.
 *	This is useful for encoding a block of data.
 */
int
strvis(char *dst, const char *src, int flag)
{
	char *extra = NULL;
	int rv;

	MAKEEXTRALIST(flag, extra, "");
	if (!extra) {
		*dst = '\0';		/* can't create extra, return "" */
		return 0;
	}
	rv = strsvis(dst, src, flag, extra);
	free(extra);
	return rv;
}


int
strvisx(char *dst, const char *src, size_t len, int flag)
{
	char *extra = NULL;
	int rv;

	MAKEEXTRALIST(flag, extra, "");
	if (!extra) {
		*dst = '\0';		/* can't create extra, return "" */
		return 0;
	}
	rv = strsvisx(dst, src, len, flag, extra);
	free(extra);
	return rv;
}
#endif
