#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

#include "breakpoint.h"
#include "ptrace_utils.h"
#include "memblock.h"
#include "symtab.h"
#include "debug_line.h"

intptr_t g_current_entry;
pid_t g_current_thread;
int opt_backtrace_limit = BACKTRACE_MAX;

static pid_t g_target_pid;


/* build symbol-table and debug-line of one file */
static void info_build_debug(int (*buildf)(const char*, intptr_t, intptr_t, int),
		const char *name, const char *path,
		intptr_t start, intptr_t end, int exe_self)
{
	if (buildf(path, start, end, exe_self) > 0) {
		return;
	}

	char debug_path[2048];
	sprintf(debug_path, "/lib/debug%s", path);
	if (buildf(debug_path, start, end, exe_self) > 0) {
		return;
	}
	sprintf(debug_path, "/usr/lib/debug%s", path);
	if (buildf(debug_path, start, end, exe_self) > 0) {
		return;
	}

	if (exe_self) {
		printf("warning: no %s found for %s\n", name, path);
	}
}
/* build symbol-table and debug-line */
static void info_build(const char *debug_info_file)
{
	/* exe */
	char pname[30];
	char exe_name[1024];
	sprintf(pname, "/proc/%d/exe", g_target_pid);
	int exe_len = readlink(pname, exe_name, sizeof(exe_name));
	if (exe_len < 0) {
		perror("error in open /proc/pid/exe");
		exit(3);
	}
	exe_name[exe_len] = '\0';

	/* maps */
	sprintf(pname, "/proc/%d/maps", g_target_pid);
	FILE *maps = fopen(pname, "r");
	if (maps == NULL) {
		perror("error in open /proc/pid/maps");
		exit(3);
	}

	char line[1024];
	intptr_t start, end;
	char perms[5], path[1024];
	int ia, ib, ic, id;
	while(fgets(line, sizeof(line), maps) != NULL) {
		sscanf(line, "%lx-%lx %s %x %x:%x %d %s",
				&start, &end, perms, &ia, &ib, &ic, &id, path);

		if (perms[2] == 'x' && path[0] == '/') {
			int exe_self = (strcmp(exe_name, path) == 0);

			if (exe_self && debug_info_file) {
				strcpy(path, debug_info_file);
			}

			info_build_debug(symtab_build, "symbol table",
					path, start, end, exe_self);
			info_build_debug(debug_line_build, "debug line",
					path, start, end, exe_self);
			info_build_debug(ptr_maps_build, "maps",
					path, start, end, exe_self);
		}
	}
	fclose(maps);

	symtab_build_finish();
	debug_line_build_finish();
}

/* attach all existing threads */
static void attach_threads(void)
{
	char tname[30];
	sprintf(tname, "/proc/%d/task", g_target_pid);
	DIR *dirp = opendir(tname);

	pid_t pid;
	struct dirent *e;
	while ((e = readdir(dirp)) != NULL) {
		pid = atoi(e->d_name);

		if (pid == 0) continue;

		ptrace_attach(pid);
		waitpid(pid, NULL, __WALL);
		ptrace_trace_child(pid);

		if (pid == g_target_pid) { /* set br only once */
			breakpoint_init(g_target_pid);
		}

		ptrace_continue(pid, 0);
	}

	closedir(dirp);
}

static void signal_handler(int signo)
{
	kill(g_target_pid, SIGSTOP);
}

