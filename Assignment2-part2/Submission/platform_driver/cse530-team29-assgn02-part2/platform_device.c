/*
 * A sample program to show the binding of platform driver and device.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "Sample_platform_device.h"


static struct P_chip P1_chip = {
		.name	= "xyz01",
		.dev_no 	= 20,
		.plf_dev = {
			.name	= "abcd",
			.id	= -1,
		}
};

static struct P_chip P2_chip = {
		.name	= "xyz02",
		.dev_no 	= 55,
		.plf_dev = {
			.name	= "defg",
			.id	= -1,
		}
};


static struct P_chip P3_chip = {
		.name   = "test007",
                .dev_no         = 55,
                .plf_dev = {
                        .name   = "platform_driver_0",
                        .id     = -1,
		}
};


/**
 * register the device when module is initiated
 */

static int p_device_init(void)
{
	int ret = 0;
	
	struct platform_device *plt_1, *plt_2, *plt_3;	

	/* Register the device */
/*	platform_device_register(&P1_chip.plf_dev);
	
	printk(KERN_ALERT "Platform device 1 is registered in init \n");

	platform_device_register(&P2_chip.plf_dev);

	printk(KERN_ALERT "Platform device 2 is registered in init \n");
	

	printk(KERN_ALERT "Platform device 3 is registered in init \n");
	platform_device_register(&P3_chip.plf_dev); */

	plt_1 = platform_device_alloc(P1_chip.plf_dev.name,P1_chip.plf_dev.id);
	if (!plt_1) {
		printk(KERN_ALERT "%s(#%d): Platform device 1 alloc failed\n",__func__,__LINE__);
		return -ENOMEM;
	}
	plt_2 = platform_device_alloc(P2_chip.plf_dev.name,P2_chip.plf_dev.id);
	if (!plt_1) {
                printk(KERN_ALERT "%s(#%d): Platform device 2 alloc failed\n",__func__,__LINE__);
                return -ENOMEM;
        }
	//plt_3 = platform_device_alloc(&P3_chip.pl_dev.name,&P3_chip.pl_dev.id);

	ret = platform_device_add(plt_1);
	if (ret) {
		printk(KERN_ALERT "%s(#%d): Platform device 1 add failed\n",__func__,__LINE__);
                return -ENOMEM;
	}
	ret = platform_device_add(plt_2);
	if (ret) {
                printk(KERN_ALERT "%s(#%d): Platform device 2 add failed\n",__func__,__LINE__);
                return -ENOMEM;
        }

	return ret;
}

static void p_device_exit(void)
{
    	platform_device_unregister(&P1_chip.plf_dev);

	platform_device_unregister(&P2_chip.plf_dev);

	//platform_device_unregister(&P3_chip.plf_dev);

	printk(KERN_ALERT "Goodbye, unregister the device\n");
}

module_init(p_device_init);
module_exit(p_device_exit);
MODULE_LICENSE("GPL");
