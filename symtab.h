#ifndef _MLD_SYMTAB_
#define _MLD_SYMTAB_

#include <stdint.h>
#include <sys/types.h>

void symtab_build(pid_t pid);
const char *symtab_by_address(uintptr_t address, int *offset);
uintptr_t symtab_by_name(const char *name);

#endif
