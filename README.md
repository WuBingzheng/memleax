## memleax

`memleax` detects memory leak of a *running* process.

`memleax` attachs a running process, hooks memory allocate/free APIs,
records all memory blocks, and reports the blocks which live longer
than 5 seconds (you can change this time by -e option) in real time.

So it is very *convenient* to use.
There is no need to recompile the program or restart the target process.
You run `memleax` to monitor the target process, wait the real-time memory
leak report, and kill it (e.g. by Ctrl-C) to stop monitoring.

NOTE: Since `memleax` does not run along with the whole life of target
process, it assumes the long-live memory blocks are memory leak.
Downside is you have to set the expire threshold by -e option according
to your scenarios; while the upside is the memory allocation for process
initialization is skipped, besides of the convenience.


## environment

* GNU/Linux, x86_64
* FreeBSD, amd64


## usage

To detect a running process:

    $ memleax [options] <target-pid>

then `memleax` monitors the target process, and will report memory leak in real time.

It stops monitoring and quits on:

* the target process quits,
* stop `memleax` (by SIGKILL or Ctrl-C), or
* the number of leaked memory blocks at one CallStack exceeds 1000 (change this threshold by -n option).

After quiting, it also gives statistics for the CallStacks with memory leak.
