---
title: COMP 3430 Operating Systems
subtitle: "Assignment 2: shell!"
date: Summer 2025
---

Overview
========

This directory contains the following:

* This `README.md` file (you're reading it!).
* A `Makefile` that can build the sample code.
* A generic, POSIX-like interface for opening and reading files in a file system
  (`nqp_io.h`).
* Two pre-compiled implementation of this interface (`nqp_exfat.o` and
  `nqp_exfat_arm.o`).
* A sample exFAT-formatted volume containing some files and programs
  (`root.img`).
* An initial template implementation of a shell that works with the provided
  volume.

Building and running
====================

The only runnable program in this directory is `nqp_shell.c`.

You can compile this program on the command line:

```bash
make
```

You can run this program on the command line by passing the volume that you
would like to have the shell use as a root directory:

```bash
./nqp_shell root.img
```

Pre-compiled implementations
----------------------------

### `nqp_exfat.o` on x86_64 (Aviary, your own Linux, or WSL)

This is a pre-compiled implementation of the interface defined in `nqp_io.h`.
The file was compiled on Aviary. This *may* work on other x86_64 Linux
installations (e.g., Windows Subsystem for Linux) but will not work on macOS at
all (neither Intel nor Apple Silicon).

This is the default option, you don't need to do anything special to use this
version of the pre-compiled file system code.

### `nqp_exfat_arm.o` on arm64 (macOS and Lima)

This is a pre-compiled implementation of the interface defined in `nqp_io.h`
that has been compiled in a virtual machine running an arm64 version of Linux.
This will work on Apple systems that are running on Apple Silicon, using Linux
in something like [Lima]. This will not work on macOS at all (neither Intel nor
Apple Silicon).

To build with the arm version, you'll have to specify a variable when you run
`make`:

```bash
make NQP_EXFAT=nqp_exfat_arm.o
```

[Lima]: https://lima-vm.io/

Using your own implementation
-----------------------------

You are more than welcome to use your own implementation of `nqp_io.h` if you
would like! You can replace `nqp_exfat.o` with your own `nqp_exfat.o` or you can
place your own `nqp_exfat.c` into this directory and it *should* be compiled
alongside your shell.
