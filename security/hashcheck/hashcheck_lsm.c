/*
 * hashcheck_lsm.c - Steve Kemp
 *
 * This is a security module which is designed to prevent
 * the execution of unknown or (maliciously) altered binaries.
 *
 * This is achieved by computing an SHA1 digest of every
 * binary before it is executed, then comparing that to the
 * (assumed) known-good value which is stored as an extended
 * attribute alongside the binary.
 *
 * The intention is thus that a malicious binary will either
 * have a missing hash, or a bogus hash.
 *
 * Potential flaws in this approach include:
 *
 *  * The use of scripting languages which can do arbitrary "stuff".
 *
 *  * An attacker updating the hashes after replacing the binarie(s).
 *
 *  * The need to update the hashes when there are security updates.
 *
 *  * It kills all ad-hoc shell-scripts.
 *
 * That said it's a reasonably simple approach which doesn't have any major
 * downside.
 *
 *
 * Deploying
 * ---------
 *
 * To add the appropriate hashes you could do something like this:
 *
 *    for i in /bin/?* /sbin/?*; do
 *          setfattr -n security.hash -v $(sha1sum $i | awk '{print $1}') $i
 *    done
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
#include <crypto/hash.h>
#include <crypto/sha.h>
#include <crypto/algapi.h>


/*
 * Given a file and a blob of memory calculate the SHA1 hash
 * of the file contents, and store it in the memory.
 *
 * This is a hacky routine, but it does work :)
 *
 */
int calc_sha1_hash(struct file *file, u8 *digest)
{
    struct crypto_shash *tfm;
    struct shash_desc *desc;
    loff_t i_size, offset = 0;
    char *rbuf;
    int rc = 0;

    // The target we're checking
    struct dentry *dentry = file->f_path.dentry;
    struct inode *inode = d_backing_inode(dentry);

    // Allocate the hashing-helper.
    tfm = crypto_alloc_shash("sha1", 0, 0);

    if (IS_ERR(tfm))
    {
        int error = PTR_ERR(tfm);
        printk(KERN_INFO "failed to setup sha1 hasher\n");
        return error;
    }

    // Allocate the description.
    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);

    if (!desc)
    {
        printk(KERN_INFO "Failed to kmalloc desc");
        goto out;
    }

    // Setup the description
    desc->tfm = tfm;
    desc->flags = crypto_shash_get_flags(tfm);

    // Init the hash
    rc = crypto_shash_init(desc);

    if (rc)
    {
        printk(KERN_INFO "failed to crypto_shash_init");
        goto out2;
    }

    // Allocate a buffer for reading the file contents
    rbuf = kzalloc(PAGE_SIZE, GFP_KERNEL);

    if (!rbuf)
    {
        printk(KERN_INFO "failed to kzalloc");
        rc = -ENOMEM;
        goto out2;
    }

    // Find out how big the file is
    i_size = i_size_read(inode);

    // Read it, in page-sized chunks.
    while (offset < i_size)
    {

        int rbuf_len;
        rbuf_len = kernel_read(file, offset, rbuf, PAGE_SIZE);

        if (rbuf_len < 0)
        {
            rc = rbuf_len;
            break;
        }

        if (rbuf_len == 0)
            break;

        offset += rbuf_len;

        rc = crypto_shash_update(desc, rbuf, rbuf_len);

        if (rc)
            break;
    }

    // Free the buffer we used for holding the file-contents
    kfree(rbuf);

    // Finalise the SHA result.
    if (!rc)
        rc = crypto_shash_final(desc, digest);

out2:
    kfree(desc);
out:
    crypto_free_shash(tfm);
    return rc;
}


/*
 * Perform a check of a program execution/map.
 *
 * Return 0 if it should be allowed, -EPERM on block.
 */
static int hashcheck_bprm_check_security(struct linux_binprm *bprm)
{
    u8 *digest;
    int i;
    char *hash = NULL;
    char *buffer = NULL;
    int rc = 0;

    // The current task & the UID it is running as.
    const struct task_struct *task = current;
    kuid_t uid = task->cred->uid;

    // The target we're checking
    struct dentry *dentry = bprm->file->f_path.dentry;
    struct inode *inode = d_backing_inode(dentry);
    int size = 0;

    // Root can access everything.
    if (uid.val == 0)
        return 0;

    // Allocate some RAM to hold the digest result
    digest = (u8*)kmalloc(SHA1_DIGEST_SIZE, GFP_TEMPORARY);

    if (!digest)
    {
        printk(KERN_INFO "failed to allocate storage for digest");
        return 0;
    }

    //
    // We're now going to calculate the hash.
    //
    memset(digest, 0, SHA1_DIGEST_SIZE);
    rc = calc_sha1_hash(bprm->file, digest);

    //
    // Now allocate a second piece of RAM to store the human-readable hash.
    //
    hash = (char*)kmalloc(PAGE_SIZE, GFP_TEMPORARY);

    if (!hash)
    {
        printk(KERN_INFO "failed to allocate storage for digest-pretty");
        rc = -ENOMEM;
        goto out;
    }

    //
    // Create the human-readable result.
    //
    memset(hash, 0, PAGE_SIZE);

    for (i = 0; i < SHA1_DIGEST_SIZE; i++)
    {
        snprintf(hash + (i * 2), 4, "%02x", digest[i]);
    }

    //
    // Allocate the buffer for reading the xattr value
    //
    buffer = kzalloc(PAGE_SIZE, GFP_KERNEL);

    if (buffer == NULL)
    {
        printk(KERN_INFO "failed to allocate buffer for xattr value\n");
        goto out2;
    }

    //
    // Get the xattr value.
    //
    size = __vfs_getxattr(dentry, inode, "security.hash", buffer, PAGE_SIZE - 1);

    //
    // This is the return-result from this function.
    //
    rc = 0;

    //
    // No hash?  Then the execution is denied.
    //
    if (size < 0)
    {
        printk(KERN_INFO "Missing `security.hash` value!\n");
        rc = -EPERM;
    }
    else
    {
        //
        // Using a constant-time comparison see if we got a match.
        //
        if (crypto_memneq(buffer, hash, strlen(hash)) == 0)
        {
            printk(KERN_INFO "Hash of %s matched expected result %s - allowing execution\n", bprm->filename, hash);
            rc = 0;
        }
        else
        {
            printk(KERN_INFO "Hash mismatch for %s - denying execution [%s != %s]\n",  bprm->filename, hash, buffer);
            rc = -EPERM;
        }
    }

    kfree(buffer);

out2:
    kfree(hash);

out:
    kfree(digest);

    return (rc);
}

/*
 * The hooks we wish to be installed.
 */
static struct security_hook_list hashcheck_hooks[] =
{
    LSM_HOOK_INIT(bprm_check_security, hashcheck_bprm_check_security),
};

/*
 * Initialize our module.
 */
static int __init hashcheck_init(void)
{
    /* register ourselves with the security framework */
    security_add_hooks(hashcheck_hooks, ARRAY_SIZE(hashcheck_hooks), "hashcheck");
    printk(KERN_INFO "LSM initialized: hashcheck\n");
    return 0;
}

security_initcall(hashcheck_init);
