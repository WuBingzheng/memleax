#ifndef MLD_BREAKPOINT_H
#define MLD_BREAKPOINT_H

#include <sys/types.h>

typedef int (*bp_handler_f) (uintptr_t, uintptr_t, uintptr_t);

struct breakpoint_s {
	const char	*name;
	bp_handler_f	handler;

	uintptr_t	entry_address;
	uintptr_t	entry_code;
};

void breakpoint_init(pid_t pid);
void breakpoint_cleanup(pid_t pid);
struct breakpoint_s *breakpoint_by_entry(uintptr_t address);

#endif
