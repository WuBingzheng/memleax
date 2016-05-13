#include <stdio.h>
#include <libdwarf/libdwarf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "memleax.h"

#define DEBUGLINE_MAX 500000
static int g_debug_line_num = 0;
static struct debug_line_s {
	uintptr_t	address;
	int		lineno;
	int		fileno;
} g_debug_lines[DEBUGLINE_MAX];

#define DEBUGFILES_MAX 5000
static char *g_filenames[DEBUGFILES_MAX];
static int g_filename_num = 0;

#define DEBUGREGION_MAX 100
static int g_address_region_num = 0;
static struct address_region_s {
	uintptr_t	start, end;
} g_address_regions[DEBUGREGION_MAX];


static void debug_line_new(Dwarf_Addr pc, Dwarf_Unsigned lineno, const char *filename)
{
	if (g_debug_line_num == DEBUGLINE_MAX) {
		printf("error: too many debug-line entry\n");
		exit(3);
	}
	if (g_filename_num == DEBUGFILES_MAX) {
		printf("error: too many files in debug-line\n");
		exit(3);
	}

	int i = g_filename_num - 1;
	while (i >= 0) {
		if (strcmp(filename, g_filenames[i]) == 0) {
			break;
		}
		i--;
	}
	if (i < 0) {
		i = g_filename_num++;
		g_filenames[i] = malloc(strlen(filename) + 1);
		strcpy(g_filenames[i], filename);
	}

	struct debug_line_s *dl = &g_debug_lines[g_debug_line_num++];
	dl->address = pc;
	dl->lineno = lineno;
	dl->fileno = i;
}

int debug_line_build(const char *path, uintptr_t start, uintptr_t end, int exe_self)
{
	uintptr_t offset = exe_self ? 0 : start;

	Dwarf_Debug dbg;
	Dwarf_Error error;
	int count = 0;

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	int res = dwarf_init(fd, DW_DLC_READ, 0, 0, &dbg, &error);
	if(res != DW_DLV_OK) {
		return -1;
	}

	Dwarf_Unsigned next_cu_offset = 0;
	while (dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL,
				&next_cu_offset, &error) == DW_DLV_OK) {

		Dwarf_Die cu_die = 0;
		if (dwarf_siblingof(dbg, NULL, &cu_die, &error) != DW_DLV_OK) {
			printf("dwarf_siblingof\n");
			exit(1);
		}

		Dwarf_Signed linecount = 0;
		Dwarf_Line *linebuf = NULL;
		res = dwarf_srclines(cu_die, &linebuf, &linecount, &error);
		if (res == DW_DLV_ERROR) {
			printf("warning: dwarf_srclines() error: %s\n", dwarf_errmsg(error));
			break;
		}
		if (res == DW_DLV_NO_ENTRY) {
			continue;
		}

		Dwarf_Addr pc;
		char *filename;
		Dwarf_Unsigned lineno;
		int i;
		for (i = 0; i < linecount; i++) {
			Dwarf_Line line = linebuf[i];
			res = dwarf_linesrc(line, &filename, &error);
			res = dwarf_lineaddr(line, &pc, &error);
			res = dwarf_lineno(line, &lineno, &error);
			debug_line_new(pc + offset, lineno, filename);
			count++;
		}

		dwarf_srclines_dealloc(dbg, linebuf, linecount);
		dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
	}

	dwarf_finish(dbg,&error);
	close(fd);

	if (count > 0) {
		struct address_region_s *ar = &g_address_regions[g_address_region_num++];
		ar->start = start;
		ar->end = end;
	}

	return count;
}

static int debug_line_cmp(const void *a, const void *b)
{
	const struct debug_line_s *dla = a;
	const struct debug_line_s *dlb = b;
	return dla->address < dlb->address ? -1 : 1;
}
void debug_line_build_finish(void)
{
	qsort(g_debug_lines, g_debug_line_num,
			sizeof(struct debug_line_s), debug_line_cmp);
}

const char *debug_line_search(uintptr_t address, int *lineno)
{
	/* check in address region? */
	int i;
	for (i = 0; i < g_address_region_num; i++) {
		struct address_region_s *ar = &g_address_regions[i];
		if (address >= ar->start && address <= ar->end) {
			break;
		}
	}
	if (i == g_address_region_num) {
		return NULL;
	}

	/* search */
	int min = 0, max = g_debug_line_num - 2;
	while (min <= max) {
		int mid = (min + max) / 2;
		struct debug_line_s *dl = &g_debug_lines[mid];
		struct debug_line_s *next = &g_debug_lines[mid+1];

		if (address < dl->address) {
			max = mid - 1;
		} else if (address > next->address) {
			min = mid + 1;
		} else {
			*lineno = dl->lineno;
			return g_filenames[dl->fileno];
		}
	}

	return NULL;
}
