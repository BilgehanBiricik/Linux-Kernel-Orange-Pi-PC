// RGB led için yazılan Character Device Driver
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/time.h>

// GPIO pinlerinin değerleri. Bu değerler pinlerin fiziksel yerlerini göstermiyor.
// OrangePi PC için bu değerler özel bir formül ile hesaplanıyor.
// Detaylı açıklama: https://linux-sunxi.org/GPIO
#define LED_R 20
#define LED_G 10
#define LED_B 9

#define DEV_COUNT 3
#define BUFLEN 256

// Device için gereken değişkenlerin tanımlanması
dev_t dev = 0;
static struct class *chardev_class;
static struct cdev chardev_cdev;

static char kernel_buffer[BUFLEN];

// Device için kullanılacak fonksiyonların tanımlanması.
static int __init device_init_module(void);
static void __exit device_exit_module(void);
static int chardev_open(struct inode *inode, struct file *file);
static int chardev_release(struct inode *inode, struct file *file);
static ssize_t chardev_write(struct file *filep, const char *buf, size_t len, loff_t *off);

// Dosya işlemlerinin tanımlanması.
// Bu device sadece yazma için kullanılacağından dolayı okuma fonksiyonu yok.
static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = chardev_open,
    .release = chardev_release,
    .write = chardev_write
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

// write fonksiyonu
static ssize_t chardev_write(struct file *filep, const char *buf, size_t len, loff_t *off)
{
    int r = 0, g = 0, b = 0;

    // user space'den kernell bufferr'a dizisine atama.
    // user space'den 3 elemanlı bir string dönüyor.
    if(copy_from_user(kernel_buffer, buf, len))
        return -EFAULT;

    // Dizinin elemanlarının basit bir şekilde int'e çevrilmesi.
    r = kernel_buffer[0] - '0';
    g = kernel_buffer[1] - '0';
    b = kernel_buffer[2] - '0';

    // RGB ledin yanması.
    gpio_set_value(LED_R, r);
    gpio_set_value(LED_G, g);
    gpio_set_value(LED_B, b);

    return len;
}

// init fonksiyonu
static int __init device_init_module(void)
{
    // kernel_buffer'ın içeriğinin sıfırlanması.
    memset(kernel_buffer, '\0', BUFLEN);

    // Major Numaranın oluşturulması
    if ((alloc_chrdev_region(&dev, 0, DEV_COUNT, "RGB_LED")) < 0)
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
    if ((chardev_class = class_create(THIS_MODULE, "RGB_LED_Class")) == NULL)
    {
        printk(KERN_ALERT "Class yapısı oluşturulamadı.\n");
        unregister_chrdev_region(dev, DEV_COUNT);
        return -1;
    }

    // Device'ın oluşturulması
    if ((device_create(chardev_class, NULL, dev, NULL, "RGB_LED_Device")) == NULL)
    {
        printk(KERN_ALERT "Device oluşturulamadı.\n");
        class_destroy(chardev_class);
    }
    printk(KERN_INFO "Device Driver oluşturuldu.\n");

    printk(KERN_INFO "GPIO pinleri tanımlarnıyor..\n");

    // GPIO pinlerinin geçerli olup olmadığı kontrol edilmesi.
    if (gpio_is_valid(LED_R) == 0)
    {
        printk(KERN_ALERT "LED_R geçerli değil.\n");
        return -1;
    }
    if (gpio_is_valid(LED_G) == 0)
    {
        printk(KERN_ALERT "LED_G geçerli değil.\n");
        return -1;
    }
    if (gpio_is_valid(LED_B) == 0)
    {
        printk(KERN_ALERT "LED_B geçerli değil.\n");
        return -1;
    }
    printk(KERN_INFO "Tüm pinler geçerli.\n");

    // GPIO pinlerinin hazırlanması.
    if (gpio_request(LED_R, "LED_R GPIO") < 0)
    {
        printk(KERN_ALERT "LED_R %d request başarısız.\n", LED_R);
        return -1;
    }
    if (gpio_request(LED_G, "LED_G GPIO") < 0)
    {
        printk(KERN_ALERT "LED_G %d request başarısız.\n", LED_G);
        return -1;
    }
    if (gpio_request(LED_B, "LED_B GPIO") < 0)
    {
        printk(KERN_ALERT "LED_B %d request başarısız.\n", LED_B);
        return -1;
    }
    printk(KERN_INFO "Tüm requestler başarılı.\n");

    // Pinleri output modda çalışması ve initial değerlerinin 0 olarak atanması.
    gpio_direction_output(LED_R, 0);
    gpio_direction_output(LED_G, 0);
    gpio_direction_output(LED_B, 0);

    return 0;
}

// exit fonksiyonu
static void __exit device_exit_module(void)
{
    gpio_set_value(LED_R, 0);
    gpio_set_value(LED_G, 0);
    gpio_set_value(LED_B, 0);
    gpio_free(LED_R);
    gpio_free(LED_G);
    gpio_free(LED_B);
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