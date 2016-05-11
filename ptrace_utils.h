#ifndef _MLD_PTRACE_UTILS_
#define _MLD_PTRACE_UTILS_

#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/user.h>

#include "memleax.h"

static inline intptr_t ptrace_get_data(intptr_t address)
{
	return ptrace(PTRACE_PEEKTEXT, g_target_pid, address, 0);
}
static inline void ptrace_set_data(intptr_t address, intptr_t data)
{
	ptrace(PTRACE_POKETEXT, g_target_pid, address, data);
}
static inline void ptrace_set_int3(intptr_t address, intptr_t code)
{
	ptrace_set_data(address, (code & 0xFFFFFFFFFFFFFF00UL) | 0xCC);
}
static inline void ptrace_get_regs(struct user_regs_struct *regs)
{
	ptrace(PTRACE_GETREGS, g_target_pid, 0, regs);
}
static inline void ptrace_set_regs(struct user_regs_struct *regs)
{
	ptrace(PTRACE_SETREGS, g_target_pid, 0, regs);
}

static inline void ptrace_continue(int signum)
{
	ptrace(PTRACE_CONT, g_target_pid, 0, signum);
}

static inline void ptrace_attach(void)
{
	if (ptrace(PTRACE_ATTACH, g_target_pid, 0, 0) != 0) {
		perror("attach process error:");
		exit(4);
	}
	// ptrace(PTRACE_SETOPTIONS, g_target_pid, 0, PTRACE_O_TRACEEXIT); // TODO
}
static inline void ptrace_detach(int signum)
{
	ptrace(PTRACE_DETACH, g_target_pid, 0, signum);
}
#endif
