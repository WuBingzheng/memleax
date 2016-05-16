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

struct symbol_s {
	uintptr_t	address;
	size_t		size;
	int8_t		weak;
	char		name[47];
};

static ARRAY(g_symbol_table, struct symbol_s, 1000);

static int symbol_cmp(const void *a, const void *b)
{
	const struct symbol_s *sa = a;
	const struct symbol_s *sb = b;
	return sa->address < sb->address ? -1 : 1;
}
int symtab_build(const char *path, uintptr_t start, uintptr_t end, int exe_self)
{
	/* finish */
	if (path == NULL) {
		array_sort(&g_symbol_table, symbol_cmp);
		return 0;
	}

	uintptr_t offset = exe_self ? 0 : start;
	int sh_type = exe_self ? SHT_SYMTAB : SHT_DYNSYM;

	/* open file */
	elf_version(EV_CURRENT);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	Elf *elf = elf_begin(fd, ELF_C_READ, NULL);

	/* find symbol section */
	Elf_Scn* section = NULL;
	Elf64_Shdr *shdr;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		if ((shdr = elf64_getshdr(section)) != NULL
				&& shdr->sh_type == sh_type) {
			break;
		}
	}
	if (section == NULL) {
		return -1;
	}

	/* build symbol table */
	Elf_Data *data = elf_getdata(section, NULL);
	if (data == NULL || data->d_size == 0) {
		return -1;
	}

	int count = 0;
	Elf64_Sym *esym = (Elf64_Sym *)data->d_buf;
	Elf64_Sym *lastsym = (Elf64_Sym *)((char*) data->d_buf + data->d_size);
	for (; esym < lastsym; esym++) {
		if ((esym->st_value == 0) ||
				(ELF64_ST_BIND(esym->st_info) == STB_NUM) ||
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

	/* clean up */
	elf_end(elf);
	close(fd);
	return count;
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
	uintptr_t address = NULL;
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
