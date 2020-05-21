# hashcheck

This is a LSM in which the kernel denies the execution of binaries to non-root users, unless:

* There is a `security.hash` extended-attribute upon the binary.
* The contents of that label match the SHA1 hash of the binary contents.

There is some back-story in the following blog-post:

* https://blog.steve.fi/linux_security_modules__round_two_.html

This builds upon the learning I made writing the [whitelist LSM](../whitelist/).
