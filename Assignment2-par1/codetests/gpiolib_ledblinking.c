#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
//#include <linux/timer.h>
//#include <linux/unistd.h>
#include <linux/delay.h>

//Getting the number of misc hscr devices to be created as an argument during module loading. Setting default as 2.
int hcsr_dev_num = 1;
module_param (hcsr_dev_num, int, S_IRUGO);

//char hcsr_name[10];

//the hcsr_open operation
static int hcsr_open(struct inode *inode, struct file *file)
{
	    pr_info("I have been awoken\n");
	        return 0;
}


//the hcsr_write operation
static int hcsr_close(struct inode *inodep, struct file *filp)
{
	    pr_info("Sleepy time\n");
	        return 0;
}


//the hcsr_write operation.
static ssize_t hcsr_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
       	printk(KERN_INFO "WRITING GPIO");
	int ret1=0,ret2=0,ret3=0;

	//Configure the GPIO for I/O pin 2
	ret1 = gpio_request(13,"gpio13");  
	ret2 = gpio_request(34,"gpio34");
	ret3 = gpio_request(77,"gpio77");
        if(ret1 != 0 || ret2 != 0 || ret3 != 0){
		printk(KERN_ALERT "GPIO for output is not requested\n");
	}
	else{
	        printk(KERN_ALERT "GPIO for output is requested\n");
	}

	ret1 = gpio_direction_output(13,0);
	ret2 = gpio_direction_output(34,0);
    	//ret3 = gpio_direction_output(77,0);  
    	if(ret1 != 0 || ret2 != 0){  
	        printk(KERN_ALERT "Direction : GPIO for output in not set output\n");  
	}  
    	else{  
	        printk(KERN_ALERT "Direction : GPIO is set output and out is low\n");  
	}	
	
	printk(KERN_INFO "setting values for input\n");
	//gpio_set_value(77,0);
	//gpio_set_value(34,0);
	//gpio_set_value(13,0);	


	//Configure the GPIO for input I/O pin 4
        ret1 = gpio_request(6,"gpio6");
        ret2 = gpio_request(36,"gpio36");
        //NO mux for the pin 4
	if(ret1 != 0 || ret2 != 0){
                printk(KERN_ALERT "GPIO for input pin is not requested\n");
        }
        else{
                printk(KERN_ALERT "GPIO for input pin is requested\n");
        }


	ret1 = gpio_direction_output(36,0);
	ret2 = gpio_direction_input(6);
        if(ret1 != 0 || ret2 != 0){
                printk(KERN_ALERT "Direction : GPIO for input is not requested\n");
        }
        else{  
                printk(KERN_ALERT "Direction : GPIO for input is requested");
        }
	
	//gpio_set_value(36,0);

	//Set output value
	int i;
	for(i=0;i<10;i++){
		gpio_direction_output(13,1);
		msleep(1000);	
		gpio_direction_output(13,0);	
		msleep(1000);	
	}
		
	printk (KERN_ALERT "Value of output set to High\n");
	gpio_export(13,true);
	gpio_export(34,true);
	gpio_export(77,true);
	return 0;
}

//the HCSR_read operation
static ssize_t hcsr_read(struct file *file, char *buf, size_t count, loff_t *ppos){
	printk(KERN_INFO "HCSR READ operation");
	int read_val = gpio_get_value(6);
	printk(KERN_ALERT "Value read for input pin 4 is %d\n",read_val);
	return 0;
}


static long hcsr_ioctl(struct file *file, unsigned int ioctl_num, unsigned long val){
	printk(KERN_INFO "HCSR IOCTL operation");
	return 0;
}

static const struct file_operations hcsr_fops = {
	.owner			= THIS_MODULE,
	.write			= hcsr_write,
	.open			= hcsr_open,
        .release		= hcsr_close,
	.read 			= hcsr_read,
	.unlocked_ioctl		= hcsr_ioctl,
};


static struct miscdevice hcsr_dev={
	.minor = MISC_DYNAMIC_MINOR,    //Assign the minor number dynamically.
	//sprintf(hcsr_name, "HSCR_%d", hcsr_dev_num),        //Generate a name in HSCR_n format
	.name = "HCSR_01",
	.fops = &hcsr_fops,
};


//Initialize the misc_devices;
static int __init misc_init(void)
{
	int error,iterator;
	char hcsr_name[10];
	
	//for(iterator=0;iterator < hcsr_dev_num;iterator++){
	//	static struct miscdevice hcsr_dev={};
	//	hcsr_dev.minor = MISC_DYNAMIC_MINOR;    //Assign the minor number dynamically.
		
	//	sprintf(hcsr_name, "HSCR_%d", iterator);	//Generate a name in HSCR_n format
	//	hcsr_dev.name = hcsr_name;
	//	hcsr_dev.fops = &hcsr_fops;
	
		error = misc_register(&hcsr_dev);
		if (error) {
        		pr_err("can't misc_register :(\n");
        		return error;
        //	}
	}

	pr_info("I'm in\n");
	return 0;
}


//exit all the misc_devices.
static void __exit misc_exit(void)
{
	misc_deregister(&hcsr_dev);
	pr_info("I'm out\n");
	//gpio_free(13);
	//gpio_free(34);
	//gpio_free(77);
	gpio_free(6);
	gpio_free(36);
}

module_init(misc_init)
module_exit(misc_exit)
MODULE_DESCRIPTION("HCSR_04 misc devices");
MODULE_AUTHOR("CSE530_TEAM29");
MODULE_LICENSE("GPL");
