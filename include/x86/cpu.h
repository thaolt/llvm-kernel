/*	$NetBSD: cpu.h,v 1.17 2009/04/30 00:07:23 rmind Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *
 *	@(#)cpu.h	5.4 (Berkeley) 5/9/91
 */

#ifndef _X86_CPU_H_
#define _X86_CPU_H_

#if defined(_KERNEL) || defined(_KMEMUSER)
#if defined(_KERNEL_OPT)
#include "opt_xen.h"
#ifdef i386
#include "opt_user_ldt.h"
#include "opt_vm86.h"
#endif
#endif

/*
 * Definitions unique to x86 cpu support.
 */
#include <machine/frame.h>
#include <machine/segments.h>
#include <machine/tss.h>
#include <machine/intrdefs.h>

#include <x86/cacheinfo.h>
#include <x86/via_padlock.h>

#include <sys/cpu_data.h>
#include <sys/evcnt.h>

struct intrsource;
struct pmap;
struct device;

#ifdef __x86_64__
#define	i386tss	x86_64_tss
#endif

#define	NIOPORTS	1024		/* # of ports we allow to be mapped */
#define	IOMAPSIZE	(NIOPORTS / 8)	/* I/O bitmap size in bytes */

/*
 * a bunch of this belongs in cpuvar.h; move it later..
 */

struct cpu_info {
	struct device *ci_dev;		/* pointer to our device */
	struct cpu_info *ci_self;	/* self-pointer */
	volatile struct vcpu_info *ci_vcpu; /* for XEN */
	void	*ci_tlog_base;		/* Trap log base */
	int32_t ci_tlog_offset;		/* Trap log current offset */

	/*
	 * Will be accessed by other CPUs.
	 */
	struct cpu_info *ci_next;	/* next cpu */
	struct lwp *ci_curlwp;		/* current owner of the processor */
	struct pmap_cpu *ci_pmap_cpu;	/* per-CPU pmap data */
	struct lwp *ci_fpcurlwp;	/* current owner of the FPU */
	int	ci_fpsaving;		/* save in progress */
	int	ci_fpused;		/* XEN: FPU was used by curlwp */
	cpuid_t ci_cpuid;		/* our CPU ID */
	int	ci_cpumask;		/* (1 << CPU ID) */
	uint8_t ci_initapicid;		/* our intitial APIC ID */
	uint8_t ci_packageid;
	uint8_t ci_coreid;
	uint8_t ci_smtid;
	struct cpu_data ci_data;	/* MI per-cpu data */

	/*
	 * Private members.
	 */
	struct evcnt ci_tlb_evcnt;	/* tlb shootdown counter */
	struct pmap *ci_pmap;		/* current pmap */
	int ci_need_tlbwait;		/* need to wait for TLB invalidations */
	int ci_want_pmapload;		/* pmap_load() is needed */
	volatile int ci_tlbstate;	/* one of TLBSTATE_ states. see below */
#define	TLBSTATE_VALID	0	/* all user tlbs are valid */
#define	TLBSTATE_LAZY	1	/* tlbs are valid but won't be kept uptodate */
#define	TLBSTATE_STALE	2	/* we might have stale user tlbs */
	int ci_curldt;		/* current LDT descriptor */
	int ci_nintrhand;	/* number of H/W interrupt handlers */
	uint64_t ci_scratch;

#ifdef XEN
	struct iplsource  *ci_isources[NIPL];
#else
	struct intrsource *ci_isources[MAX_INTR_SOURCES];
#endif
	volatile int	ci_mtx_count;	/* Negative count of spin mutexes */
	volatile int	ci_mtx_oldspl;	/* Old SPL at this ci_idepth */

	/* The following must be aligned for cmpxchg8b. */
	struct {
		uint32_t	ipending;
		int		ilevel;
	} ci_istate __aligned(8);
#define ci_ipending	ci_istate.ipending
#define	ci_ilevel	ci_istate.ilevel

	int		ci_idepth;
	void *		ci_intrstack;
	uint32_t	ci_imask[NIPL];
	uint32_t	ci_iunmask[NIPL];

	uint32_t ci_flags;		/* flags; see below */
	uint32_t ci_ipis;		/* interprocessor interrupts pending */
	int sc_apic_version;		/* local APIC version */

