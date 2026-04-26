#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/string.h>

struct oob_object {
    char data[64]; // Buffer to hold data, intentionally small for OOB demonstration
};
static struct dentry *oob_dir;
static struct dentry *oob_file;
static struct oob_object *obj;

static ssize_t trigger_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) {

    unsigned int idx;
    char ch;
    char val;

    char kbuf[64];

    size_t n = min(len, sizeof(kbuf) -1); 

    if (copy_from_user(kbuf, buf, n)) {
        return -EFAULT;
    }

    kbuf[n] = '\0'; // Null-terminate the buffer
    pr_info("oob lab received: %s\n", kbuf);

    if (strncmp(kbuf, "alloc", 5) == 0) {
        if (obj != NULL) {
            pr_info("oob lab: object allocataed already \n");
        } else {
            obj = kmalloc(sizeof(*obj), GFP_KERNEL);
            if (!obj) {
                pr_err("oob_lab: didn't allocate memory \n");
                return -ENOMEM;
            }
            memset(obj->data, 'A', sizeof(obj->data));
        }
    } else if (strncmp(kbuf, "free", 4) == 0) {
        if (obj == NULL) {
            pr_info("oob_lab: You didn't allocate memory yet \n");
        } else {
            kfree(obj);
            obj = NULL;
            pr_info("oob_lab: You freed the memory \n");
        }
    } else if (sscanf(kbuf, "read %u", &idx) ==1) {
        if (obj == NULL) {
            pr_info("oob_lab: Memory not allocated yet \n");
        } else if (idx >= sizeof(obj->data)) {
            pr_info("oob_lab: It is out of bounds read \n");
        } else {
            val = obj->data[idx];
            pr_info("oob_lab: Read data[%u] = %c \n", idx, val);
        }
    } else if (sscanf(kbuf, "write %u %c", &idx, &ch) == 2) {
        if (obj == NULL) {
            pr_info("oob_lab: Memory not allocated yet \n");
        } else if (idx >= sizeof(obj->data)) {
            pr_info("oob_lab: It is out of bounds write \n");
        } else {
            obj->data[idx] = ch;
            pr_info("oob_lab: Wrote data[%u] = %c \n", idx, ch);
        }
    } else if (sscanf(kbuf, "oobread %u", &idx) ==1) {
        if (obj == NULL) {
            pr_info("oob_lab: Memory not allocated yet \n");
        } else {
            val = *((char *)obj + sizeof(*obj) + idx); // Intentional OOB read
            pr_info("oob_lab: OOB Read data at offset %u = %c \n", idx, val);
        }
    }
     else {
        pr_info("oob_lab: Not a valid command \n");
    }

    return len; // Return the number of bytes written
}

static const struct file_operations oob_fops = {
    .owner = THIS_MODULE,
    .write = trigger_write,
};

//Initialize the module
static int __init oob_lab_init(void) {

    obj = NULL;

    oob_dir = debugfs_create_dir("oob_lab", NULL);

    if (!oob_dir) {
      pr_err("oob_lab: didn't create debugfs directory \n");
      return -ENOMEM;  
    }

    oob_file = debugfs_create_file("trigger", 0200, oob_dir, NULL, &oob_fops);

    if (!oob_file) {
        debugfs_remove_recursive(oob_dir);
        return -ENOMEM;
    }

    pr_info("OOB Lab: Module loaded\n");
    return 0;
}
//Exit the module
static void __exit oob_lab_exit(void) {
    
    if (obj != NULL) {
        kfree(obj);
        obj = NULL;
    }

    debugfs_remove_recursive(oob_dir);

    pr_info("OOB Lab: Module leaving\n");
}

module_init(oob_lab_init);
module_exit(oob_lab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("p5yc40515");
MODULE_DESCRIPTION("Out of bounds practice lab");