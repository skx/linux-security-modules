Linux Security Modules
----------------------

This repository contains a small collection of linux security modules, which were written as a part of a learning/experimentation process.

## Modules

There are three modules contained within this repository, two of which are simple tests and one of which is more "real".

The only real/useful module is:

* [can-exec](security/can-exec)
   * The user-space helper `/sbin/can-exec` is invoked to determine whether a user can execute a specific command.
   * Because user-space controls execution policies can be written/updated dynamically.

The following two modules were written as I started the learning-process, and demonstrate creating simple standalone modules, albeit ones which do not actually provide any significant security benefit:

* [whitelist](security/whitelist/)
   * Only allow execution of binaries which have a specific `xattr` present.
* [hashcheck](security/hashcheck/)
   * Only allow execution of commands with `xattr` containing valid SHA1sum of binaries.
   * This builds upon the previous module.


## Linux Compatibility & Compilation

The code has been tested upon kernels as recent as 5.4.22.

Copy the contents of `security/` into your local Kernel-tree, and run `make menuconfig` to enable the appropriate options.

**NOTE**: Over time the two files `security/Kconfig` & `security/Makefile` might need resyncing with the base versions installed with the Linux source-tree, you can look for mentions of `CAN_EXEC`, `HASH_CHECK`, & `WHITELIST` to see what I've done to add the modules.

For a Debian GNU/Linux host, building a recent kernel, these are the dependencies you'll need to install:

      # apt-get install flex bison bc libelf-dev libssl-dev \
                        build-essential make libncurses5-dev \
                        git-core
