can-exec
--------

This is a LSM in which the kernel calls a user-mode helper to decide
if binaries should be executed.

Build this as you would expect then:

* Install `/sbin/can-exec` from the `samples/` directory.
   * This will be invoked to decide if (non-root) users can execute binaries.
* Enable the module.
   * `echo 1 > /proc/sys/kernel/can-exec/enabled`

There is some back-story in the following blog-post:

* https://blog.steve.fi/yet_more_linux_security_module_craziness___.html
