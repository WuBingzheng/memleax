CFLAGS          = -g -O2 -Wall
LDFLAGS		= -lunwind-ptrace -lunwind -lunwind-x86_64 -ldwarf -lelf

SOURCES         = $(wildcard *.c)
OBJS            = $(patsubst %.c,%.o,$(SOURCES))

TARGET          = memleax

memleax : $(OBJS)
	gcc -o $@ $^ $(LDFLAGS)

clean :
	rm -f $(OBJS) depends memleax

depends : $(SOURCES)
	gcc -MM $(CFLAGS) *.c > depends

include depends