	uint32_t	ci_signature;	 /* X86 cpuid type */
	uint32_t	ci_feature_flags;/* X86 %edx CPUID feature bits */
	uint32_t	ci_feature2_flags;/* X86 %ecx CPUID feature bits */
	uint32_t	ci_feature3_flags;/* X86 extended %edx feature bits */
	uint32_t	ci_feature4_flags;/* X86 extended %ecx feature bits */
	uint32_t	ci_padlock_flags;/* VIA PadLock feature bits */
	uint32_t	ci_vendor[4];	 /* vendor string */
	uint32_t	ci_cpu_serial[3]; /* PIII serial number */
	volatile uint32_t	ci_lapic_counter;

	const struct cpu_functions *ci_func;  /* start/stop functions */
	struct trapframe *ci_ddb_regs;

	u_int ci_cflush_lsize;	/* CFLUSH insn line size */
	struct x86_cache_info ci_cinfo[CAI_COUNT];

	union descriptor *ci_gdt;

#ifdef i386
	struct i386tss	ci_doubleflt_tss;
	struct i386tss	ci_ddbipi_tss;
#endif
	char *ci_doubleflt_stack;
	char *ci_ddbipi_stack;

	struct evcnt ci_ipi_events[X86_NIPI];

	struct via_padlock	ci_vp;	/* VIA PadLock private storage */

	struct i386tss	ci_tss;		/* Per-cpu TSS; shared among LWPs */
	char		ci_iomap[IOMAPSIZE]; /* I/O Bitmap */
	int ci_tss_sel;			/* TSS selector of this cpu */

	/*
	 * The following two are actually region_descriptors,
	 * but that would pollute the namespace.
	 */
	uintptr_t	ci_suspend_gdt;
	uint16_t	ci_suspend_gdt_padding;
	uintptr_t	ci_suspend_idt;
	uint16_t	ci_suspend_idt_padding;

	uint16_t	ci_suspend_tr;
	uint16_t	ci_suspend_ldt;
	uintptr_t	ci_suspend_fs;
	uintptr_t	ci_suspend_gs;
	uintptr_t	ci_suspend_kgs;
	uintptr_t	ci_suspend_efer;
	uintptr_t	ci_suspend_reg[12];
	uintptr_t	ci_suspend_cr0;
	uintptr_t	ci_suspend_cr2;
	uintptr_t	ci_suspend_cr3;
	uintptr_t	ci_suspend_cr4;
	uintptr_t	ci_suspend_cr8;

	/* The following must be in a single cache line. */
	int		ci_want_resched __aligned(64);
	int		ci_padout __aligned(64);
};

/*
 * Macros to handle (some) trapframe registers for common x86 code.
 */
#ifdef __x86_64__
#define	X86_TF_RAX(tf)		tf->tf_rax
#define	X86_TF_RDX(tf)		tf->tf_rdx
#define	X86_TF_RSP(tf)		tf->tf_rsp
#define	X86_TF_RIP(tf)		tf->tf_rip
#define	X86_TF_RFLAGS(tf)	tf->tf_rflags
#else
#define	X86_TF_RAX(tf)		tf->tf_eax
#define	X86_TF_RDX(tf)		tf->tf_edx
#define	X86_TF_RSP(tf)		tf->tf_esp
#define	X86_TF_RIP(tf)		tf->tf_eip
#define	X86_TF_RFLAGS(tf)	tf->tf_eflags
#endif

/*
 * Processor flag notes: The "primary" CPU has certain MI-defined
 * roles (mostly relating to hardclock handling); we distinguish
 * betwen the processor which booted us, and the processor currently
 * holding the "primary" role just to give us the flexibility later to
 * change primaries should we be sufficiently twisted.
 */

#define	CPUF_BSP	0x0001		/* CPU is the original BSP */
#define	CPUF_AP		0x0002		/* CPU is an AP */
#define	CPUF_SP		0x0004		/* CPU is only processor */
#define	CPUF_PRIMARY	0x0008		/* CPU is active primary processor */

