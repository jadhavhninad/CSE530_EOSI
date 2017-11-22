#include <linux/rbtree.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define DEVICE_NAME "rbt530_dev"  // device name to be created and registered
#define DEVICE_CLASS "eosi_class"     //the device class as shown in /sys/class
#define DEVICES 1


static dev_t rb_dev_number;
static struct class *rb_dev_data;
static struct device *rb_dev_device;

static int __init rbt530_driver_init(void);
static void __exit rbt530_driver_exit(void);
static int rb_open(struct inode *, struct file *);
static int rb_release(struct inode *, struct file *);
static ssize_t rb_read(struct file *, char *, size_t, loff_t *);
static ssize_t rb_write(struct file *, const char *, size_t, loff_t *);
struct rb_node *rb_search(struct rb_root *, int);
int rb_insert(struct rb_root *, int, int);
static long rb_ioctl(struct file *, unsigned int , unsigned long);
//struct mutex rb_mutex;
static DEFINE_MUTEX(rb_mutex);

//mutex_init(&rb_mutex);

typedef struct rb_object {
	struct rb_node my_node;
	int key;
	int data;
}rb_object_t;

struct rb_dev {
        struct cdev cdev;               /* The cdev structure */
        char name[20];                  /* Name of device*/
	char input[2];
	struct rb_root rbt530;
	long set_cmd;
}*rb_devp;


int rb_open(struct inode *inode, struct file *file)
{
	//initialize mutex
	mutex_init(&rb_mutex);
	//mutex lock
	mutex_lock(&rb_mutex);
	
	struct rb_dev *rb_devp;

	printk("\nopening\n");
		
	// Get the per-device structure that contains this cdev */
	rb_devp = container_of(inode->i_cdev, struct rb_dev, cdev);
		
	//Now we can access the driver struct elements.
	file->private_data = rb_devp;
	printk("\n%s is opening \n", rb_devp->name);
	return 0;
}

 
/*
  * Release the driver
  */
int rb_release(struct inode *inode, struct file *file)
{
	struct rb_dev *rb_devp = file->private_data;
	printk("\n%s is closing\n", rb_devp->name);
	
	//release mutex.
	mutex_unlock(&rb_mutex);
	return 0;
}


long rb_ioctl(struct file *file, unsigned int ioctl_num, unsigned long val)
{
	struct rb_dev *rb_devp = file->private_data;
	rb_devp->set_cmd = ioctl_num;
	return 0;
}


ssize_t rb_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
        struct rb_dev *rb_devp = file->private_data;
	int wr_err=0; 
	wr_err = copy_from_user(rb_devp->input,buf,count);
	
	if(wr_err!=0){
		printk(KERN_INFO "Error while writing data.\n");
		//remove mutex
		return -1;
	}
	
	int keyVal, dataVal;
	keyVal = rb_devp->input[0] - '0';
	dataVal = rb_devp->input[1] - '0';
	
	/*
	struct rb_node *newentry=NULL;
        rb_object_t *my_rbtree = container_of(newentry, rb_object_t, my_node);
	my_rbtree->key = keyVal;
	my_rbtree->data = dataVal;
	*/


	struct rb_object *my_rbtree = (struct rb_object *)kmalloc(sizeof(struct rb_object), GFP_KERNEL);
	my_rbtree->key = keyVal;
	my_rbtree->data = dataVal;
	//my_rbtree->my_node = (struct rb_node)RB_ROOT;


	struct rb_node *searchVal = rb_search(&rb_devp->rbt530,keyVal);
	

	if(dataVal == 1){
		if(searchVal) {
			printk("Node replaced");
			rb_replace_node(searchVal, &my_rbtree->my_node, &rb_devp->rbt530);
		}
		else	
			rb_insert(&rb_devp->rbt530,keyVal, dataVal);
	}
	if(dataVal == 0){
		//Erase a node. What to do if the node is root? kernel. Found that rb_erase handles rebalancing
		rb_erase(searchVal,&rb_devp->rbt530);
	}

	return 0;
}


//Search a node
struct rb_node *rb_search(struct rb_root *root, int keytoFind){


	struct rb_node *walker = root->rb_node;

	while(walker){
	
		rb_object_t *structdata = container_of(walker, rb_object_t, my_node);
		int cmpresult;

		cmpresult = keytoFind > structdata->key ? 1 : 0 ;
		
		if(cmpresult == 0)
			walker = walker->rb_left;
		else if (cmpresult == 1)
			walker = walker->rb_right;
		else
			return &structdata->my_node;
		}
	return NULL;
}


