/*	$NetBSD: uvm_km.h,v 1.18 2008/12/01 10:54:57 ad Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * from: Id: uvm_km.h,v 1.1.2.2 1997/12/30 12:03:15 mrg Exp
 */

#ifndef _UVM_UVM_KM_H_
#define _UVM_UVM_KM_H_

/*
 * uvm_km.h
 */

#ifdef _KERNEL

/*
 * prototypes
 */

void uvm_km_init(vaddr_t, vaddr_t);
void uvm_km_pgremove(vaddr_t, vaddr_t);
void uvm_km_pgremove_intrsafe(struct vm_map *, vaddr_t, vaddr_t);
#if defined(DEBUG)
void uvm_km_check_empty(struct vm_map *, vaddr_t, vaddr_t);
#else
#define	uvm_km_check_empty(a, b, c)	/* nothing */
#endif /* defined(DEBUG) */
void uvm_km_va_drain(struct vm_map *, uvm_flag_t);

#endif /* _KERNEL */

#endif /* _UVM_UVM_KM_H_ */
