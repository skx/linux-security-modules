can-exec
--------

This is a LSM in which the kernel calls a user-mode helper to decide
if binaries should be executed.

Every time a command is to be executed the kernel will invoke a user-space helper:

    /sbin/can-exec $UID $COMMAND

The arguments supplied are the UID of the invoking user, and the command
they're trying to execute.  If the user-space binary exits with a return-code
of zero the execution will be permitted, otherwise it will be denied.



Installation & Configuration
----------------------------

Build the kernel with this support enabled, and then you'll need to
configure the user-space side.

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


Once the user-space binary is in-place you'll need to enable the enforcement
by running:

     echo 1 > /proc/sys/kernel/can-exec/enabled


Testing
-------

To test that the module is enabled you'll want to run:

    $ echo $(cat /sys/kernel/security/lsm)
    capability,safesetid,can_exec

If you see `can_exec` listed, and you get output from this command you're good:

    $ dmesg | grep LSM
    [    0.282365] LSM: Security Framework initializing
    [    0.283323] LSM initialized: can_exec


Links
-----

There is some back-story in the following blog-post:

* https://blog.steve.fi/yet_more_linux_security_module_craziness___.html
