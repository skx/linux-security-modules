# Linux Security Modules

This repository contains a small collection of linux security modules, which were written as a part of a learning/experimentation process.

The code present has been compiled and tested against the most recent long-term kernel, at the time of writing that is __5.10.17__.

If you want to port this code to a newer kernel, in the future, then the following bug-report is a good overview of how I approach things:

* https://github.com/skx/linux-security-modules/issues/13



## Included Modules

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




## Compilation

Copy the contents of `security/` into your local Kernel-tree, and run `make menuconfig` to enable the appropriate options.

Further notes are available within the appropriate module subdirectories.

For a Debian GNU/Linux host, these are the kernel build-dependencies you'll need to install, if they're not already present:

      # apt-get install flex bison bc libelf-dev libssl-dev \
                        build-essential make libncurses5-dev \
                        git-core



### Tracking Kernel Changes

As new kernels are released it is possible the two files `security/Kconfig` & `security/Makefile` might need resyncing with the base versions installed with the Linux source-tree.

You should be able to update them just by running `diff` and copying any lines referring to the modules `CAN_EXEC`, `HASH_CHECK`, & `WHITELIST` into place.
