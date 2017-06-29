can-exec
--------

This is a LSM in which the kernel calls a user-mode helper to decide
if binaries should be executed.

Build this as you would expect then:

* Install `/sbin/can-exec` from the `samples/` directory.
   * This will be invoked to decide if (non-root) users can execute binaries.
* Enable the module.
   * `echo 1 > /proc/sys/kernel/can-exec/enabled`


Configuration
-------------

The user-space helper will be executed to determine whether a command
can be executed - and that will read a per-user configuration file to
make the choice.

For example I have this on my system:

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

`root` is implicitly allowed to execute everything.

As the user-space executable is called on-demand changes you make to the
configuration files will immediately take effect.

Links
-----

There is some back-story in the following blog-post:

* https://blog.steve.fi/yet_more_linux_security_module_craziness___.html
