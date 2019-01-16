// Sıcaklık kontrolü için yazılan Character Device Driver
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

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
static ssize_t chardev_read(struct file *filep, char __user *buf, size_t len, loff_t *off);
static ssize_t chardev_write(struct file *filep, const char *buf, size_t len, loff_t *off);

// Dosya işlemlerinin tanımlanması.
static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = chardev_open,
    .release = chardev_release,
    .read = chardev_read,
    .write = chardev_write
};

// open fonksiyonu
static int chardev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device File açıldı.\n");
    return 0;
}

// release fonksiyonu
static int chardev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device File kapatıldı.\n");
    return 0;
}

// read fonksiyonu
static ssize_t chardev_read(struct file *filep, char __user *buf, size_t len, loff_t *off)
{
    if(copy_to_user(buf, kernel_buffer, BUFLEN))
        return -EFAULT;
    return BUFLEN;
}

// write fonksiyonu
static ssize_t chardev_write(struct file *filep, const char *buf, size_t len, loff_t *off)
{
    int temperature;

    // kernel_buffer'ın içinin temizlenmesi
    memset(kernel_buffer, '\0', BUFLEN);
    // user space'den verilerin alınması
    if(copy_from_user(kernel_buffer, buf, len))
        return -EFAULT;
    // kernel_buffer'daki değerin temperature değişkenine atanması
    sscanf(kernel_buffer, "%d", &temperature);

    printk(KERN_INFO "Sıcaklık : %d", temperature);

    // temperature değişkenin içindeki değere hangi aralığa denk geliyorsa 
    // ona uygun renk kodu kernel_bufer'a atanıyor.
    if (temperature < 10)
        strcpy(kernel_buffer, "001");
    else if (temperature >= 10 && temperature < 15)
        strcpy(kernel_buffer, "011");
    else if (temperature >= 15 && temperature < 25)
        strcpy(kernel_buffer, "010");
    else if (temperature >= 25 && temperature < 30)
        strcpy(kernel_buffer, "110");    
    else if (temperature >= 30)
        strcpy(kernel_buffer, "100");

    printk(KERN_INFO "Renk : %s\n", kernel_buffer);

    return len;
}

// init fonksiyonu
static int __init device_init_module(void)
{
    memset(kernel_buffer, '\0', BUFLEN);

    // Major Numaranın oluşturulması
    if ((alloc_chrdev_region(&dev, 0, 1, "Temp_Control")) < 0)
    {
        printk(KERN_ALERT "Major numara oluşturulamadı. \n");
        return -1;
    }
    printk(KERN_INFO "Major : %d Minor : %d\n", MAJOR(dev), MINOR(dev));

    // cdev yapısının oluşturulması
    cdev_init(&chardev_cdev, &fops);

    // Character Device'ın sisteme eklenmesi
    if ((cdev_add(&chardev_cdev, dev, 1)) < 0)
    {
        printk(KERN_ALERT "Device sisteme eklenemedi.\n");
        unregister_chrdev_region(dev, 1);
        return -1;
    }

    //Class yapısının oluşturulması
    if ((chardev_class = class_create(THIS_MODULE, "Temp_Control_Class")) == NULL)
    {
        printk(KERN_ALERT "Class yapısı oluşturulamadı.\n");
        unregister_chrdev_region(dev, 1);
        return -1;
    }

    // Device'ın oluşturulması
    if ((device_create(chardev_class, NULL, dev, NULL, "Temp_Control_Device")) == NULL)
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
    device_destroy(chardev_class, dev);
    class_destroy(chardev_class);
    cdev_del(&chardev_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "Device Driver kaldırıldı.\n");
}

module_init(device_init_module);
module_exit(device_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NativeCore");