//Insert a node
int rb_insert(struct rb_root *root, int keyVal, int dataVal){

	struct rb_node **new = &(root->rb_node), *parent=NULL;
	
	//create a new node
	
	struct rb_object *my_rbtree = (struct rb_object *)kmalloc(sizeof(struct rb_object), GFP_KERNEL);	
	my_rbtree->key = keyVal;
	my_rbtree->data = dataVal;
//	my_rbtree->my_node = RB_ROOT;
	
	while(*new){
	
		struct rb_object *this = container_of(*new, rb_object_t, my_node);	
		

		int cmpresult= my_rbtree->key > this->key ? 1 : 0;	
		
		if(cmpresult == 0)
			new =&((*new)->rb_left);
		else
			new= &((*new)->rb_right);
	
	}

	
	rb_link_node(&my_rbtree->my_node, parent, new);
	rb_insert_color(&my_rbtree->my_node,root);

	return 0;

}


//Read a node value
ssize_t rb_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
        struct rb_dev *rb_devp = file->private_data;
	struct rb_node *seeker = rb_devp->rbt530.rb_node;        
	int result;

	if(rb_devp->set_cmd==1){
	//traverse right till last element is reached
		while(seeker->rb_right){
	                seeker = seeker->rb_right;
                }
	
	//return its value
        rb_object_t *structdata = container_of(seeker, rb_object_t, my_node);
	result = structdata->key;
	
	}
	else if(rb_devp->set_cmd==0){
		while(seeker->rb_left){
			seeker = seeker->rb_left;
		  }
        rb_object_t *structdata = container_of(seeker, rb_object_t, my_node);
        result = structdata->key;
	}
	else
		result = -1;

        /* 
         * Actually put the data into the buffer 
         *
        while (count && rb_devp->input[bytes_read]) {
                put_user(rd_devp->input[bytes_read], buf++);
                count--;
                bytes_read++;
        }

        * 
         * Most read functions return the number of bytes put into the buffer, we canc directlt return the dat,
	 * instead of using the buffer. Try if no buffer works.
         */

	return result;

}


/* File operations structure. Defined in linux/fs.h */
static struct file_operations rb_fops = {
	.owner              = THIS_MODULE,           /* Owner */
        .open               = rb_open,        /* Open method */
	.unlocked_ioctl     = rb_ioctl,     /* IOCTL for driver rbtree */
        .release            = rb_release,     /* Release method */
        .write              = rb_write,       /* Write method */
        .read               = rb_read,        /* Read method */
};



int __init rbt530_driver_init(void)
{
        int ret;

        /* Request dynamic allocation of a device major number */
        if (alloc_chrdev_region(&rb_dev_number, 0, 1, DEVICE_NAME) < 0) {
                        printk(KERN_DEBUG "Can't register device\n"); return -1;
        }

        /* Populate sysfs entries */
        rb_dev_data = class_create(THIS_MODULE, DEVICE_CLASS);

        /* Allocate memory for the per-device structure */
        rb_devp = kmalloc(sizeof(struct rb_dev), GFP_KERNEL);

        if (!rb_devp) {
                printk("Bad Kmalloc\n"); return -ENOMEM;
        }

        /* Request I/O region */
        sprintf(rb_devp->name, DEVICE_NAME);

        /* Connect the file operations with the cdev */
        cdev_init(&rb_devp->cdev, &rb_fops);
        rb_devp->cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&rb_devp->cdev, (rb_dev_number), 1);

        if (ret) {
                printk("Bad cdev\n");
                return ret;
        }

        /* Send uevents to udev, so it'll create /dev nodes */
        rb_dev_device = device_create(rb_dev_data, NULL, MKDEV(MAJOR(rb_dev_number), 0), NULL, DEVICE_NAME);    

        rb_devp->rbt530=RB_ROOT;

        printk("RB Tree driver initialized.\n");
        return 0;
}


void __exit rbt530_driver_exit(void)
{
        /* Release the major number */
        unregister_chrdev_region((rb_dev_number), 1);
		
        /* Destroy device */
        device_destroy (rb_dev_data, MKDEV(MAJOR(rb_dev_number), 0));
        cdev_del(&rb_devp->cdev);
        kfree(rb_devp);
	
        /* Destroy driver_class */
        class_destroy(rb_dev_data);
			
        printk("rbt530_dev driver removed.\n");
}


module_init(rbt530_driver_init);
module_exit(rbt530_driver_exit);
MODULE_LICENSE("GPL v2");