#define	CPUF_SYNCTSC	0x0800		/* Synchronize TSC */
#define	CPUF_PRESENT	0x1000		/* CPU is present */
#define	CPUF_RUNNING	0x2000		/* CPU is running */
#define	CPUF_PAUSE	0x4000		/* CPU is paused in DDB */
#define	CPUF_GO		0x8000		/* CPU should start running */

/*
 * We statically allocate the CPU info for the primary CPU (or,
 * the only CPU on uniprocessors), and the primary CPU is the
 * first CPU on the CPU info list.
 */
extern struct cpu_info cpu_info_primary;
extern struct cpu_info *cpu_info_list;

#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	cii = 0, ci = cpu_info_list; \
					ci != NULL; ci = ci->ci_next

#define CPU_STARTUP(_ci, _target)	((_ci)->ci_func->start(_ci, _target))
#define CPU_STOP(_ci)	        	((_ci)->ci_func->stop(_ci))
#define CPU_START_CLEANUP(_ci)		((_ci)->ci_func->cleanup(_ci))

#if !defined(__GNUC__) || defined(_MODULE)
/* For non-GCC and modules */
struct cpu_info	*x86_curcpu(void);
void	cpu_set_curpri(int);
# ifdef __GNUC__
lwp_t	*x86_curlwp(void) __attribute__ ((const));
# else
lwp_t   *x86_curlwp(void);
# endif
#endif

#define cpu_number() 		(cpu_index(curcpu()))

#define CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

#define	X86_AST_GENERIC		0x01
#define	X86_AST_PREEMPT		0x02

#define aston(l, why)		((l)->l_md.md_astpending |= (why))
#define	cpu_did_resched(l)	((l)->l_md.md_astpending &= ~X86_AST_PREEMPT)

void cpu_boot_secondary_processors(void);
void cpu_init_idle_lwps(void);
void cpu_init_msrs(struct cpu_info *, bool);

extern uint32_t cpus_attached;
#ifndef XEN
#define	curcpu()		x86_curcpu()
#define	curlwp			x86_curlwp()
#else
/* XXX initgdt() calls pmap_kenter_pa() which calls splvm() before %fs is set */
#define curcpu()		(&cpu_info_primary)
#define curlwp			curcpu()->ci_curlwp
#endif
#define	curpcb			(&curlwp->l_addr->u_pcb)

/*
 * Arguments to hardclock, softclock and statclock
 * encapsulate the previous machine state in an opaque
 * clockframe; for now, use generic intrframe.
 */
struct clockframe {
	struct intrframe cf_if;
};

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the i386, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
extern void	cpu_need_proftick(struct lwp *l);

/*
 * Notify the LWP l that it has a signal pending, process as soon as
 * possible.
 */
extern void	cpu_signotify(struct lwp *);

/*
 * We need a machine-independent name for this.
 */
extern void (*delay_func)(unsigned int);
struct timeval;

#define	DELAY(x)		(*delay_func)(x)
#define delay(x)		(*delay_func)(x)

extern int biosbasemem;
extern int biosextmem;
extern unsigned int cpu_feature;
extern unsigned int cpu_feature2;
extern unsigned int cpu_feature_padlock;
extern int cpu;
extern int cpuid_level;
extern int cpu_class;
extern char cpu_brand_string[];

extern int i386_use_fxsave;
extern int i386_has_sse;
extern int i386_has_sse2;

extern void (*x86_cpu_idle)(void);
#define	cpu_idle() (*x86_cpu_idle)()

/* machdep.c */
void	dumpconf(void);
void	cpu_reset(void);
void	i386_proc0_tss_ldt_init(void);
void	dumpconf(void);
void	cpu_reset(void);
void	x86_64_proc0_tss_ldt_init(void);
void	x86_64_init_pcb_tss_ldt(struct cpu_info *);

/* longrun.c */
u_int 	tmx86_get_longrun_mode(void);
void 	tmx86_get_longrun_status(u_int *, u_int *, u_int *);
void 	tmx86_init_longrun(void);

/* identcpu.c */
void 	cpu_probe(struct cpu_info *);
void	cpu_identify(struct cpu_info *);

/* cpu_topology.c */
void	x86_cpu_toplogy(struct cpu_info *);

/* vm_machdep.c */
void	cpu_proc_fork(struct proc *, struct proc *);

