#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

static int device_open(struct inode *inode, struct file *filp) {
	printk("%s.%d\n", __func__, __LINE__);
	return 0;
}

static int device_release(struct inode *inode, struct file *filp) {
	printk("%s.%d\n", __func__, __LINE__);
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t len, loff_t *off) {
	printk("%s.%d: length %zd\n", __func__, __LINE__, len);
	return len;
}

static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	printk("%s.%d: cmd 0x%x, arg 0x%lx\n", __func__, __LINE__, cmd, arg);
	return -EINVAL;
}

static struct file_operations fops = {
  .open = device_open,
  .release = device_release,
  .read = device_read,
  .unlocked_ioctl = device_ioctl,
};

static int major;
static int init(void) {
	major = register_chrdev(0, "testdev", &fops);
	printk("%s.%d: major %d\n", __func__, __LINE__, major);
	if (major < 0)
		return major;
	return 0;
}

static void cleanup(void) {
	printk("%s.%d\n", __func__, __LINE__);
	unregister_chrdev(major, "testdev");
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
