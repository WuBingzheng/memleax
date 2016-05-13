#ifndef _MLD_SYMTAB_
#define _MLD_SYMTAB_

#include <stdint.h>

int symtab_build(const char *path, uintptr_t start, uintptr_t end, int exe_self);
void symtab_build_finish(void);

const char *symtab_by_address(uintptr_t address, int *offset);
uintptr_t symtab_by_name(const char *name);

#endif
