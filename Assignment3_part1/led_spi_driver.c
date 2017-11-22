#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
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
#include <linux/ioctl.h>


#define IOCTL_SET_END _IOR('k',1, int)
#define DEVICE_NAME "WS2812"
#define DEVICE_CLASS "led_spi_class"
#define LED_SPIDEV_MAJOR 500 // Change this
#define N_SPI_MINORS			32	/* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);
//static dev_t led_spi_device_number;
//static struct  device *led_spi_device;
//static struct cdev led_cdev;
static struct class *led_spi_class;

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static unsigned bufsiz = 4096;


struct led_spi_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*tx_buffer;
	u32			speed_hz;
};

static ssize_t led_spi_async(struct led_spi_data *, struct spi_message *);


/*File operations go here*/

static int led_spi_open(struct inode *inode, struct file *filp)
{
	struct led_spi_data	*ledspi;
	int			status = -ENXIO;
	mutex_lock(&device_list_lock);
	list_for_each_entry(ledspi, &device_list, device_entry) {
		if (ledspi->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status) {
		pr_debug("ledspi: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}
	if (!ledspi->tx_buffer) {
		ledspi->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!ledspi->tx_buffer) {
			dev_dbg(&ledspi->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_find_dev;
		}
	}
	
	ledspi->users++;
	filp->private_data = ledspi;
	nonseekable_open(inode, filp);
	mutex_unlock(&device_list_lock);
	return 0;

	err_find_dev:
		mutex_unlock(&device_list_lock);
		return status;
}






static ssize_t led_spi_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *f_pos)
{
	struct led_spi_data	*ledspi;
	ssize_t			status = 0;
	//unsigned long		missing;
	int k=8,itr,p=0,i,lim;
	//struct spi_transfer; - unable to resolve 	
	struct spi_message m;

	if (count > bufsiz)
		return -EMSGSIZE;
	
	ledspi = filp->private_data;

	mutex_lock(&ledspi->buf_lock);
	
	for(itr=0;itr<3;itr++){
		p=0;
		lim = k+8;
		for(i=k;i<lim;k++){
			ledspi->tx_buffer[p] = buf[k];
			p++;
		}
		//status = spidev_sync_write(spidev, count);
		struct spi_transfer t= {
			.tx_buf		= ledspi->tx_buffer,
			.len		= 8, //transferring 8 bits at a time.
			.speed_hz	= ledspi->speed_hz,
		};
		
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		return led_spi_async(ledspi, &m);
	}

	mutex_unlock(&ledspi->buf_lock);
	return status;
}


static ssize_t led_spi_async(struct led_spi_data *ledspi, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	struct spi_device *spi;
	spin_lock_irq(&ledspi->spi_lock);
	spi = ledspi->spi;
	spin_unlock_irq(&ledspi->spi_lock);
	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spi, message);
	if (status == 0)
		status = message->actual_length;
	return status;
}


static int led_spi_release(struct inode *inode, struct file *filp)
{
	struct led_spi_data	*ledspi;
	mutex_lock(&device_list_lock);
	ledspi = filp->private_data;
	filp->private_data = NULL;
	/* last close? */
	ledspi->users--;
	if (!ledspi->users) {
		int		dofree;
		kfree(ledspi->tx_buffer);
		ledspi->tx_buffer = NULL;
		spin_lock_irq(&ledspi->spi_lock);
		if (ledspi->spi)
			ledspi->speed_hz = ledspi->spi->max_speed_hz;

		/* ... after we unbound from the underlying device? */
		dofree = (ledspi->spi == NULL);
		spin_unlock_irq(&ledspi->spi_lock);
		if (dofree)
			kfree(ledspi);
	}
	mutex_unlock(&device_list_lock);
	return 0;
}





static long led_spi_ioctl(struct file *file, unsigned int ioctl_num, unsigned long val){
	printk(KERN_INFO "LED_SPI : Driver: ioctl()\n");
	return 0;
}

static const struct file_operations led_spi_fops = {
	.owner =	THIS_MODULE,
	.write =	led_spi_write,
	//.read =		led_spi_read,
	.unlocked_ioctl = led_spi_ioctl,
	//.compat_ioctl = led_spi_compat_ioctl,
	.open =		led_spi_open,
	.release =	led_spi_release,
	//.llseek =	no_llseek,
};


/*check this
//----------------
#ifdef CONFIG_OF
static const struct of_device_id spidev_dt_ids[] = {
		{ .compatible = "rohm,dh2228fv" },
			{ .compatible = "lineartechnology,ltc2488" },
				{},
};
MODULE_DEVICE_TABLE(of, spidev_dt_ids);
#endif
*/

static int led_spi_probe(struct spi_device *spi){
	struct led_spi_data	*ledspi;
	int			status;
	unsigned long		minor;
	
	//if (spi->dev.of_node && !of_match_device(spidev_dt_ids, &spi->dev)) {
	//	dev_err(&spi->dev, "buggy DT: spidev listed directly in DT\n");
	//	WARN_ON(spi->dev.of_node &&
	//	!of_match_device(spidev_dt_ids, &spi->dev));
	//}

	/* Allocate driver data */
	ledspi = kzalloc(sizeof(*ledspi), GFP_KERNEL);
	if (!ledspi)
		return -ENOMEM;
	
	/* Initialize the driver data */
	ledspi->spi = spi;
	spin_lock_init(&ledspi->spi_lock);
	mutex_init(&ledspi->buf_lock);
	
	INIT_LIST_HEAD(&ledspi->device_entry);
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;
		ledspi->devt = MKDEV(LED_SPIDEV_MAJOR, minor);
		dev = device_create(led_spi_class, &spi->dev, ledspi->devt, ledspi, "WS2812%d.%d", spi->master->bus_num, spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&ledspi->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);
	ledspi->speed_hz = spi->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi, ledspi);
	else
		kfree(ledspi);
	return status;
}


