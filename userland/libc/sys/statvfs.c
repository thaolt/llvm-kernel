/*	$NetBSD: statvfs.c,v 1.6 2008/04/28 20:23:00 martin Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: statvfs.c,v 1.6 2008/04/28 20:23:00 martin Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/statvfs.h>

#ifdef __weak_alias
__weak_alias(statvfs,_statvfs)
__weak_alias(fstatvfs,_fstatvfs)
#endif

int
statvfs(const char *file, struct statvfs *st)
{
	return statvfs1(file, st, ST_WAIT);
}

int
fstatvfs(int f, struct statvfs *st)
{
	return fstatvfs1(f, st, ST_WAIT);
}

#if 0
int	__fhstatvfs140(const void *fhp, size_t fh_size, struct statvfs *buf,
	int flags);
int	__fhstatvfs40(const void *fh, size_t fh_size, struct statvfs *st);

int
__fhstatvfs40(const void *fh, size_t fh_size, struct statvfs *st)
{
	return __fhstatvfs140(fh, fh_size, st, ST_WAIT);
}
#endif

