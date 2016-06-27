/*
 * build symbol table of ELF
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#include <sys/ptrace.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <libelf.h>
#include <libunwind-ptrace.h>
#include <string.h>

#include "array.h"
#include "memleax.h"
#include "proc_info.h"
#include "debug_file.h"

struct symbol_s {
	uintptr_t	address;
	size_t		size;
	int8_t		weak;
	char		name[47];
};

static ARRAY(g_symbol_table, struct symbol_s, 1000);

static int symtab_build_section(Elf *elf, Elf_Scn *section, uintptr_t offset)
{
	Elf64_Shdr *shdr = elf64_getshdr(section);
	if (shdr == NULL) {
		return 0;
	}

	if (shdr->sh_type != SHT_SYMTAB && shdr->sh_type != SHT_DYNSYM) {
		return 0;
	}

	Elf_Data *data = elf_getdata(section, NULL);
	if (data == NULL || data->d_size == 0) {
		return 0;
	}

	int count = 0;
	Elf64_Sym *esym = (Elf64_Sym *)data->d_buf;
	Elf64_Sym *lastsym = (Elf64_Sym *)((char*) data->d_buf + data->d_size);
	for (; esym < lastsym; esym++) {
		if ((esym->st_value == 0) || (esym->st_size == 0) ||
				(esym->st_shndx == SHN_UNDEF) ||
#ifdef STB_NUM
				(ELF64_ST_BIND(esym->st_info) == STB_NUM) ||
#endif
				(ELF64_ST_TYPE(esym->st_info) != STT_FUNC)) {
			continue;
		}

		struct symbol_s *sym = array_push(&g_symbol_table);

		char *name = elf_strptr(elf, shdr->sh_link, (size_t)esym->st_name);
		strncpy(sym->name, name, sizeof(sym->name));
		sym->name[sizeof(sym->name) - 1] = '\0';

		sym->address = esym->st_value + offset;
		sym->size = esym->st_size;
		sym->weak = (ELF64_ST_BIND(esym->st_info) == STB_WEAK);

		count++;
	}
	return count;
}

static int symtab_build_file(const char *path, uintptr_t start, uintptr_t end)
{
	/* open file */
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	elf_version(EV_CURRENT);
	Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		close(fd);
		return -1;
	}

	/* get offset */
	Elf64_Ehdr *hdr = elf64_getehdr(elf);
	uintptr_t offset = hdr->e_type == ET_EXEC ? 0 : start;

	/* find symbol section */
	Elf_Scn* section = NULL;
	int count = 0;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		count += symtab_build_section(elf, section, offset);
	}

	/* clean up */
	elf_end(elf);
	close(fd);
	return count;
}

static int symbol_cmp(const void *a, const void *b)
{
	const struct symbol_s *sa = a;
	const struct symbol_s *sb = b;
	return sa->address < sb->address ? -1 : 1;
}

void symtab_build(pid_t pid)
{
	const char *path, *debugp;
	size_t start, end;
	int exe_self;
	while ((path = proc_maps(pid, &start, &end, &exe_self)) != NULL) {
		debug_try_init(path, exe_self);
		while ((debugp = debug_try_get()) != NULL) {
			if (symtab_build_file(debugp, start, end) > 0) {
				break;
			}
		}
		if (exe_self && debugp == NULL) {
			printf("Warning: no symbol table found for %s\n", path);
		}
	}

	/* finish */
	array_sort(&g_symbol_table, symbol_cmp);
}

const char *symtab_by_address(uintptr_t address, int *offset)
{
	int min = 0, max = g_symbol_table.item_num - 1;
	struct symbol_s *table = g_symbol_table.data;
	while (min <= max) {
		int mid = (min + max) / 2;
		struct symbol_s *sym = &table[mid];
		if (address < sym->address) {
			max = mid - 1;
		} else if (address > sym->address + sym->size) {
			min = mid + 1;
		} else {
			*offset = address - sym->address;
			return sym->name;
		}
	}
	return NULL;
}

uintptr_t symtab_by_name(const char *name)
{
	uintptr_t address = 0;
	struct symbol_s *sym;
	array_for_each(sym, &g_symbol_table) {
		if (strcmp(sym->name, name) == 0) {
			if (!sym->weak) {
				return sym->address;
			}
			if (address == 0) {
				address = sym->address;
			}
		}
	}
	return address;
}
