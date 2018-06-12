/*
 * can_exec_lsm.c - Steve Kemp
 *
 * This is a security module which is designed to allow a system
 * administrator to permit/deny the execution of arbitrary binaries
 * via the UID of the invoking user, and the path they're executing.
 *
 * Deploying
 * ---------
 *
 * Once compiled you'll find that your kernel has a new file:
 *
 *      /proc/sys/kernel/can-exec/enabled
 *
 * Enable the support by writing `1` to that file, but note that you'll
 * need to have setup the binary `/sbin/can-exec` before you do that!
 *
 * The user-space binary will receive two command-line arguments:
 *
 *  * The UID of the invoking user.
 *
 *  * The complete path, but not arguments, to the binary the user is invoking
 *
 * The user-space helper should return an exit-code of `0` if the execution
 * should be permitted, otherwise it will be denied.
 *
 * Steve
 * --
 *
 */

#include <linux/xattr.h>
#include <linux/binfmts.h>
#include <linux/lsm_hooks.h>
#include <linux/sysctl.h>
#include <linux/string_helpers.h>
#include <linux/lsm_hooks.h>
#include <linux/types.h>
#include <linux/cred.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>


//
// Is this module enabled?
//
// Controlled via /proc/sys/kernel/can-exec/enabled
//
static int can_exec_enabled = 0;


//
// Attempt to get the fully-qualified path of the given file.
//
// I suspect this is fragile and won't cope with mount-points, etc.
//
char *get_path(struct file *file, char *buf, int buflen)
{
    struct dentry *dentry = file->f_path.dentry;
    char *ret = dentry_path_raw(dentry, buf, buflen);
    return ret;
}


//
//  If this module is enabled then call our user-space helper,
// `/sbin/can-exec` to decide if child-processes can be executed.
//
static int can_exec_bprm_check_security_usermode(struct linux_binprm *bprm)
{
    struct subprocess_info *sub_info;
    int ret = 0;
    char *argv[4];

    //
    // The current task & UID.
    //
    const struct task_struct *task = current;
    kuid_t uid = task->cred->uid;

    //
    // Environment for our user-space helper.
    //
    static char *envp[] =
    {
        "HOME=/",
        "TERM=linux",
        "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL
    };

    //
    // If this module is not enabled we allow all.
    //
    if (can_exec_enabled == 0)
        return 0;

    //
    // If we're trying to exec our helper - then allow it
    //
    if (strcmp(bprm->filename, "/sbin/can-exec") == 0)
        return 0;

    //
    // The command we'll be executing.
    //
    argv[0] = "/sbin/can-exec";                    // helper
    argv[1] = (char *)kmalloc(10, GFP_KERNEL);     // UID
    argv[2] = NULL;                                // CMD
    argv[3] = NULL;                                // Terminator

    //
    // Populate the UID.
    //
    sprintf(argv[1], "%d",  uid.val);

    //
    // Get the fully-qualified path to the command the user is trying to run.
    //
    {
        char *path_buff = kmalloc(PAGE_SIZE, GFP_KERNEL);
        char *path = NULL;

        if (unlikely(!path_buff))
        {
            printk(KERN_INFO "kmalloc failed for path_buff");

            // avoid leaking
            kfree(argv[1]);

            return -ENOMEM;
        }

        memset(path_buff, 0, PAGE_SIZE);

        path = get_path(bprm->file, path_buff, PAGE_SIZE);

        if (path != NULL)
        {
            argv[2] = kstrdup(path, GFP_KERNEL);
            kfree(path_buff);
        }
        else
        {
            printk(KERN_INFO "calling get_path failed!");

            // avoid leaking
            kfree(argv[1]);
            kfree(path_buff);
            return -EPERM;
        }
    }

    //
    // Prepare to execute the user-space helper.
    //
    sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL,
                                         NULL, NULL, NULL);

    if (sub_info == NULL)
    {
        printk(KERN_INFO "failed to call call_usermodehelper_setup\n");

        // Avoid leaking
        kfree(argv[1]);
        kfree(argv[2]);
        return -ENOMEM;
    }


    //
    // Call the helper and get the return-code.
    //
    ret = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
    ret = (ret >> 8) & 0xff;

    //
    // Prevent leaks
    //
    kfree(argv[1]);
    kfree(argv[2]);

    //
    // Show the result.
    //
    printk(KERN_INFO "Return code from user-space was %d\n", ret);

    if (ret == 0)
        return (ret);
    else
        return (-EPERM);

}


struct ctl_path can_exec_sysctl_path[] =
{
    { .procname = "kernel", },
    { .procname = "can-exec", },
    { }
};

static struct ctl_table can_exec_sysctl_table[] =
{
    {
        .procname       = "enabled",
        .data           = &can_exec_enabled,
        .maxlen         = sizeof(int),
        .mode           = 0644,
        .proc_handler   = proc_dointvec,
    },
    { }
};


/*
 * The hooks we wish to be installed.
 */
static struct security_hook_list can_exec_hooks[] __lsm_ro_after_init =
{
    LSM_HOOK_INIT(bprm_check_security, can_exec_bprm_check_security_usermode),
};

/*
 * Initialize our module.
 */
static int __init can_exec_init(void)
{
    /* register /proc/sys/can-exec/enabled */
    if (!register_sysctl_paths(can_exec_sysctl_path, can_exec_sysctl_table))
        panic("sysctl registration failed.\n");

    /* register ourselves with the security framework */
    security_add_hooks(can_exec_hooks, ARRAY_SIZE(can_exec_hooks), "can_exec");
    printk(KERN_INFO "LSM initialized: can_exec\n");
    return 0;
}

security_initcall(can_exec_init);
