## memleax

`memleax` detects memory leak of a running process.


## environment

Linux, x86_64


## dependence

* libunwind
* libdwarf
* elfutils-libelf


## how it works

`memleax` attachs a running process, hooks memory allocate/free APIs,
and reports the memory blocks that live longer than an appointed
time.


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


## usage

To detect a running process:

    $ memleax [options] <target-pid>
