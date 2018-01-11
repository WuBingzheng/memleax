#ifndef MLD_SYMTAB_H
#define MLD_SYMTAB_H

#include <stdint.h>
#include <sys/types.h>

/**
 * @brief symtab_build
 * @param pid
 */
void symtab_build(pid_t pid);

/**
 * @brief symtab_by_address
 * @param address
 * @param offset
 * @return
 */
const char *symtab_by_address(uintptr_t address, int *offset);

/**
 * @brief symtab_by_name
 * @param name
 * @return
 */
uintptr_t symtab_by_name(const char *name);

#endif
