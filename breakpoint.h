#ifndef _MLD_BREAKPOINT_
#define _MLD_BREAKPOINT_

typedef void (*bp_handler_f) (intptr_t, intptr_t, intptr_t);

struct breakpoint_s {
	const char	*name;
	bp_handler_f	handler;

	intptr_t	entry_address;
	intptr_t	entry_code;
};

void breakpoint_init(void);
void breakpoint_cleanup(void);
struct breakpoint_s *breakpoint_by_entry(intptr_t address);

#endif
