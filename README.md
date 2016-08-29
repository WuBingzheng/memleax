## memleax

`memleax` debugs memory leak of a *running* process, without recompiling or restarting.


## why write this

Once I met a memory leak problem in production environment.
The process began leaking memory after long-time running. If using current
memory debuggers, such as `Valgrind`, I have to restart the target process
and wait for several days again. Besides it impacts the target process
much for several days, which is not good for production.

So I need a tool to debug a running process without restarting it.
I searched the internet but didn't find one. Then I wrote such a tool myself.

Hope this is useful for others.


## status

Stable and completed.

`memleax` is a tool with single and clear aim. There is
no known bug to fix and no new feature to add by now.

However if you get any new feature or bug, please report
to [GitHub] (https://github.com/WuBingzheng/memleax),
or mail to <mailto:wubingzheng@gmail.com>.


## how it works

`memleax` attaches to a running process, hooks memory allocate/free APIs,
records all memory blocks, and reports the blocks which live longer
than 10 seconds (you can change this time by -e option) in real time.

It is very *convenient* to use, and suitable for production environment.
There is no need to recompile the program or restart the target process.
You run `memleax` to monitor the target process, wait for the real-time memory
leak report, and then kill it (e.g. by Ctrl-C) to stop monitoring.

NOTE: Since `memleax` does not run along with the whole life of target
process, it assumes the long-lived memory blocks are memory leaks.


## performance impact

Because target process's each memory allocation/free API invokes a TRAP, the
performance impact depends on how often the target program calls memory
APIs.
For example, it impacts lightly to nginx with HTTP, while heavily with HTTPS,
because OpenSSL calls malloc terribly.

Although performance impact is worthy of consideration, since `memleax` is
run to attach to the target process only when you are certain it is in memory leak,
and stopped after real-time memory leak report, it is not needed to attach to
the target process for long time.


## difference from Valgrind

+ `Valgrind` starts target process, while `memleax` attaches to a running process;

+ `Valgrind` gives memory leak report on quitting, while `memleax` assumes
that long-living memory blocks are leaks, so it reports in real time;

+ `Valgrind` reports all unfreed memory include program init, while `memleax`
reports only after attaching, skipping the init phase;

+ `Valgrind` runs target process on its virtual CPU, which makes it slow.
While `memleax` hooks memory APIs, which *may be* less slow if the target process
call memory APIs not often.

+ `Valgrind` debugs kinds of memory bugs, while `memleax` is lightweight and
only detects memory leak.

In summary, I think `Valgrind` is more powerful, while `memleax` is more
convenient and suitable for production environment.


## licence

GPLv2


## OS-machine

+ GNU/Linux-x86_64, tested on CentOS 7.2 and Ubuntu 16.04
+ FreeBSD-amd64, tested on FreeBSD 10.3


## requirement

+ `libunwind`
+ `libelf`
+ `libdw` or `libdwarf`. `libdw` is preferred. They are used to read dwarf debug-line
information. If you do not have them neither, set `--disable-debug_line` to
`configure` to disable it. As a result you will not see file name and line
number in backtrace.

NOTE: On FreeBSD 10.3, there are built-in `libelf` and `libdwarf` already.
However another `libelf` and `libdwarf` still can be installed by `pkg`.
`memleax` works with `built-in libelf` and `pkg libdwarf`. So you should
install `libdwarf` by `pkg`, and must not install `libelf` by `pkg`.


## build

After all required libraries are installed, run

    $ ./configure
    $ make
    $ sudo make install


## usage

### start

To debug a running process, run:

    $ memleax [options] <target-pid>

then `memleax` begins to monitor the target process, and report memory leak in real time.

You should always set expire time by `-e` options according to your scenarios.
For example, if you are debugging an HTTP server with keepalive, and there are
connections last for more than 5 minutes, you should set `-e 360` to cover it.
If your program is expected to free every memory in 1 second, you should set `-e 2`
to get report in time.

### wait and check the report

The memory blocks live longer than the threshold, are showed as:

    CallStack[3]: memory expires with 101 bytes, backtrace:
        0x00007fd322bd8220  libc-2.17.so  malloc()+0
        0x000000000040084e  test  foo()+14  foo.c:12
        0x0000000000400875  test  bar()+37  bar.c:20
        0x0000000000400acb  test  main()+364  test.c:80

`CallStack[3]` is the ID of CallStack where memory leak happens.

The backtrace is showed only on the first time, while it only shows the
ID and counter if expiring again:

    CallStack[3]: memory expires with 101 bytes, 2 times again

If the expired memory block is freed later, it shows:

    CallStack[6]: expired-memory frees after 10 seconds

If there are too many expired-memory-blocks are freed on one CallStack,
this CallStack will not be showed again:

    Warning: too many expired-free at CallStack[6]. will not show this CallStack later

When you think you have found the answer, stop the debug.

### stop

`memleax` quits on:

* you stop it, by Ctrl-C or kill,
* the target process quits,
* too many leaks at one CallStack (option -m), or
* too many CallStacks with memory leak (option -c).

After quitting, it also gives statistics for the CallStacks with memory leak:

    CallStack[3]: may-leak=20 (2020 bytes)
        expired=20 (2020 bytes), free_expired=0 (0 bytes)
        alloc=20 (2020 bytes), free=0 (0 bytes)
        freed memory live time: min=0 max=0 average=0
        un-freed memory live time: max=20
        0x00007fd322bd8220  libc-2.17.so  malloc()+0
        0x000000000040084e  test  foo()+14  foo.c:12
        0x0000000000400875  test  bar()+37  bar.c:20
        0x0000000000400acb  test  main()+364  test.c:80
