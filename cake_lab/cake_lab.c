static int __init cake_lab_init(void) {
    printk(KERN_INFO "I present you with cake lets eat!\n");
    return 0;
}

static void __exit cake_lab_exit(void) {
    printk(KERN_INFO "I hope it was delicious!\n");
}

module_init(cake_lab_init);
module_exit(cake_lab_exit);