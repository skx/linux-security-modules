Linux Security Modules
----------------------

A collection of three simple linux security modules, more for learning and experimentation than for serious use.

* [whitelist](whitelist/)
   * Only allow execution of commands with xattr present.
* [hashcheck](hashcheck/)
   * Only allow execution of commands with xattr containing valid SHA1sum of binaries.

Finally the more serious module [can_exec](can_exec) which invokes a user-space helper to decide if commands can be executed.

On my system I have this:

      root@kernel:~# cat /etc/can-exec/redis.conf
      /usr/bin/redis-server

      root@kernel:~# cat /etc/can-exec/nobody.conf
      /bin/sh
      /bin/dash
      /bin/bash
      /usr/bin/id
      /usr/bin/uptime

To enable the module you'll need to run this, post-boot:

    # echo 1 > /proc/sys/kernel/can-exec/enabled
