obj-m += rgb_led_dev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) udp_client.c -o udp_client
clean:
	make -C /lib/moudles/$(shell uname -r)/build M=$(PWD) clean
