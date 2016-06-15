## memleax

`memleax` detects memory leak of a *running* process.


## why write this

Once I met a memory leak problem in production environment.
The process began memory leak after long-time running. If using current
memory debuggers, such as `Valgrind`, I have to restart the target process
and wait for several days again. Besides it impacts the target process
much for several days, which is not good for production environment.

So I need a tool to debug a running process without restarting it.
I searched the internet but didn't find one. Then I wrote such a tool myself.

Hope this is useful for others.


## how it works

`memleax` attachs a running process, hooks memory allocate/free APIs,
records all memory blocks, and reports the blocks which live longer
than 5 seconds (you can change this time by -e option) in real time.

It is very *convenient* to use, and suitable for production environment.
There is no need to recompile the program or restart the target process.
You run `memleax` to monitor the target process, wait for the real-time memory
leak report, and then kill it (e.g. by Ctrl-C) to stop monitoring.

NOTE: Since `memleax` does not run along with the whole life of target
process, it assumes the long-live memory blocks are memory leak.
Downside is you have to set the expire threshold by -e option according
to your scenarios; while the upside is the memory allocation for process
initialization is skipped, besides of the convenience.


## performace influence

Because target progress's each memory allocation/free API invokes a TRAP, the
performance influence depends on how often the target program calls memory
APIs.
For example, it impacts lightly to nginx with HTTP, while heavily with HTTPS,
because OpenSSL calls malloc seriously.

Although performance influence is worthy of consideration, since `memleax` is
run to attach the target progress only when you certain it is in memory leak,
and stopped after real-time memory leak report, so it is not need to attach
the target progress for long time.


## difference with Valgrind

+ `Valgrind` starts target process, while `memleax` attachs a running process;

+ `Valgrind` gives memory leak report on quiting, while `memleax` assumes
that long-living memory blocks are leaks, so it reports in real time;

+ `Valgrind` runs target process on its virtual CPU, which makes it slow.
While `memleax` hooks memory APIs, which *maybe* less slow, if the target process
call memory APIs not often.

+ `Valgrind` debugs kinds of memory bugs, while `memleax` is lightweight and
only detects memory leak.

In summary, I think `Valgrind` is more powerful, while `memleax` is more
convenient and suitable for production environment.


## environment

+ GNU/Linux x86_64, test on CentOS 7.2 and Ubuntu 16.04
+ FreeBSD amd64, test on FreeBSD 10.3


## requirement

+ libunwind
+ libdwarf, if you do not have this, set `--disable-libdwarf` to `configure` to
  disable it, and you will not see file name and line number in backtrace.
+ libelf, on GNU/Linux, while FreeBSD has libelf already.


## install

### debian

	sudo apt-get install libunwind-dev libelf-dev libdwarf-dev
	./configure
	make && sudo make install


## usage

To detect a running process:

    $ memleax [options] <target-pid>

then `memleax` monitors the target process, and will report memory leak in real time.

It stops monitoring and quits on:

* the target process quits,
* `memleax` exits (by SIGKILL or Ctrl-C), or
* the number of leaked memory blocks at one CallStack exceeds 1000 (change this threshold by -n option).

After quiting, it also gives statistics for the CallStacks with memory leak.
