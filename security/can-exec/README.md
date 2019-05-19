can-exec
--------

This is a LSM in which the kernel calls a user-mode helper to decide
if binaries should be executed.

Every time a command is to be executed the kernel will invoke a user-space helper:

    /sbin/can-exec $UID $COMMAND

The arguments supplied are the UID of the invoking user, and the command
they're trying to execute.  If the user-space binary exits with a return-code
of zero the execution will be permitted, otherwise it will be denied.



## Installation & Configuration

First of all you'll need to build the kernel with this module enabled.  Since there have been changes to the Kernel "recently", to allow LSM module-stacking, these are the explicit settings I used for my own tests:

     #
     # Security options
     #
     CONFIG_KEYS=y
     CONFIG_KEYS_COMPAT=y
     # CONFIG_PERSISTENT_KEYRINGS is not set
     # CONFIG_BIG_KEYS is not set
     # CONFIG_TRUSTED_KEYS is not set
     CONFIG_ENCRYPTED_KEYS=m
     # CONFIG_KEY_DH_OPERATIONS is not set
     CONFIG_SECURITY_DMESG_RESTRICT=y
     CONFIG_SECURITY=y
     CONFIG_SECURITYFS=y
     CONFIG_SECURITY_NETWORK=y
     CONFIG_PAGE_TABLE_ISOLATION=y
     CONFIG_SECURITY_NETWORK_XFRM=y
     CONFIG_SECURITY_PATH=y
     # CONFIG_INTEL_TXT is not set
     CONFIG_HAVE_HARDENED_USERCOPY_ALLOCATOR=y
     # CONFIG_HARDENED_USERCOPY is not set
     # CONFIG_FORTIFY_SOURCE is not set
     # CONFIG_STATIC_USERMODEHELPER is not set
     # CONFIG_SECURITY_SELINUX is not set
     # CONFIG_SECURITY_SMACK is not set
     # CONFIG_SECURITY_TOMOYO is not set
     # CONFIG_SECURITY_APPARMOR is not set
     # CONFIG_SECURITY_LOADPIN is not set
     # CONFIG_SECURITY_YAMA is not set
     CONFIG_SECURITY_SAFESETID=y
     CONFIG_SECURITY_CAN_EXEC=y
     # CONFIG_SECURITY_HASH_CHECK is not set
     # CONFIG_SECURITY_WHITELIST is not set
     # CONFIG_INTEGRITY is not set
     CONFIG_DEFAULT_SECURITY_CAN_EXEC=y
     # CONFIG_DEFAULT_SECURITY_DAC is not set
     CONFIG_LSM="yama,loadpin,safesetid,integrity,can-exec,selinux,smack,tomoyo,apparmor"
     CONFIG_XOR_BLOCKS=m
     CONFIG_ASYNC_CORE=m
     CONFIG_ASYNC_MEMCPY=m
     CONFIG_ASYNC_XOR=m
     CONFIG_ASYNC_PQ=m
     CONFIG_ASYNC_RAID6_RECOV=m
     CONFIG_CRYPTO=y


## Kernel Testing

Once you've rebooted into your new kernel you should be able to see that the module is compiled successfully and available by running:

    $ echo $(cat /sys/kernel/security/lsm)
    capability,safesetid,can_exec

If you see `can_exec` listed, and you get output from this command you're good:

    $ dmesg | grep LSM
    [    0.282365] LSM: Security Framework initializing
    [    0.283323] LSM initialized: can_exec

Finally you should also see the file `/proc/sys/kernel/can-exec/enabled` exists, and will have the contents `0` - as the module is not yet enabled.


## Setup User-Space

The goal of this module is that the kernel will invoke a user-space helper whenever a binary is executed.  The next step is thus to make that binary available.

* Install `/sbin/can-exec` from the `samples/` directory.
   * This will be invoked to decide if users can execute binaries.
   * The sample implementation will read configuration files beneath `/etc/can-exec`, but you could rewrite it to only allow execution of commands between specific times of the day, or something entirely different!

I have these configuration-files setup upon my system:

      root@kernel:~# cat /etc/can-exec/redis.conf
      /usr/bin/redis-server

      root@kernel:~# cat /etc/can-exec/nobody.conf
      /bin/sh
      /bin/dash
      /bin/bash
      /usr/bin/id
      /usr/bin/uptime

That means:

* The `redis` user can execute __only__ the single binary `/usr/bin/redis-server`.
* The `nobody` user can execute:
   * `/bin/sh`
   * `/bin/dash`
   * `/bin/bash`
   * `/usr/bin/id`
   * `/usr/bin/uptime`

Once the user-space binary is in-place you can enable the enforcement
by running:

     echo 1 > /proc/sys/kernel/can-exec/enabled


## Links

There is some back-story in the following blog-post:

* https://blog.steve.fi/yet_more_linux_security_module_craziness___.html
