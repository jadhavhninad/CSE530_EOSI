#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#include <asm/delay.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>
#include <linux/types.h>

#define DEV_NAME "HCSR_01"


static ktime_t echo_start,echo_end;
int rise_seen=0,fall_seen=0;


//Getting the number of misc hscr devices to be created as an argument during module loading. Setting default as 2.
int hcsr_dev_num = 1,irqid;
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

static irqreturn_t fall_edge(int irq, void *dev_id, struct pt_regs *regs) {
	echo_end = ktime_get();
	fall_seen=1;
	return IRQ_HANDLED;
}

static irqreturn_t rise_edge(int irq, void *dev_id, struct pt_regs *regs) {
        echo_start = ktime_get();
	rise_seen=1;
	return IRQ_HANDLED;
}

//the hcsr_write operation.
static ssize_t hcsr_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
       	printk(KERN_INFO "WRITING GPIO");
	int ret1=0,ret2=0,ret3=0;
	
	//Configure the GPIO for Trigger I/O pin 4
	ret1 = gpio_request(6,"gpio6");  
	ret2 = gpio_request(36,"gpio36");
        if(ret1 != 0 || ret2 != 0){
		printk(KERN_ALERT "GPIO for output is not requested\n");
	}
	else{
	        printk(KERN_ALERT "GPIO for output is requested\n");
	}

	ret1 = gpio_direction_output(6,0);
	ret2 = gpio_direction_output(36,0);
    	if(ret1 != 0 || ret2 != 0){  
	        printk(KERN_ALERT "Direction : GPIO for output in not set output\n");  
	}  
    	else{  
	        printk(KERN_ALERT "Direction : GPIO is set output and out is low\n");  
	}	
	

	//Configure the GPIO for Echo I/O pin 6
        ret1 = gpio_request(1,"gpio1");
        ret2 = gpio_request(20,"gpio20");
	ret3 = gpio_request(68,"gpio68");
        
	if(ret1 != 0 || ret2 != 0 || ret3 !=0){
                printk(KERN_ALERT "GPIO for input pin is not requested\n");
        }
        else{
                printk(KERN_ALERT "GPIO for input pin is requested\n");
        }


	ret1 = gpio_direction_output(20,1);
	ret2 = gpio_direction_output(1,0);
	ret3 = gpio_direction_output(68,0);
        if(ret1 != 0 || ret2 != 0 || ret3 != 0){
                printk(KERN_ALERT "Direction : GPIO for input is not requested\n");
        }
        else{  
                printk(KERN_ALERT "Direction : GPIO for input is requested");
        }
	

	irqid = gpio_to_irq(1);
	if(irqid < 0){
		printk(KERN_INFO "IRQ not registered\n");
		return -1;
	}
	else
		printk(KERN_INFO "IRQ registered");

	
	int error;
	//interrupt handler	
	if ((error = request_irq(irqid, (irq_handler_t) fall_edge, IRQF_TRIGGER_FALLING,DEV_NAME ,NULL))) {
		printk (KERN_ALERT "irq on falling edge got into error!\n");
		return -1;
	}

	//interrupt handler     
        if ((error = request_irq(irqid, (irq_handler_t) rise_edge, IRQF_TRIGGER_RISING,DEV_NAME ,NULL))) {
                printk (KERN_ALERT "irq on falling edge got into error!\n");
                return -1;
        }


	//Set output value
	//Initialize echo as input
	gpio_direction_input(1);
	int i;
	for(i=0;i<5;i++){
		//Rising edge
		gpio_direction_output(6,1);
		udelay(10);
		//Falling edge
		gpio_direction_output(6,0);
		
		while(rise_seen == 0 && fall_seen == 0)
			udelay(2);
		
		uint64_t duration = ktime_to_ns(ktime_sub(echo_end,echo_start)); 
		rise_seen=0, fall_seen=0;		
		uint64_t dist = (duration*0.034)/2;
		//int echoVal = gpio_get_value(1);
		printk(KERN_INFO "Distance = %lld\n",dist);
		printk(KERN_INFO " Sample %d collected\n",i);	
		udelay(50);
	}
		
	printk (KERN_ALERT "Value of output set to High\n");
	return 0;
}

//the HCSR_read operation
static ssize_t hcsr_read(struct file *file, char *buf, size_t count, loff_t *ppos){
	printk(KERN_INFO "HCSR READ operation");
	int read_val = gpio_get_value(1);
	printk(KERN_ALERT "Value read for input pin 1 is %d\n",read_val);
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
	.name = DEV_NAME,
	.fops = &hcsr_fops,
};


//Initialize the misc_devices;
static int __init misc_init(void)
{
	int error;
	//iterator;
	//char hcsr_name[10];
	
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
	gpio_free(6);
	gpio_free(36);
	gpio_free(1);
	gpio_free(20);
	gpio_free(68);
	free_irq(irqid,NULL);
}

module_init(misc_init)
module_exit(misc_exit)
MODULE_DESCRIPTION("HCSR_04 misc devices");
MODULE_AUTHOR("CSE530_TEAM29");
MODULE_LICENSE("GPL");
