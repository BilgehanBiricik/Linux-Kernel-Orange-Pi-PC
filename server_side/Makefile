obj-m += temp_control_dev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) udp_server.c -o udp_server
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
