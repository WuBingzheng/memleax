## memleax

`memleax` detects memory leak of a *running* process.

It is very convenient, need not re-compiling or restarting target process,
and gives memory leak report in real time.


## how it works

`memleax` attachs a running process, hooks memory allocate/free APIs,
records all memory blocks, and reports the blocks which lives longer
than 5 seconds (you can change this time by -e option).


## difference from Valgrind

* `Valgrind` starts target process, while `memleax` attachs a running process;

* `Valgrind` gives report on target process ending, while `memleax` assumes
that long-living memory blocks are leaks, so it reports in real time;

* `Valgrind` runs target process on its virtual CPU, which makes it slow.
While `memleax` hooks memory APIs, which is less slow.

* Of course, `Valgrind` is much more powerful, while `memleax` is lightweight,
and only detects memory leak.

In summary, I think `Valgrind` is more powerful, while `memleax` is more suitable
for production environment.


## environment

Linux, x86_64


## dependence

* libunwind
* elfutils-libelf
* libdwarf

If you do not have libdwarf, modify `Makefile` to disable it.
As a result, you can not see file name and line number in backtrace.


## usage

To detect a running process:

    $ memleax [options] <target-pid>