static int led_spi_remove(struct spi_device *spi)
{
	struct led_spi_data	*ledspi = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&ledspi->spi_lock);
	ledspi->spi = NULL;
	spin_unlock_irq(&ledspi->spi_lock);
	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&ledspi->device_entry);
	device_destroy(led_spi_class, ledspi->devt);
	clear_bit(MINOR(ledspi->devt), minors);
	if (ledspi->users == 0)
		kfree(ledspi);
	mutex_unlock(&device_list_lock);
	return 0;
}


//---------------


static struct spi_driver led_spi_driver = {
	.driver = {
		.name = DEVICE_NAME,
	//	.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	
	.probe = led_spi_probe,
	.remove = led_spi_remove,
};

/*-------------------------------------------------------------------------*/

static int __init led_spi_init(void)
{
	int status = 0;
	
	/*
	 * DYNAMIC ALLOCATION IS GIVING ERROR.
	//dynamically allocate the MAJOR number.
	if(alloc_chardev_region(&led_spi_device_number, 0, 1, DEVICE_NAME) < 0)
		return -1;
	

	// Request dynamic allocation of a device major number
        if (alloc_chrdev_region(&rb_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "RB :Can't register device\n"); return -1;
	}


	//Create class. If class creation fails, unregister the led_spi char device.
	led_spi_class = class_create(THIS_MODULE, DEVICE_CLASS);
	if (IS_ERR(led_spi_class)) {
		unregister_chrdev_region(led_spi_device_number, 1);
		return PTR_ERR(led_spi_class);
	}


        // Connect the file operations with the cdev 
        cdev_init(&led_cdev, &led_spi_fops);
        led_cdev.owner = THIS_MODULE;

        // Connect the major/minor number to the cdev 
        status = cdev_add(&led_cdev, led_spi_device_number, 1);

        if (status < 0) {
                printk(KERN_INFO "LED_SPI : CDEV ADD FAILED\n");
		class_destroy(led_spi_class);
		unregister_chrdev_region(led_spi_device_number, 1);
                return status;
        }
        
       	led_spi_device = device_create(led_spi_class, NULL, MKDEV(MAJOR(led_spi_device_number), 0), NULL, DEVICE_NAME);    

	
	//Register the spi driver.If driver creation fails, destroy the class, device and the char_dev region
	status = spi_register_driver(&led_spi_driver);
	if (status < 0) {
		device_destroy (led_spi_class, MKDEV(MAJOR(led_spi_device_number), 0));
		class_destroy(led_spi_class);
		unregister_chrdev_region(led_spi_device_number,1);
	}
	
	return status;
	*/
	
	status = register_chrdev(LED_SPIDEV_MAJOR, DEVICE_NAME, &led_spi_fops);
	if (status < 0)
		return status;

	led_spi_class = class_create(THIS_MODULE, DEVICE_CLASS);
	if (IS_ERR(led_spi_class)) {
		unregister_chrdev(LED_SPIDEV_MAJOR, DEVICE_CLASS);
		return PTR_ERR(led_spi_class);
	}
	
	status = spi_register_driver(&led_spi_driver);
	if (status < 0) {
		class_destroy(led_spi_class);
		unregister_chrdev(LED_SPIDEV_MAJOR, DEVICE_CLASS);
	}

	return status;

}
module_init(led_spi_init);


//Unregister the spi driver, destroy the spi class, unregister the char device
static void __exit led_spi_exit(void)
{
	spi_unregister_driver(&led_spi_driver);
	//device_destroy (led_spi_class, MKDEV(MAJOR(led_spi_device_number), 0));
	class_destroy(led_spi_class);
	//unregister_chrdev_region(led_spi_device_number,1);


	unregister_chrdev(LED_SPIDEV_MAJOR, DEVICE_CLASS);



}
module_exit(led_spi_exit);
MODULE_LICENSE("GPL");
