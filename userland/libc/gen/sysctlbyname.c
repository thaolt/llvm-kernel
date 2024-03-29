/*	$NetBSD: sysctlbyname.c,v 1.5 2009/07/22 22:53:41 alc Exp $ */

/*-
 * Copyright (c) 2003,2004 The NetBSD Foundation, Inc.
 *	All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
__RCSID("$NetBSD: sysctlbyname.c,v 1.5 2009/07/22 22:53:41 alc Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <sys/sysctl.h>

#ifdef __weak_alias
__weak_alias(sysctlbyname,_sysctlbyname)
#endif

/*
 * trivial sysctlbyname() function for the "lazy".
 */
int
sysctlbyname(const char *gname, void *oldp, size_t *oldlenp,
	     const void *newp, size_t newlen)
{
	int name[CTL_MAXNAME], rc;
	u_int namelen;

	rc = sysctlgetmibinfo(gname, &name[0], &namelen, NULL, NULL, NULL,
			      SYSCTL_VERSION);
	if (rc == 0)
		rc = sysctl(&name[0], namelen, oldp, oldlenp, newp, newlen);
	return (rc);
}
