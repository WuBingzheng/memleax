/*
 * swappers of ptrace, on GNU/Linux and FreeBSD
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#ifndef MLD_PTRACE_UTILS_H
#define MLD_PTRACE_UTILS_H

#include <sys/ptrace.h>
#include <stdio.h>

#ifdef MLX_LINUX
#include <sys/user.h>

typedef struct user_regs_struct registers_info_t;
#define REG_RAX(ri) (ri).rax
#define REG_RIP(ri) (ri).rip
#define REG_RSI(ri) (ri).rsi
#define REG_RDI(ri) (ri).rdi
#define REG_RSP(ri) (ri).rsp

static inline void ptrace_get_regs(pid_t pid, struct user_regs_struct *regs)
{
	ptrace(PTRACE_GETREGS, pid, 0, regs);
}
static inline void ptrace_set_regs(pid_t pid, struct user_regs_struct *regs)
{
	ptrace(PTRACE_SETREGS, pid, 0, regs);
}

static inline uintptr_t ptrace_get_data(pid_t pid, uintptr_t address)
{
	return ptrace(PTRACE_PEEKTEXT, pid, address, 0);
}
static inline void ptrace_set_data(pid_t pid, uintptr_t address, uintptr_t data)
{
	ptrace(PTRACE_POKETEXT, pid, address, data);
}
static inline void ptrace_set_int3(pid_t pid, uintptr_t address, uintptr_t code)
{
	ptrace_set_data(pid, address, (code & 0xFFFFFFFFFFFFFF00UL) | 0xCC);
}

static inline uintptr_t ptrace_get_child(pid_t pid)
{
	uintptr_t child;
	ptrace(PTRACE_GETEVENTMSG, pid, 0, &child);
	return child;
}
static inline int ptrace_new_child(pid_t pid, int status)
{
	return (status >> 16);
}
static inline void ptrace_continue(pid_t pid, int signum)
{
	ptrace(PTRACE_CONT, pid, 0, signum);
}
static inline void ptrace_attach(pid_t pid)
{
	if (ptrace(PTRACE_ATTACH, pid, 0, 0) != 0) {
		perror("== Attach process error:");
		exit(4);
	}
}
static inline void ptrace_trace_child(pid_t pid)
{
	ptrace(PTRACE_SETOPTIONS, pid, 0,
			PTRACE_O_TRACECLONE |
			PTRACE_O_TRACEVFORK |
			PTRACE_O_TRACEFORK);
}
static inline void ptrace_detach(pid_t pid, int signum)
{
	ptrace(PTRACE_DETACH, pid, 0, signum);
}
#endif /* MLX_LINUX */


#ifdef MLX_FREEBSD
#include <machine/reg.h>

typedef struct reg registers_info_t;
#define REG_RAX(ri) (ri).r_rax
#define REG_RIP(ri) (ri).r_rip
#define REG_RSI(ri) (ri).r_rsi
#define REG_RDI(ri) (ri).r_rdi
#define REG_RSP(ri) (ri).r_rsp

static inline void ptrace_get_regs(pid_t pid, struct reg *regs)
{
	ptrace(PT_GETREGS, pid, (caddr_t)regs, 0);
}
static inline void ptrace_set_regs(pid_t pid, struct reg *regs)
{
	ptrace(PT_SETREGS, pid, (caddr_t)regs, 0);
}

static inline int ptrace_get_data(pid_t pid, uintptr_t address)
{
	return ptrace(PT_READ_I, pid, (caddr_t)address, 0);
}
static inline void ptrace_set_data(pid_t pid, uintptr_t address, int data)
{
	ptrace(PT_WRITE_I, pid, (caddr_t)address, data);
}
static inline void ptrace_set_int3(pid_t pid, uintptr_t address, int code)
{
	ptrace_set_data(pid, address, (code & 0xFFFFFF00U) | 0xCC);
}
static inline void ptrace_continue(pid_t pid, int signum)
{
	ptrace(PT_CONTINUE, pid, (caddr_t)1, signum);
}
static inline void ptrace_attach(pid_t pid)
{
	if (ptrace(PT_ATTACH, pid, 0, 0) != 0) {
		perror("== Attach process error:");
		exit(4);
	}
}
static inline void ptrace_trace_child(pid_t pid)
{
	ptrace(PT_FOLLOW_FORK, pid, 0, 1);
}
static inline void ptrace_detach(pid_t pid, int signum)
{
	ptrace(PT_DETACH, pid, 0, signum);
}

static inline int ptrace_new_child(pid_t pid, int status)
{
	struct ptrace_lwpinfo pi;
	ptrace(PT_LWPINFO, pid, (caddr_t)&pi, sizeof(pi));
	return (pi.pl_flags & PL_FLAG_FORKED);
}
#endif /* MLX_FREEBSD */

#endif
