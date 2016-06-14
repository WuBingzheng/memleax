#include <stdio.h>
#include "debug_line.h"

#ifdef MLX_WITH_LIBDWARF

#include <libdwarf.h>
#include <dwarf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "memleax.h"
#include "proc_info.h"

struct dwarf_die_s {
	Dwarf_Addr	start;
	Dwarf_Addr	end;
	Dwarf_Addr	offset;
	Dwarf_Line	*dw_lines;
	Dwarf_Signed	dw_line_nr;
	int		comp_dir_len;
};
static ARRAY(g_dwarf_dies, struct dwarf_die_s, 1000);

static int debug_line_build_file(const char *path, size_t start,
		size_t end, int exe_self)
{
	uintptr_t offset = exe_self ? 0 : start;

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	Dwarf_Debug dbg;
	Dwarf_Error error;
	int count = 0;
	int res = dwarf_init(fd, DW_DLC_READ, 0, 0, &dbg, &error);
	if(res != DW_DLV_OK) {
		printf("Warning: dwarf_init %s error: %s\n", path, dwarf_errmsg(error));
		return -1;
	}

	Dwarf_Unsigned next_cu_offset = 0;
	while (dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL,
				&next_cu_offset, &error) == DW_DLV_OK) {

		Dwarf_Die cu_die = NULL;
		if (dwarf_siblingof(dbg, NULL, &cu_die, &error) != DW_DLV_OK) {
			printf("Warning: dwarf_siblingof %s error: %s\n", path, dwarf_errmsg(error));
			return -1;
		}

		Dwarf_Signed linecount = 0;
		Dwarf_Line *linebuf = NULL;
		res = dwarf_srclines(cu_die, &linebuf, &linecount, &error);
		if (res == DW_DLV_ERROR) {
			printf("Warning: dwarf_srclines %s error: %s\n", path, dwarf_errmsg(error));
			return -1;
		}
		if (res == DW_DLV_NO_ENTRY) {
			continue;
		}
		if (linecount == 0) {
			continue;
		}

		/* new dwarf-die */
		struct dwarf_die_s *dd = array_push(&g_dwarf_dies);
		dd->dw_lines = linebuf;
		dd->dw_line_nr = linecount;
		dd->offset = offset;
		dwarf_lineaddr(linebuf[0], &dd->start, &error);
		dwarf_lineaddr(linebuf[linecount - 1], &dd->end, &error);
		dd->start += offset;
		dd->end += offset;

		/* get compile directory lenth */
		Dwarf_Attribute comp_dir_attr = 0;
		char *dir = NULL;
		dwarf_attr(cu_die, DW_AT_comp_dir, &comp_dir_attr, &error);
		dwarf_formstring(comp_dir_attr, &dir, &error);
		dd->comp_dir_len = strlen(dir);
		dwarf_dealloc(dbg, comp_dir_attr, DW_DLA_ATTR);

		count++;
	}

	/* dbg, line and die are used later, so do not dealloc them */
	close(fd);
	return count;
}

static int dwarf_die_cmp(const void *a, const void *b)
{
	const struct dwarf_die_s *dda = a;
	const struct dwarf_die_s *ddb = b;
	return dda->start < ddb->start ? -1 : 1;
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

	array_sort(&g_dwarf_dies, dwarf_die_cmp);
}


const char *debug_line_search(uintptr_t address, int *lineno)
{
	/* search dwarf-die */
	int min = 0, max = g_dwarf_dies.item_num - 1;
	struct dwarf_die_s *table = g_dwarf_dies.data;
	struct dwarf_die_s *dd;
	while (min <= max) {
		int mid = (min + max) / 2;
		dd = &table[mid];
		if (address < dd->start) {
			max = mid - 1;
		} else if (address > dd->end) {
			min = mid + 1;
		} else {
			break;
		}
	}
	if (min > max) { /* not found */
		return NULL;
	}

	/* search debug-line from dwarf-die */
	min = 0;
	max = dd->dw_line_nr - 2;
	while (min <= max) {
		int mid = (min + max) / 2;
		Dwarf_Line line1 = dd->dw_lines[mid];
		Dwarf_Line line2 = dd->dw_lines[mid+1];

		Dwarf_Addr addr1, addr2;
		dwarf_lineaddr(line1, &addr1, NULL);
		dwarf_lineaddr(line2, &addr2, NULL);
		addr1 += dd->offset;
		addr2 += dd->offset;

		if (address < addr1) {
			max = mid - 1;
		} else if (address > addr2) {
			min = mid + 1;
		} else {
			Dwarf_Unsigned du_line_no;
			dwarf_lineno(line1, &du_line_no, NULL);
			*lineno = du_line_no;

			char *filename;
			dwarf_linesrc(line1, &filename, NULL);
			return filename + dd->comp_dir_len + 1;
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
