#include <stdio.h>
#include "debug_line.h"

#ifdef MLD_WITH_LIBDWARF

#include <libdwarf/libdwarf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "memleax.h"
#include "proc_info.h"

struct debug_line_s {
	uintptr_t	address;
	char		*filename;
	int		lineno;
};
static ARRAY(g_debug_lines, struct debug_line_s, 1000);

struct address_region_s {
	uintptr_t	start;
	uintptr_t	end;
};
static ARRAY(g_address_regions, struct address_region_s, 100);


static void debug_line_new(Dwarf_Addr pc, Dwarf_Unsigned lineno, const char *filename)
{
	struct debug_line_s *dl = array_push(&g_debug_lines);
	dl->address = pc;
	dl->lineno = lineno;

	static char *fname_cache1 = "";
	static char *fname_cache2 = "";
	if (strcmp(filename, fname_cache1) == 0) {
		dl->filename = fname_cache1;
	} else if (strcmp(filename, fname_cache2) == 0) {
		dl->filename = fname_cache2;
		char *tmp = fname_cache1;
		fname_cache1 = fname_cache2;
		fname_cache2 = tmp;
	} else {
		dl->filename = strdup(filename);
		fname_cache2 = dl->filename;
	}
}

static int debug_line_build_file(const char *path, size_t start,
		size_t end, int exe_self)
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
		struct address_region_s *ar = array_push(&g_address_regions);
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

void debug_line_build(pid_t pid)
{
	const char *path;
	size_t start, end;
	int exe_self;
	while ((path = proc_maps(pid, &start, &end, &exe_self)) != NULL) {
		try_debug(debug_line_build_file, "debug info",
				path, start, end, exe_self);
	}

	array_sort(&g_debug_lines, debug_line_cmp);
}


const char *debug_line_search(uintptr_t address, int *lineno)
{
	/* check in address region? */
	struct address_region_s *ar;
	array_for_each(ar, &g_address_regions) {
		if (address >= ar->start && address <= ar->end) {
			break;
		}
	}
	if (array_out(ar, &g_address_regions)) {
		return NULL;
	}

	/* search */
	struct debug_line_s *lines = g_debug_lines.data;
	int min = 0, max = g_debug_lines.item_num - 2;
	while (min <= max) {
		int mid = (min + max) / 2;
		struct debug_line_s *dl = &lines[mid];
		struct debug_line_s *next = &lines[mid+1];

		if (address < dl->address) {
			max = mid - 1;
		} else if (address > next->address) {
			min = mid + 1;
		} else {
			*lineno = dl->lineno;
			return dl->filename;
		}
	}

	return NULL;
}

#else /* MLD_WITH_LIBDWARF */
void debug_line_build(pid_t pid)
{
}
const char *debug_line_search(uintptr_t address, int *lineno)
{
	return NULL;
}
#endif /* MLD_WITH_LIBDWARF */
