// DHT11 sensörü için yazılan Character Device Driver
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>

// GPIO pinlerinin değerleri. Bu değerler pinlerin fiziksel yerlerini göstermiyor.
// OrangePi PC için bu değerler özel bir formül ile hesaplanıyor.
// Detaylı açıklama: https://linux-sunxi.org/GPIO
#define DHTPIN 6

#define MAXTIMINGS 85
#define BUFLEN 256
#define DEV_COUNT 1

// Device için gereken değişkenlerin tanımlanması
dev_t dev = 0;
static struct class *chardev_class;
static struct cdev chardev_cdev;

static char kernel_buffer[BUFLEN];
static int dht11_data[5] = {0, 0, 0, 0, 0};

// Device için kullanılacak fonksiyonların tanımlanması.
static int __init device_init_module(void);
static void __exit device_exit_module(void);
static int chardev_open(struct inode *inode, struct file *file);
static int chardev_release(struct inode *inode, struct file *file);
static ssize_t chardev_read(struct file *filep, char __user *buf, size_t len, loff_t *off);

// Dosya işlemlerinin tanımlanması.
// Bu device sadece yazma için kullanılacağından dolayı okuma fonksiyonu yok.
static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = chardev_open,
    .release = chardev_release,
    .read = chardev_read
};

// open fonksiyonu
static int chardev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device File açıldı.\n");
    return 0;
}

// release fonksyionu
static int chardev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device File kapatıldı.\n");
    return 0;
}

// read fonksiyonu
static ssize_t chardev_read(struct file *filep, char __user *buf, size_t len, loff_t *off)
{
    uint8_t laststate = 1;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht11_data[0] = dht11_data[1] = dht11_data[2] = dht11_data[3] = dht11_data[4] = 0;

    // 18 milisaniye pini kapalı tutulması.
    gpio_direction_output(DHTPIN, 0);
    gpio_set_value(DHTPIN, 0);
    mdelay(18);

    // 40 milisaniye de pinin açılması.
    gpio_set_value(DHTPIN, 1);
    udelay(40);

    // Pinden değerlerin okunmaya başlaması.
    gpio_direction_input(DHTPIN);

    // Değişikliğin fark edilmesi ve verinin okunması.
    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (gpio_get_value(DHTPIN) == laststate)
        {
            counter++;
            udelay(1);
            if (counter == 255)
                break;
        }

        laststate = gpio_get_value(DHTPIN);
        if (counter == 255)
            break;

        // Gelen ilk 3 verinin önemsenmemesi.
        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_data[j / 8] <<= 1;
            if (counter > 16)
                dht11_data[j / 8] |= 1;
            j++;
        }
    }

    // 40 bitin (8bit x 5) kontrol edilmesi.
    // Veri iyi durumdaysa kernel_buffer'a atanması.
    if ((j >= 40) && (dht11_data[4] == ((dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3]) & 0xFF)))
    {
        memset(kernel_buffer, '\0', BUFLEN);
        printk(KERN_INFO "Humidity = %d.%d %% Temperature = %d.%d *C \n", dht11_data[0], dht11_data[1], dht11_data[2], dht11_data[3]);
        sprintf(kernel_buffer, "%d", dht11_data[2]);
    }
    else
    {
        printk(KERN_INFO "Data not good, skip\n");
    }

    if (copy_to_user(buf, kernel_buffer, BUFLEN))
        return -EFAULT;

    return BUFLEN;
}

// init fonksiyonu
static int __init device_init_module(void)
{
    // kernel_buffer'ın içeriğinin sıfırlanması.
    memset(kernel_buffer, '\0', BUFLEN);

    // Major Numaranın oluşturulması
    if ((alloc_chrdev_region(&dev, 0, DEV_COUNT, "DHT11")) < 0)
    {
        printk(KERN_ALERT "Major numara oluşturulamadı. \n");
        return -1;
    }
    printk(KERN_INFO "Major : %d Minor : %d\n", MAJOR(dev), MINOR(dev));

    // cdev yapısının oluşturulması
    cdev_init(&chardev_cdev, &fops);

    // Character Device'ın sisteme eklenmesi
    if ((cdev_add(&chardev_cdev, dev, DEV_COUNT)) < 0)
    {
        printk(KERN_ALERT "Device sisteme eklenemedi.\n");
        unregister_chrdev_region(dev, DEV_COUNT);
        return -1;
    }

    //Class yapısının oluşturulması
    if ((chardev_class = class_create(THIS_MODULE, "DHT11_Class")) == NULL)
    {
        printk(KERN_ALERT "Class yapısı oluşturulamadı.\n");
        unregister_chrdev_region(dev, DEV_COUNT);
        return -1;
    }

    // Device'ın oluşturulması
    if ((device_create(chardev_class, NULL, dev, NULL, "DHT11_Device")) == NULL)
    {
        printk(KERN_ALERT "Device oluşturulamadı.\n");
        class_destroy(chardev_class);
    }
    printk(KERN_INFO "Device Driver oluşturuldu.\n");

    return 0;
}

// exit fonksiyonu
static void __exit device_exit_module(void)
{
    gpio_free(DHTPIN);
    device_destroy(chardev_class, dev);
    class_destroy(chardev_class);
    cdev_del(&chardev_cdev);
    unregister_chrdev_region(dev, DEV_COUNT);
    printk(KERN_INFO "Device Driver kaldırıldı.\n");
}

module_init(device_init_module);
module_exit(device_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NativeCore");
