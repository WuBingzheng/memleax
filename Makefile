CFLAGS	= -g -O2 -Wall
LDLIBS	= -lunwind-ptrace -lunwind -lunwind-x86_64 -lelf

SOURCES	= $(wildcard *.c)
OBJS	= $(patsubst %.c,%.o,$(SOURCES))

TARGET	= memleax

# delete the following 2 lines if you do not have libdwarf,
# and you will lose debug-line show in output.
CFLAGS += -DMLD_WITH_LIBDWARF
LDLIBS += -ldwarf

memleax : $(OBJS)
	gcc -o $@ $^ $(LDLIBS)

clean :
	rm -f $(OBJS) depends memleax

depends : $(SOURCES)
	gcc -MM $(CFLAGS) *.c > depends

include depends