/* locore.s */
struct region_descriptor;
void	lgdt(struct region_descriptor *);
#ifdef XEN
void	lgdt_finish(void);
void	i386_switch_context(lwp_t *);
#endif

struct pcb;
void	savectx(struct pcb *);
void	lwp_trampoline(void);
void	child_trampoline(void);
#ifdef XEN
void	startrtclock(void);
void	xen_delay(unsigned int);
void	xen_initclocks(void);
#else
/* clock.c */
void	initrtclock(u_long);
void	startrtclock(void);
void	i8254_delay(unsigned int);
void	i8254_microtime(struct timeval *);
void	i8254_initclocks(void);
#endif

/* cpu.c */

void	cpu_probe_features(struct cpu_info *);

/* npx.c */
void	npxsave_lwp(struct lwp *, bool);
void	npxsave_cpu(bool);

/* vm_machdep.c */
paddr_t	kvtop(void *);

#ifdef USER_LDT
/* sys_machdep.h */
int	x86_get_ldt(struct lwp *, void *, register_t *);
int	x86_set_ldt(struct lwp *, void *, register_t *);
#endif

/* isa_machdep.c */
void	isa_defaultirq(void);
int	isa_nmi(void);

#ifdef VM86
/* vm86.c */
void	vm86_gpfault(struct lwp *, int);
#endif /* VM86 */

/* consinit.c */
void kgdb_port_init(void);

/* bus_machdep.c */
void x86_bus_space_init(void);
void x86_bus_space_mallocok(void);

#include <machine/psl.h>	/* Must be after struct cpu_info declaration */

#endif /* _KERNEL || __KMEMUSER */

#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/types.h>
#else
#include <stdbool.h>
#endif /* _KERNEL || _STANDALONE */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_BIOSBASEMEM		2	/* int: bios-reported base mem (K) */
#define	CPU_BIOSEXTMEM		3	/* int: bios-reported ext. mem (K) */
/* 	CPU_NKPDE		4	obsolete: int: number of kernel PDEs */
#define	CPU_BOOTED_KERNEL	5	/* string: booted kernel name */
#define CPU_DISKINFO		6	/* struct disklist *:
					 * disk geometry information */
#define CPU_FPU_PRESENT		7	/* int: FPU is present */
#define	CPU_OSFXSR		8	/* int: OS uses FXSAVE/FXRSTOR */
#define	CPU_SSE			9	/* int: OS/CPU supports SSE */
#define	CPU_SSE2		10	/* int: OS/CPU supports SSE2 */
#define	CPU_TMLR_MODE		11	/* int: longrun mode
					 * 0: minimum frequency
					 * 1: economy
					 * 2: performance
					 * 3: maximum frequency
					 */
#define	CPU_TMLR_FREQUENCY	12	/* int: current frequency */
#define	CPU_TMLR_VOLTAGE	13	/* int: curret voltage */
#define	CPU_TMLR_PERCENTAGE	14	/* int: current clock percentage */
#define	CPU_MAXID		15	/* number of valid machdep ids */

/*
 * Structure for CPU_DISKINFO sysctl call.
 * XXX this should be somewhere else.
 */
#define MAX_BIOSDISKS	16

struct disklist {
	int dl_nbiosdisks;			   /* number of bios disks */
	struct biosdisk_info {
		int bi_dev;			   /* BIOS device # (0x80 ..) */
		int bi_cyl;			   /* cylinders on disk */
		int bi_head;			   /* heads per track */
		int bi_sec;			   /* sectors per track */
		uint64_t bi_lbasecs;		   /* total sec. (iff ext13) */
#define BIFLAG_INVALID		0x01
#define BIFLAG_EXTINT13		0x02
		int bi_flags;
	} dl_biosdisks[MAX_BIOSDISKS];

	int dl_nnativedisks;			   /* number of native disks */
	struct nativedisk_info {
		char ni_devname[16];		   /* native device name */
		int ni_nmatches; 		   /* # of matches w/ BIOS */
		int ni_biosmatches[MAX_BIOSDISKS]; /* indices in dl_biosdisks */
	} dl_nativedisks[1];			   /* actually longer */
};
#endif /* !_X86_CPU_H_ */
