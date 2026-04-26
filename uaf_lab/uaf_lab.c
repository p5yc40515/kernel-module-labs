#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/types.h>

struct uaf_object {
    char data[64];
};

static struct uaf_object *obj1;
static struct uaf_object *obj2;

static bool obj1_freed;

static struct dentry *uaf_dir;
static struct dentry *uaf_file;

static void reset_lab_state(void)
{
    if (obj1 && !obj1_freed) {
        kfree(obj1);
    }

    if (obj2) {
        kfree(obj2);
    }

    obj1 = NULL;
    obj2 = NULL;
    obj1_freed = false;
}

static ssize_t trigger_write(struct file *file,
                             const char __user *buf,
                             size_t len,
                             loff_t *ppos)
{
    char kbuf[64];
    size_t n = len;
    char value;

    if (n > sizeof(kbuf) - 1)
        n = sizeof(kbuf) - 1;

    if (copy_from_user(kbuf, buf, n))
        return -EFAULT;

    kbuf[n] = '\0';

    if (strncmp(kbuf, "alloc", 5) == 0) {
        if (obj1 && !obj1_freed) {
            pr_info("uaf_lab: obj1 already allocated at %px\n", obj1);
        } else {
            obj1 = kmalloc(sizeof(*obj1), GFP_KERNEL);
            if (!obj1)
                return -ENOMEM;

            memset(obj1->data, 0, sizeof(obj1->data));
            obj1_freed = false;
            pr_info("uaf_lab: obj1 allocated at %px\n", obj1);
        }

    } else if (sscanf(kbuf, "fill %c", &value) == 1) {
        if (!obj1 || obj1_freed) {
            pr_info("uaf_lab: cannot fill obj1 (missing or freed)\n");
        } else {
            memset(obj1->data, value, sizeof(obj1->data));
            pr_info("uaf_lab: obj1 filled with '%c' at %px\n", value, obj1);
        }

    } else if (strncmp(kbuf, "free", 4) == 0) {
        if (!obj1) {
            pr_info("uaf_lab: obj1 is NULL, nothing to free\n");
        } else if (obj1_freed) {
            pr_info("uaf_lab: obj1 already freed, dangling ptr=%px\n", obj1);
        } else {
            pr_info("uaf_lab: freeing obj1 at %px\n", obj1);
            kfree(obj1);
            obj1_freed = true;
            pr_info("uaf_lab: obj1 freed, dangling ptr still %px\n", obj1);
        }

    } else if (strncmp(kbuf, "use", 3) == 0) {
        if (!obj1) {
            pr_info("uaf_lab: obj1 is NULL\n");
        } else {
            pr_info("uaf_lab: using obj1 ptr=%px freed=%d first_byte='%c' (0x%02x)\n",
                    obj1, obj1_freed, obj1->data[0], (unsigned char)obj1->data[0]); // Note: accessing obj1->data[0] after free is intentional for the lab, but in a real kernel module this would be unsafe and should be avoided.
        }

    } else if (sscanf(kbuf, "uaf_write %c", &value) == 1) {
        if (!obj1) {
            pr_info("uaf_lab: obj1 is NULL\n");
        } else if (!obj1_freed) {
            pr_info("uaf_lab: obj1 is not freed yet, refusing stale-write test\n");
        } else {
            obj1->data[0] = value;
            pr_info("uaf_lab: wrote '%c' through dangling obj1 ptr=%px\n",
                    value, obj1);
        }

    } else if (sscanf(kbuf, "alloc2 %c", &value) == 1) {
        if (obj2) {
            pr_info("uaf_lab: obj2 already allocated at %px\n", obj2);
        } else {
            obj2 = kmalloc(sizeof(*obj2), GFP_KERNEL);
            if (!obj2)
                return -ENOMEM;

            memset(obj2->data, value, sizeof(obj2->data));
            pr_info("uaf_lab: obj2 allocated at %px and filled with '%c'\n",
                    obj2, value);
        }

    } else if (strncmp(kbuf, "check2", 6) == 0) {
        if (!obj2) {
            pr_info("uaf_lab: obj2 is NULL\n");
        } else {
            pr_info("uaf_lab: obj2 ptr=%px first_byte='%c' (0x%02x)\n",
                    obj2, obj2->data[0], (unsigned char)obj2->data[0]);
        }

    } else if (strncmp(kbuf, "reset", 5) == 0) {
        reset_lab_state();
        pr_info("uaf_lab: lab state reset\n");

    } else {
        pr_info("uaf_lab: unknown command '%s'\n", kbuf);
    }

    return len;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = trigger_write,
};

static int __init uaf_lab_init(void)
{
    obj1 = NULL;
    obj2 = NULL;
    obj1_freed = false;

    uaf_dir = debugfs_create_dir("uaf_lab", NULL);
    if (!uaf_dir)
        return -ENOMEM;

    uaf_file = debugfs_create_file("trigger", 0200, uaf_dir, NULL, &fops);
    if (!uaf_file) {
        debugfs_remove_recursive(uaf_dir);
        return -ENOMEM;
    }

    pr_info("uaf_lab: loaded\n");
    return 0;
}

static void __exit uaf_lab_exit(void)
{
    reset_lab_state();
    debugfs_remove_recursive(uaf_dir);
    pr_info("uaf_lab: unloaded\n");
}

module_init(uaf_lab_init);
module_exit(uaf_lab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("p5yc40515");
MODULE_DESCRIPTION("Cleaner UAF training lab");