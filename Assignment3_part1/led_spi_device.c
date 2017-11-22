#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/spi/spi.h>

/* MODULE PARAMETERS */
static uint led_spi_bus = 1;
static uint speed_hz = 1.5*1000*1000;
static struct spi_device *led_spi_device;
#define DEVICE_NAME "WS2812"

static int __init led_spidev_init(void) {
	struct spi_board_info led_spi_device_info = {
		.modalias = DEVICE_NAME,
		.max_speed_hz = speed_hz,
		.bus_num = led_spi_bus,
		.chip_select = 0,
		.mode = 0,
	};

	struct spi_master *master;

	int ret;

	master = spi_busnum_to_master(led_spi_device_info.bus_num);
	if( !master )
		return -ENODEV;
	
	led_spi_device = spi_new_device( master, &led_spi_device_info );
	if( !led_spi_device )
		return -ENODEV;

	//led_spi_device->bits_per_word = 16;
	ret = spi_setup( led_spi_device );
	
	if( ret )
		spi_unregister_device(led_spi_device);
	else
		printk( KERN_INFO "LED_DEVICE : Device setup ready." );

	return ret;
}

static void __exit led_spidev_exit(void) {
	spi_unregister_device(led_spi_device);
}


module_init(led_spidev_init);
module_exit(led_spidev_exit);
MODULE_LICENSE("GPL");

