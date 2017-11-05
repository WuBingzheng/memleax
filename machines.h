#ifndef MLX_MACHINES_H
#define MLX_MACHINES_H

#include "ptrace_utils.h"

/* We use the names in Linux, and define the aliases for FreeBSD. */
#ifdef MLX_FREEBSD
#define esp r_esp
#define eax r_eax
#define esi r_esi
#define edi r_edi
#define eip r_eip

#define rsp r_rsp
#define rax r_rax
#define rsi r_rsi
#define rdi r_rdi
#define rip r_rip
#endif


#if defined(MLX_X86)
static inline uintptr_t call_return_address(pid_t pid, registers_info_t *regs)
{
	return ptrace_get_data(pid, regs->esp);
}
static inline uintptr_t call_return_value(registers_info_t *regs)
{
	return regs->eax;
}
static inline uintptr_t call_arg1(pid_t pid, registers_info_t *regs)
{
	return ptrace_get_data(pid, regs->esp + 4);
}
static inline uintptr_t call_arg2(pid_t pid, registers_info_t *regs)
{
	return ptrace_get_data(pid, regs->esp + 8);
}
static inline uintptr_t pc_unwind(pid_t pid, registers_info_t *regs)
{
	regs->eip--;
	ptrace_set_regs(pid, regs);
	return regs->eip;
}
static inline void set_breakpoint(pid_t pid, uintptr_t address, uintptr_t code)
{
	ptrace_set_data(pid, address, (code & 0xFFFFFF00U) | 0xCC);
}
static inline int is_breakpoint(pid_t pid, uintptr_t address)
{
	return (ptrace_get_data(pid, address) & 0xFF) == 0xCC;
}

#elif defined(MLX_X86_64)
static inline uintptr_t call_return_address(pid_t pid, registers_info_t *regs)
{
	return ptrace_get_data(pid, regs->rsp);
}
static inline uintptr_t call_return_value(registers_info_t *regs)
{
	return regs->rax;
}
static inline uintptr_t call_arg1(pid_t pid, registers_info_t *regs)
{
	return regs->rdi;
}
static inline uintptr_t call_arg2(pid_t pid, registers_info_t *regs)
{
	return regs->rsi;
}
static inline uintptr_t pc_unwind(pid_t pid, registers_info_t *regs)
{
	regs->rip--;
	ptrace_set_regs(pid, regs);
	return regs->rip;
}
static inline void set_breakpoint(pid_t pid, uintptr_t address, uintptr_t code)
{
  #ifdef MLX_LINUX
	ptrace_set_data(pid, address, (code & 0xFFFFFFFFFFFFFF00UL) | 0xCC);
  #else // MLX_FREEBSD
	ptrace_set_data(pid, address, (code & 0xFFFFFF00U) | 0xCC);
  #endif
}
static inline int is_breakpoint(pid_t pid, uintptr_t address)
{
	return (ptrace_get_data(pid, address) & 0xFF) == 0xCC;
}

#elif defined(MLX_ARMv7)
static inline uintptr_t call_return_address(pid_t pid, registers_info_t *regs)
{
	return regs->uregs[14];
}
static inline uintptr_t call_return_value(registers_info_t *regs)
{
	return regs->uregs[0];
}
static inline uintptr_t call_arg1(pid_t pid, registers_info_t *regs)
{
	return regs->uregs[0];
}
static inline uintptr_t call_arg2(pid_t pid, registers_info_t *regs)
{
	return regs->uregs[1];
}
static inline uintptr_t pc_unwind(pid_t pid, registers_info_t *regs)
{
	return regs->uregs[15];
}
static inline void set_breakpoint(pid_t pid, uintptr_t address, uintptr_t code)
{
	ptrace_set_data(pid, address, 0xE7F001F0);
}
static inline int is_breakpoint(pid_t pid, uintptr_t address)
{
	return ptrace_get_data(pid, address) == 0xE7F001F0;
}

#elif defined(MLX_AARCH64)
static inline uintptr_t call_return_address(pid_t pid, registers_info_t *regs)
{
	return regs->regs[30];
}
static inline uintptr_t call_return_value(registers_info_t *regs)
{
	return regs->regs[0];
}
static inline uintptr_t call_arg1(pid_t pid, registers_info_t *regs)
{
	return regs->regs[0];
}
static inline uintptr_t call_arg2(pid_t pid, registers_info_t *regs)
{
	return regs->regs[1];
}
static inline uintptr_t pc_unwind(pid_t pid, registers_info_t *regs)
{
	return regs->pc;
}
static inline void set_breakpoint(pid_t pid, uintptr_t address, uintptr_t code)
{
	ptrace_set_data(pid, address, 0xd4200000);
}
static inline int is_breakpoint(pid_t pid, uintptr_t address)
{
	return ptrace_get_data(pid, address) == 0xd4200000;
}
#endif

#endif
