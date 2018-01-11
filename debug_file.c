/*
 * try and validate debug-info files
 *
 * Author: Wu Bingzheng
 *   Date: 2016-6
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <gelf.h>
#include <string.h>

#include "debug_file.h"
#include "memleax.h"

/**
 * @brief gnu_debuglink_crc32
 * @param crc
 * @param buf
 * @param len
 * @return
 */
static uint32_t gnu_debuglink_crc32 (uint32_t crc, unsigned char *buf, size_t len)
{
	static const uint32_t crc32_table[256] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
		0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
		0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
		0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
		0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
		0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
		0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
		0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
		0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
		0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
		0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
		0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
		0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
		0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
		0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
		0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
		0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
		0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
		0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
		0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
		0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
		0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
		0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
		0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
		0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
		0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
		0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
		0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
		0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
		0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
		0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
		0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
		0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
		0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
		0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
		0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
		0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
		0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
		0x2d02ef8d
	};
	unsigned char *end;

	crc = ~crc & 0xffffffff;
	for (end = buf + len; buf < end; ++buf)
		crc = crc32_table[(crc ^ *buf) & 0xff] ^ (crc >> 8);
	return ~crc & 0xffffffff;
}

/**
 * @brief file_crc32
 * @param fd
 * @return
 */
static uint32_t file_crc32(int fd)
{
	int rlen;
	uint32_t crc32 = 0;
	unsigned char buffer[4096];
	lseek(fd, 0, SEEK_SET);
	while ((rlen = read(fd, buffer, sizeof(buffer))) > 0) {
		crc32 = gnu_debuglink_crc32(crc32, buffer, rlen);
	}
	return crc32;
}

/**
 * @brief elf_section_data
 * @param fd
 * @param name
 * @param out_buf
 * @param out_size
 * @return
 */
static int elf_section_data(int fd, const char *name, uint8_t *out_buf, int out_size)
{
	elf_version(EV_CURRENT);
	Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		return -1;
	}

	GElf_Ehdr elfhdr;
	if (gelf_getehdr(elf, &elfhdr) == NULL) {
		return -1;
	}
	Elf_Scn *strtab_sec = elf_getscn(elf, elfhdr.e_shstrndx);
	Elf_Data *strtab_data = elf_getdata(strtab_sec, NULL);
	const char *strings = strtab_data->d_buf;

	Elf_Scn* section = NULL;
	int ret = -1;
	while ((section = elf_nextscn(elf, section)) != NULL) {
		GElf_Shdr shdr;
		if (gelf_getshdr(section, &shdr) == NULL) {
			break;
		}

		if (strcmp(strings + shdr.sh_name, name) == 0) {
			Elf_Data *sdata = elf_getdata(section, NULL);
			if (sdata->d_size > out_size) {
				break;
			}
			memcpy(out_buf, sdata->d_buf, sdata->d_size);
			ret = sdata->d_size;
			break;
		}
	}

	elf_end(elf);
	return ret;
}

/**
 * @brief elf_build_id
 * @param fd
 * @param build_id
 * @return
 */
static int elf_build_id(int fd, uint8_t *build_id)
{
	uint8_t buffer[1000];
	if (elf_section_data(fd, ".note.gnu.build-id", buffer, sizeof(buffer)) < 0) {
		return 0;
	}
	int buildid_len = *(uint32_t *)(buffer + 4);
	memcpy(build_id, buffer + 16, buildid_len);
	return buildid_len;
}

/**
 * @brief elf_debug_link
 * @param fd
 * @param fname
 * @return
 */
static uint32_t elf_debug_link(int fd, char *fname)
{
	uint8_t buffer[1000];
	if (elf_section_data(fd, ".gnu_debuglink", buffer, sizeof(buffer)) < 0) {
		return 0;
	}
	strcpy(fname, (const char *)buffer);
	return *(uint32_t *)(buffer + ((strlen(fname) + 4) & 0xFFFFFFFCU));
}

/**
 * @brief g_path
 */
static char g_path[1024];

/**
 * @brief g_dir
 */
static char g_dir[1024];

/**
 * @brief g_index
 */
static int g_index, g_exe_self;

/**
 * @brief g_build_id
 */
static uint8_t g_build_id[100];

/**
 * @brief g_buildid_len
 */
static int g_buildid_len;

/**
 * @brief g_debug_fname
 */
static char g_debug_fname[1024];

/**
 * @brief g_debug_crc32
 */
static uint32_t g_debug_crc32;

/**
 * @brief debug_valid
 * @param debug_path
 * @return
 */
static int debug_valid(const char *debug_path)
{
	int fd = open(debug_path, O_RDONLY);
	if (fd < 0) {
		return 0;
	}

	if (g_buildid_len != 0) {
		uint8_t bid[100];
		int bid_len = elf_build_id(fd, bid);
		close(fd);

		if (bid_len != g_buildid_len || memcmp(g_build_id, bid, bid_len) != 0) {
			printf("Warning: unmatch build-id of debug file %s to %s\n",
					debug_path, g_path);
			return 0;
		}
		return 1;
	}
	if (g_debug_crc32 != 0) {
		uint32_t crc32 = file_crc32(fd);
		close(fd);
		if (crc32 != g_debug_crc32) {
			printf("Warning: unmatch CRC32 of debug file %s to %s\n",
					debug_path, g_path);
			return 0;
		}
		return 1;
	}

	printf("Warning: use debug file %s to %s without checking\n", debug_path, g_path);
	return 1;
}

void debug_try_init(const char *path, int exe_self)
{
	strcpy(g_path, path);
	g_exe_self = exe_self;
	g_index = 0;

	const char *p = strrchr(path, '/');
	memcpy(g_dir, path, p - path);
	g_dir[p - path] = '\0';

	int fd = open(path, O_RDONLY);
	g_buildid_len = elf_build_id(fd, g_build_id);
	g_debug_crc32 = elf_debug_link(fd, g_debug_fname);
	close(fd);

	if (g_debug_crc32 == 0) {
		sprintf(g_debug_fname, "%s.debug", p + 1);
	}
}

const char *debug_try_get(void)
{
	static char debug_path[2048];

	switch(g_index) {
	case 0:
		g_index = 1;
		if (g_exe_self && opt_debug_info_file) {
			if (debug_valid(opt_debug_info_file)) {
				return opt_debug_info_file;
			}
			printf("Error: invalid debug-info file %s\n", opt_debug_info_file);
			exit(4);
		}
	case 1:
		g_index = 2;
		return g_path;
	case 2:
		g_index = 3;
		if (g_buildid_len != 0) {
			char *p = debug_path;
			p += sprintf(debug_path, "/usr/lib/debug/.build-id/%02x/", g_build_id[0]);
			int i;
			for (i = 1; i < g_buildid_len; i++) {
				p += sprintf(p, "%02x", g_build_id[i]);
			}
			p += sprintf(p, ".debug");
			if (debug_valid(debug_path)) {
				return debug_path;
			}
		}
	case 3:
		g_index = 4;
		sprintf(debug_path, "%s/%s", g_dir, g_debug_fname);
		if (debug_valid(debug_path)) {
			return debug_path;
		}
	case 4:
		g_index = 5;
		sprintf(debug_path, "%s/.debug/%s", g_dir, g_debug_fname);
		if (debug_valid(debug_path)) {
			return debug_path;
		}
	case 5:
		g_index = 6;
		sprintf(debug_path, "/usr/lib/debug%s/%s", g_dir, g_debug_fname);
		if (debug_valid(debug_path)) {
			return debug_path;
		}
	default:
		return NULL;
	}
}
