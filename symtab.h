#ifndef MLD_SYMTAB_H
#define MLD_SYMTAB_H

#include <stdint.h>
#include <sys/types.h>

void symtab_build(pid_t pid);
const char *symtab_by_address(uintptr_t address, int *offset);
uintptr_t symtab_by_name(const char *name);

#endif
