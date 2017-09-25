#ifndef MLX_MACHINES_H
#define MLX_MACHINES_H

#include "ptrace_utils.h"

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
#elif defined(MLX_ARM)

#elif defined(MLX_ARM64)
#endif

#endif
