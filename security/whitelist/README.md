whitelist
---------

This is a LSM in which the kernel denies the execution of binaries
to non-root users, unless there is an extended-attribute named
`security.whitelisted` present upon the binary.

**NOTE**: The content/value of that attribute doesn't matter, only
the existance is tested

There is some back-story in the following blog-post:

* https://blog.steve.fi/so_i_accidentally_wrote_a_linux_security_module.html

This module is available as a complete (github) pull-request against
the Linux kernel here:

* https://github.com/skx/linux/pull/1
