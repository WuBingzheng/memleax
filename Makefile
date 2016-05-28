CFLAGS	= -g -O2 -Wall -I/usr/local/include/
LDLIBS	= -L/usr/local/lib/ -lunwind-ptrace -lunwind -lunwind-x86_64 -lelf -lprocstat -lutil

OBJS	= breakpoint.o callstack.o debug_line.o memblock.o memleax.o proc_info.o ptr_backtrace.o symtab.o

TARGET	= memleax

# delete the following 2 lines if you do not have libdwarf,
# and you will lose debug-line show in output.
CFLAGS += -DMLX_WITH_LIBDWARF
LDLIBS += -ldwarf

CFLAGS += -DMLX_LIBELF_INNER

CFLAGS += -DMLX_FREEBSD

memleax : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

clean :
	rm -f $(OBJS) depends memleax

breakpoint.o: breakpoint.c callstack.h hash.h list.h ptr_backtrace.h \
	memblock.h breakpoint.h ptrace_utils.h memleax.h symtab.h
callstack.o: callstack.c hash.h list.h array.h symtab.h callstack.h \
	ptr_backtrace.h debug_line.h memleax.h memblock.h
debug_line.o: debug_line.c debug_line.h array.h memleax.h proc_info.h
memblock.o: memblock.c hash.h list.h memblock.h callstack.h \
	ptr_backtrace.h
memleax.o: memleax.c breakpoint.h ptrace_utils.h memleax.h memblock.h \
	list.h callstack.h hash.h ptr_backtrace.h symtab.h debug_line.h \
	proc_info.h
proc_info.o: proc_info.c proc_info.h
ptr_backtrace.o: ptr_backtrace.c ptr_backtrace.h proc_info.h memleax.h \
	symtab.h list.h hash.h ptrace_utils.h
symtab.o: symtab.c array.h memleax.h proc_info.h
