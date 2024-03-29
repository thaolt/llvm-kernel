/*	$NetBSD: ksyms.h,v 1.23 2009/03/07 21:59:25 ad Exp $	*/

/*
 * Copyright (c) 2001, 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_KSYMS_H_
#define _SYS_KSYMS_H_

#ifdef _KSYMS_PRIVATE
#define	ELFSIZE	ARCH_ELFSIZE
#include <sys/exec_elf.h>
#include <sys/queue.h>

struct ksyms_symtab {
	TAILQ_ENTRY(ksyms_symtab) sd_queue; /* All active tables */
	const char *sd_name;	/* Name of this table */
	Elf_Sym *sd_symstart;	/* Address of symbol table */
	uintptr_t sd_minsym;	/* symbol with minimum value */
	uintptr_t sd_maxsym;	/* symbol with maximum value */
	char *sd_strstart;	/* Address of corresponding string table */
	int sd_usroffset;	/* Real address for userspace */
	int sd_symsize;		/* Size in bytes of symbol table */
	int sd_strsize;		/* Size of string table */
	int sd_nglob;		/* Number of global symbols */
	bool sd_gone;		/* dead but around for open() */
};

/*
 * Static allocated ELF header.
 * Basic info is filled in at attach, sizes at open.
 */
#define	SYMTAB		1
#define	STRTAB		2
#define	SHSTRTAB	3
#define	SHBSS		4
#define NSECHDR		5

#define	NPRGHDR		1
#define	SHSTRSIZ	32

struct ksyms_hdr {
	Elf_Ehdr	kh_ehdr;
	Elf_Phdr	kh_phdr[NPRGHDR];
	Elf_Shdr	kh_shdr[NSECHDR];
	char 		kh_strtab[SHSTRSIZ];
};
#endif	/* _KSYMS_PRIVATE */

/*
 * Do a lookup of a symbol using the in-kernel lookup algorithm.
 */
struct ksyms_gsymbol {
	const char *kg_name;
	union {
		void *ku_sym;		 /* Normally Elf_Sym */
		unsigned long *ku_value;
	} _un;
#define	kg_sym _un.ku_sym
#define	kg_value _un.ku_value
};

#define	KIOCGSYMBOL	_IOW('l', 1, struct ksyms_gsymbol)
#define	KIOCGVALUE	_IOW('l', 2, struct ksyms_gsymbol)
#define	KIOCGSIZE	_IOR('l', 3, int)


#if defined(_KERNEL) || defined(_KMEMUSER)
/*
 * Definitions used in ksyms_getname() and ksyms_getval().
 */
#define	KSYMS_CLOSEST	0001	/* Nearest lower match */
#define	KSYMS_EXACT	0002	/* Only exact match allowed */
#define KSYMS_EXTERN	0000	/* Only external symbols (pseudo) */
#define KSYMS_PROC	0100	/* Procedures only */
#define KSYMS_ANY	0200	/* Also local symbols (DDB use only) */

/*
 * Prototypes
 */
int ksyms_getname(const char **, const char **, vaddr_t, int);
int ksyms_getval(const char *, const char *, unsigned long *, int);
int ksyms_getval_unlocked(const char *, const char *, unsigned long *, int);
int ksyms_addsymtab(const char *, void *, vsize_t, char *, vsize_t);
int ksyms_delsymtab(const char *);
void ksyms_init(void);
void ksyms_addsyms_elf(int, void *, void *);
void ksyms_addsyms_explicit(void *, void *, size_t, void *, size_t);
int ksyms_sift(char *, char *, int);
void ksyms_modload(const char *, void *, vsize_t, char *, vsize_t);
void ksyms_modunload(const char *);

#endif /* _KERNEL */
#endif /* _SYS_KSYMS_H_ */
