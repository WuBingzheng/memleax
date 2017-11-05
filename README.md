## memleax

`memleax` debugs memory leak of a *running* process by attaching it,
without recompiling or restarting.


## status

Stable and completed.

`memleax` is a tool with single and clear aim. There is
no known bug to fix and no new feature to add by now.

However if you get any new feature or bug, please report
to [GitHub](https://github.com/WuBingzheng/memleax),
or [mail to me](mailto:wubingzheng@gmail.com).


## how it works

`memleax` debugs memory leak of a running process by attaching it.
It hooks the target process's invocation of memory allocation and free,
and reports the memory blocks which live long enough as memory leak, in real time.
The default expire threshold is 10 seconds, however you should always
set it by `-e` option according to your scenarios.

It is very *convenient* to use, and suitable for production environment.
There is no need to recompile the program or restart the target process.
You run `memleax` to monitor the target process, wait for the real-time memory
leak report, and then kill it (e.g. by Ctrl-C) to stop monitoring.

`memleax` follows new threads, but not forked processes.
If you want to debug multiple processes, just run multiple `memleax`.

## performance impact

Because target process's each invocation of memory allocation and free makes
a TRAP, the performance impact depends on the frequency of memory invocation
in target process.

For example, it impacts lightly to `nginx` with HTTP, while heavily with HTTPS,
because `OpenSSL` calls `malloc()` terribly.


## difference from Valgrind

+ `Valgrind` starts target process, while `memleax` attaches to a running process;

+ `Valgrind` gives memory leak report after target process quits, while `memleax`
reports in real time;

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

+ `GNU/Linux` at `x86` and `x86_64`, tested on CentOS 7.2 and Ubuntu 16.04
+ `GNU/Linux` at `armv7` and `aarch64`, tested on Raspbian and [pi64](https://github.com/bamarni/pi64).
+ `FreeBSD` at `i386` and `amd64`, tested on FreeBSD 10.3

NOTE: If `memleax` can not show function backtrace on `GNU/Linux` at `aarch64`,
you could try to compile the target program with `-funwind-tables` flag of GCC.


## install by package

There are DEB and RPM packages for
[releases](https://github.com/WuBingzheng/memleax/releases).

For Arch Linux users, `memleax` is available in AUR. Thanks to jelly.

For FreeBSD users, `memleax` is available in FreeBSD Ports Collection.
Thanks to tabrarg.

I tried to submit `memleax` to Fedora [EPEL](https://bugzilla.redhat.com/show_bug.cgi?id=1417531),
but failed. Any help is welcomed.

## build from source

The development packages of the following libraries are required:

+ `libunwind`
+ `libelf`
+ `libdw` or `libdwarf`. `libdw` is preferred. They are used to read dwarf debug-line
information. If you do not have them neither, set `--disable-debug_line` to
`configure` to disable it. As a result you will not see file name and line
number in backtrace.

These packages may have different names in different distributions, such as
`libelf` may names `libelf`, `elfutils-libelf`, or `libelf1`.

NOTE: On FreeBSD 10.3, there are built-in `libelf` and `libdwarf` already.
However another `libelf` and `libdwarf` still can be installed by `pkg`.
`memleax` works with `built-in libelf` and `pkg libdwarf`. So you should
install `libdwarf` by `pkg`, and must not install `libelf` by `pkg`.

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
