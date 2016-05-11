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

#include "memleax.h"


/* global symblo-table */
#define SYMBOL_MAX 100000
static int g_symtab_num = 0;
static struct symbol_s {
       intptr_t        address;
       size_t          size;
       int             weak;
       const char      *name;
} g_symbol_table[SYMBOL_MAX];


static void symbol_new(const char *name, intptr_t address, size_t size, int weak)
{
	static char sym_name_buf[SYMBOL_MAX * 20];
	static char *sym_name_pos = sym_name_buf;

	if (g_symtab_num == SYMBOL_MAX) {
		printf("error: too many symbols in symbol-table\n");
		exit(3);
	}

	struct symbol_s *sym = &g_symbol_table[g_symtab_num++];
	sym->address = address;
	sym->size = size;
	sym->weak = weak;
	sym->name = sym_name_pos;

	strcpy(sym_name_pos, name);
	sym_name_pos += strlen(name) + 1;
}

int symtab_build(const char *path, intptr_t start, intptr_t end, int exe_self)
{
	intptr_t offset = exe_self ? 0 : start;
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

		char *name = elf_strptr(elf, shdr->sh_link, (size_t)esym->st_name);
		symbol_new(name, esym->st_value + offset, esym->st_size,
				ELF64_ST_BIND(esym->st_info) == STB_WEAK);
		count++;
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
void symtab_build_finish(void)
{
	qsort(g_symbol_table, g_symtab_num, sizeof(struct symbol_s), symbol_cmp);
}

const char *symtab_by_address(intptr_t address, int *offset)
{
	int min = 0, max = g_symtab_num - 1;
	while (min <= max) {
		int mid = (min + max) / 2;
		struct symbol_s *sym = &g_symbol_table[mid];
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

intptr_t symtab_by_name(const char *name)
{
	int i;
	intptr_t address = NULL;
	for (i = 0; i < g_symtab_num; i++) {
		if (strcmp(g_symbol_table[i].name, name) == 0) {
			if (!g_symbol_table[i].weak) {
				return g_symbol_table[i].address;
			}
			if (address == 0) {
				address = g_symbol_table[i].address;
			}
		}
	}
	return address;
}
