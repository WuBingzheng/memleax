#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "breakpoint.h"
#include "ptrace_utils.h"
#include "memblock.h"
#include "symtab.h"
#include "debug_line.h"
#include "proc_info.h"
#include "memleax.h"

uintptr_t g_current_entry;
pid_t g_current_thread;
int opt_backtrace_limit = BACKTRACE_MAX;

static pid_t g_target_pid;
static const char *opt_debug_info_file;


/* try debug-info-files for buiding some info */
void try_debug(int (*buildf)(const char*, size_t, size_t, int),
		const char *name, const char *path,
		size_t start, size_t end, int exe_self)
{
	if (exe_self && opt_debug_info_file) {
		if (buildf(opt_debug_info_file, start, end, 1) < 0) {
			printf("Error: no %s found in debug-info-file %s.",
					name, opt_debug_info_file);
			exit(4);
		}
		return;
	}

	if (buildf(path, start, end, exe_self) > 0) {
		return;
	}

	/* try debug-info-dir */
	char debug_path[2048];
	sprintf(debug_path, "%s.debug", path);
	if (buildf(debug_path, start, end, exe_self) > 0) {
		return;
	}
	sprintf(debug_path, "/lib/debug%s.debug", path);
	if (buildf(debug_path, start, end, exe_self) > 0) {
		return;
	}
	sprintf(debug_path, "/usr/lib/debug%s.debug", path);
	if (buildf(debug_path, start, end, exe_self) > 0) {
		return;
	}
	if (buildf(path, start, end, exe_self) > 0) {
		return;
	}

	if (exe_self) {
		printf("Warning: no %s found for %s\n", name, path);
	}
}


static void signal_handler(int signo)
{
	kill(g_target_pid, SIGSTOP);
}

