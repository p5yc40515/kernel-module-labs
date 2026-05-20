#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#define CAKE_LAB_FAKE_LEN 1500
#define CAKE_LAB_FAKE_MEM 3000


struct cake_lab_class;
struct cake_lab_child;

struct cake_lab_parent {
    struct list_head active_classes;
    int qlen;
    int backlog;
    spinlock_t lock;
    struct cake_lab_class *class;

};

static struct cake_lab_parent *the_parent;

struct cake_lab_class {
    int classid;
    bool active;
    bool deleted;
    struct cake_lab_child *child;
    struct list_head active_node;
  
};

struct cake_lab_child {
    int qlen;
    int backlog;
    int buffer_used;
    int buffer_limit;
    int drops;
    
};

static struct cake_lab_child *child_create(void) {
    struct cake_lab_child *child = kzalloc(sizeof(struct cake_lab_child), GFP_KERNEL);
    if (!child) {
        printk(KERN_ERR "We didn't allocate anything!\n");
        return NULL;
    }
    child->qlen = 0;
    child->backlog = 0;
    child->buffer_used = 0;
    child->buffer_limit = 2000;
    child->drops = 0;
    return child;
}

static void child_destroy(struct cake_lab_child *child) {
    if (child == NULL) {
        printk(KERN_ERR "No allocation to free!\n");
        return;
    }
    kfree(child);
}

static struct cake_lab_class *class_create(void) {
    struct cake_lab_class *cl = kzalloc(sizeof(struct cake_lab_class), GFP_KERNEL);
    if (!cl) {
        printk(KERN_ERR "We didn't allocate anything!\n");
        return NULL;
    }
    cl->classid = 1;
    cl->active = false;
    cl->deleted = false;
    INIT_LIST_HEAD(&cl->active_node);
    cl->child = child_create();
    if (!cl->child) {
        kfree(cl);
        return NULL;
    }
    return cl;
}

static void class_destroy(struct cake_lab_class *cl) {
    if (cl == NULL) {
        printk(KERN_ERR "Nothing to free!\n");
        return;
    }
    child_destroy(cl->child);
    cl->child = NULL;
    kfree(cl);
}

static struct cake_lab_parent *parent_create(void) {
    struct cake_lab_parent *parent = kzalloc(sizeof(struct cake_lab_parent), GFP_KERNEL);
    if (!parent) {
        printk(KERN_ERR "Still no allocation!\n");
        return NULL;
    }
    INIT_LIST_HEAD(&parent->active_classes);
    parent->qlen = 0;
    parent->backlog = 0;
    spin_lock_init(&parent->lock);
    parent->class = class_create();
    if (!parent->class) {
        kfree(parent);
        return NULL;
    }
    return parent;
}

static void parent_destroy(struct cake_lab_parent *parent) {
    if (parent == NULL) {
        printk(KERN_ERR "Nothing to free!\n");
        return;
    }
    class_destroy(parent->class);
    parent->class = NULL;
    kfree(parent);
}

static void dump_state(struct cake_lab_parent *parent) {
    
    struct cake_lab_class *cl;
    struct cake_lab_child *child;
    if (parent == NULL) {
        printk(KERN_ERR "No parent to dump!\n");
        return ;
    }
    printk(KERN_INFO "Parent qlen: %d, backlog: %d\n", parent->qlen, parent->backlog);
    
    cl = parent->class;
    if (cl == NULL) {
        printk(KERN_ERR "No class to dump!\n");
        return;
    }
    printk(KERN_INFO "Class id: %d, active: %d, deleted: %d\n", cl->classid, cl->active, cl->deleted);
    
    child = cl->child;
    if (child == NULL) {
        printk(KERN_ERR "No child to dump!\n");
        return;
    }
    printk(KERN_INFO "Child qlen: %d, Child backlog: %d, buffer used: %d, buffer limit: %d, drops: %d\n", child->qlen, child->backlog, child->buffer_used, child->buffer_limit, child->drops);
}

static int child_enqueue(struct cake_lab_child *child) {
    if (child == NULL) {
        printk(KERN_ERR "No child to enqueue right now!\n");
        return -EINVAL;
    }
    child->qlen += 1;
    child->backlog += CAKE_LAB_FAKE_LEN;
    child->buffer_used += CAKE_LAB_FAKE_MEM;
    printk(KERN_INFO "Enqueued a packet! Child qlen: %d, Child backlog: %d, buffer used: %d\n", child->qlen, child->backlog, child->buffer_used);
    if (child->buffer_used > child->buffer_limit) {
        child->qlen -= 1;
        child->backlog -= CAKE_LAB_FAKE_LEN;
        child->buffer_used -= CAKE_LAB_FAKE_MEM;
        child->drops += 1;
        printk(KERN_INFO "Packet has been dropped! Total : %d\n", child->drops);
        printk(KERN_INFO "Child buffer limit exceeded! Buffer used: %d, Buffer limit: %d\n", child->buffer_used, child->buffer_limit);
        return 0;
    }

    return 0;

}

static int parent_enqueue(struct cake_lab_parent *parent) {
    
}

static int __init cake_lab_init(void) {
    
    the_parent = parent_create();
    if (!the_parent) {
        printk(KERN_ERR "Didn't work out!\n");
        return -ENOMEM;
    }
    dump_state(the_parent);
    child_enqueue(the_parent->class->child);
    dump_state(the_parent);
    
    printk(KERN_INFO "I present you with cake lets eat!\n");
    return 0;
}

static void __exit cake_lab_exit(void) {
    
    parent_destroy(the_parent);
    the_parent = NULL;
    printk(KERN_INFO "I hope it was delicious!\n");
}

module_init(cake_lab_init);
module_exit(cake_lab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("p5y");
MODULE_DESCRIPTION("Linux kernel module that will simulate cake qdisc uaf vulnerability");