int main(int argc, char * const *argv)
{
	const char *debug_info_file = NULL;
	time_t memory_expire = 5;

	char *help = "Usage: memleax [options] target-pid\n"
			"Options:\n"
			"  -e <expire>\n"
			"      set memory free expire time, default is 5 seconds.\n"
			"  -d <debug-info-file>\n"
			"      set debug-info file.\n"
			"  -l <backtrace-limit>\n"
			"      set backtrace deep limit. less backtrace, better\n"
			"      performace. max is 50, and default is max.\n"
			"  -h  print help.\n"
			"  -v  print version.\n";

	/* parse options */
	int ch;
	while((ch = getopt(argc, argv, "hve:d:l:")) != -1) {
		switch(ch) {
		case 'e':
			memory_expire = atoi(optarg);
			if (memory_expire == 0) {
				printf("invalid expire: %s\n", optarg);
				return 1;
			}
			break;
		case 'd':
			debug_info_file = optarg;
			break;
		case 'l':
			opt_backtrace_limit = atoi(optarg);
			if (opt_backtrace_limit == 0) {
				printf("invalid backtrace limit: %s\n", optarg);
				return 1;
			}
			if (opt_backtrace_limit > BACKTRACE_MAX) {
				printf("too big backtrace limit: %s\n", optarg);
				return 1;
			}
			break;
		case 'h':
			printf("%s", help);
			return 0;
		case 'v':
			printf("%s\n", "1.0");
			return 0;
		default:
			printf("%s", help);
			return 1;
		}
	}
	if (optind + 1 != argc) {
		fprintf(stderr, "invalid argument\n");
		return 1;
	}
	g_target_pid = atoi(argv[optind]);
	if (g_target_pid == 0) {
		fprintf(stderr, "invalid target pid: %s\n", argv[optind]);
		return 2;
	}

	/* prepare */
	info_build(debug_info_file);

	attach_threads();

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* begin work */
	printf("== Begin monitoring process %d...\n", g_target_pid);
	time_t begin = time(NULL);
	struct breakpoint_s *bp = NULL;
	intptr_t return_address = 0, return_code = 0;
	intptr_t arg1 = 0, arg2 = 0;
	pid_t last_thread = 0;
	pid_t hold_threads[1000];
	int hold_thread_num = 0;

	while (1) {
		/* wait for breakpoint */
		int status;
		pid_t pid = waitpid(-1, &status, __WALL);
		log_debug("wait: pid=%d status=%x\n", pid, status);
		if (pid < 0) {
			perror("\n== Error on waitpid()");
			break;
		}
		if (!WIFSTOPPED(status)) {
			if (pid != g_target_pid) {
				continue;
			}
			printf("\n== Target process exit.\n");
			break;
		}
		int signum = WSTOPSIG(status);
		if (signum == SIGSTOP) { /* by signal_handler() */
			printf("\n== Stop monitoring after %ld seconds.\n", time(NULL) - begin);
			break;
		}
		if (signum != SIGTRAP) { /* forward signals */
			ptrace_continue(pid, signum);
			continue;
		}

		/* new thread or process? */
		int pevent = status >> 16;
		if (pevent != 0) {
			pid_t child = ptrace_get_child(pid);
			waitpid(child, NULL, __WALL);
			if (pevent == PTRACE_EVENT_CLONE) { /* thread */
				log_debug("new thread id=%d\n", child);
				ptrace_continue(child, 0);
			} else { /* process, detach it */
				log_debug("new process id=%d\n", child);
				breakpoint_cleanup(child);
				ptrace_detach(child, 0);
			}
			ptrace_continue(pid, 0);
			continue;
		}

		/* get registers */
		struct user_regs_struct regs;
		ptrace_get_regs(pid, &regs);

		/* unwind the RIP back by 1 to let the CPU execute the
		 * original instruction that was there. */
		regs.rip -= 1;
		ptrace_set_regs(pid, &regs);
		log_debug("\nbreak at: %llx\n", regs.rip);

		/* another thread is in a function, so hold this one */
		if (return_address != 0 && last_thread != pid) {
			log_debug("hold thread: %d\n", pid);
			hold_threads[hold_thread_num++] = pid;
			continue;
		}

		/* breaking after a function-entry-breakpoint, which means
		 * we are at the function-return-breakpoint, or at another
		 * function-entry-breakpoint. In the latter case, we ignore
		 * the formor function-entry-breakpoint. */
		if (return_address != 0) {
			/* recover return code */
			ptrace_set_data(pid, return_address, return_code);
			/* re-set breakpoint at entry address */
			ptrace_set_int3(pid, bp->entry_address, bp->entry_code);
		}

		if (regs.rip == return_address) {
			/* -- at function return */

			/* set these for building backstrace far away */
			g_current_entry = bp->entry_address;
			g_current_thread = pid;

			bp->handler(regs.rax, arg1, arg2);
			return_address = 0;

			/* wakeup hold-threads */
			int i;
			for (i = 0; i < hold_thread_num; i++) {
				ptrace_continue(hold_threads[i], 0);
				log_debug("wakeup thread: %d\n", hold_threads[i]);
			}
			hold_thread_num = 0;

		} else if ((bp = breakpoint_by_entry(regs.rip)) != NULL) {
			/* -- at function entry */

			/* recover entry code */
			ptrace_set_data(pid, bp->entry_address, bp->entry_code);

			/* set breakpoint at return address */
			return_address = ptrace_get_data(pid, regs.rsp);
			return_code = ptrace_get_data(pid, return_address);
			ptrace_set_int3(pid, return_address, return_code);

			/* save arguments */
			arg1 = regs.rdi;
			arg2 = regs.rsi;

			last_thread = pid;

		} else {
			printf("unknown breakpoint %llx\n", regs.rip);
			break;
		}

		ptrace_continue(pid, 0);

		memblock_expire(memory_expire);
	}

	/* clean up, entry and return point */
	breakpoint_cleanup(g_target_pid);
	if (return_address != 0) {
		ptrace_set_data(g_target_pid, return_address, return_code);
	}

	ptrace_detach(g_target_pid, 0);

	if (time(NULL) - begin >= memory_expire * 2) {
		printf("== Callstack statistics: (in ascending order)\n\n");
		callstack_report();
	} else {
		printf("== Monitor %ld seconds at least for statistics.\n",
				memory_expire * 2);
	}
	return 0;
}