int main(int argc, char * const *argv)
{
	time_t memory_expire = 5;
	int stop_number = 1000;

	char *help = "Usage: memleax [options] target-pid\n"
			"Options:\n"
			"  -e <expire>\n"
			"      set memory free expire time, default is 5 seconds.\n"
			"  -d <debug-info-file>\n"
			"      set debug-info file.\n"
			"  -n <stop-number>\n"
			"      stop monitoring if number of expired memory block\n"
			"      of a CallStack exceeds this one. default is 1000.\n"
			"  -l <backtrace-limit>\n"
			"      set backtrace deep limit. less backtrace, better\n"
			"      performace. max is 50, and default is max.\n"
			"  -h  print help.\n"
			"  -v  print version.\n";

	/* parse options */
	int ch;
	while((ch = getopt(argc, argv, "hve:d:l:n:")) != -1) {
		switch(ch) {
		case 'e':
			memory_expire = atoi(optarg);
			if (memory_expire == 0) {
				printf("invalid expire: %s\n", optarg);
				return 1;
			}
			break;
		case 'd':
			opt_debug_info_file = optarg;
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
		case 'n':
			stop_number = atoi(optarg);
			if (stop_number == 0) {
				printf("invalid stop_number: %s\n", optarg);
				return 1;
			}
			break;
		case 'h':
			printf("%s", help);
			return 0;
		case 'v':
			printf("Version: 0.1\n");
			printf("Author: Wu Bingzheng\n");
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
	ptr_maps_build(g_target_pid);
	symtab_build(g_target_pid);
	debug_line_build(g_target_pid);

#ifdef MLX_LINUX
	/* Linux: attach all other exist threads */
	pid_t task_id;
	while ((task_id = proc_tasks(g_target_pid)) > 0) {
		if (task_id == g_target_pid) {
			continue;
		}
		ptrace_attach(task_id);
		waitpid(task_id, NULL, __WALL);
		ptrace_trace_child(task_id);
		ptrace_continue(task_id, 0);
	}
#endif

	/* attach target and set breakpoint */
	ptrace_attach(g_target_pid);
	waitpid(g_target_pid, NULL, 0);
	ptrace_trace_child(g_target_pid);

	breakpoint_init(g_target_pid);

	ptrace_continue(g_target_pid, 0);

	/* signals */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* begin work */
	printf("== Begin monitoring process %d...\n", g_target_pid);
	time_t begin = time(NULL);
	struct breakpoint_s *bp = NULL;
	uintptr_t return_address = 0, return_code = 0;
	uintptr_t arg1 = 0, arg2 = 0;
	pid_t last_thread = 0;
	pid_t hold_threads[1000];
	int hold_thread_num = 0;

	while (1) {
		/* wait for breakpoint */
		int status;
#ifdef MLX_LINUX
		pid_t pid = waitpid(-1, &status, __WALL);
#else /* FreeBSD */
		pid_t pid = waitpid(-1, &status, 0);
#endif
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
		if (signum == SIGSTOP) {
#ifdef MLX_FREEBSD
			/* fork in FreeBSD: trap at child process first instruction */
			if (ptrace_new_child_first(pid)) {
				breakpoint_cleanup(pid);
				ptrace_detach(pid, 0);
				continue;
			}
#endif
			/* by signal_handler() */
			printf("\n== Terminate monitoring.\n");
			break;
		}
		if (signum != SIGTRAP) { /* forward signals */
			ptrace_continue(pid, signum);
			continue;
		}

#ifdef MLX_FREEBSD
		/* fork in FreeBSD: trap because of new child */
		if (ptrace_new_child_forked(pid)) {
			ptrace_continue(pid, 0);
			continue;
		}
#endif
#ifdef MLX_LINUX
		/* new child (thread or process) in Linux */
		if (ptrace_new_child(status)) {
			pid_t child = ptrace_get_child(pid);
			waitpid(child, NULL, __WALL);
			if (ptrace_new_child_thread(status)) { /* thread */
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
#endif

		/* get registers */
		registers_info_t regs;
		ptrace_get_regs(pid, &regs);

		/* unwind the RIP back by 1 to let the CPU execute the
		 * original instruction that was there. */
		REG_RIP(regs) -= 1;
		ptrace_set_regs(pid, &regs);

		uintptr_t rip = REG_RIP(regs);
		log_debug("\nbreak at: %lx\n", rip);

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

		if (rip == return_address) {
			/* -- at function return */

			/* set these for building backstrace far away */
			g_current_entry = bp->entry_address;
			g_current_thread = pid;

			bp->handler(REG_RAX(regs), arg1, arg2);
			return_address = 0;

			/* wakeup one hold-thread */
			if (hold_thread_num != 0) {
				ptrace_continue(hold_threads[--hold_thread_num], 0);
				log_debug("wakeup thread: %d\n", hold_threads[hold_thread_num]);
			}

		} else if ((bp = breakpoint_by_entry(rip)) != NULL) {
			/* -- at function entry */

			/* recover entry code */
			ptrace_set_data(pid, bp->entry_address, bp->entry_code);

			/* set breakpoint at return address */
			return_address = ptrace_get_data(pid, REG_RSP(regs));
			return_code = ptrace_get_data(pid, return_address);
			ptrace_set_int3(pid, return_address, return_code);

			/* save arguments */
			arg1 = REG_RDI(regs);
			arg2 = REG_RSI(regs);

			last_thread = pid;

		} else {
			/* maybe other thread met a return-breakpoint which
			 * is recoverd already */
			uintptr_t code = ptrace_get_data(pid, rip);
			if ((code & 0xFF) == 0xCC) {
				printf("unknown breakpoint %lx\n", rip);
				break;
			}
		}

		if (memblock_expire(memory_expire) >= stop_number) {
			printf("\n== %d expired memory blocks of one CallStack.\n", stop_number);
			break;
		}

		ptrace_continue(pid, 0);
	}

	/* clean up, entry and return point */
	breakpoint_cleanup(g_target_pid);
	if (return_address != 0) {
		ptrace_set_data(g_target_pid, return_address, return_code);
	}

	ptrace_detach(g_target_pid, 0);

	if (time(NULL) - begin <= memory_expire) {
		printf("== Your monitoring time is too short. "
				"%ld seconds is need.\n", memory_expire);
		return 2;
	}

	callstack_report();
	return 0;
}
