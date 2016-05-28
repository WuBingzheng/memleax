#ifndef _MLD_BREAKPOINT_
#define _MLD_BREAKPOINT_

#include <sys/types.h>

typedef void (*bp_handler_f) (uintptr_t, uintptr_t, uintptr_t);

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
