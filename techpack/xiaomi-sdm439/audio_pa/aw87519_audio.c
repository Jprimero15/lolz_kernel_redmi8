/*
 * aw87519_audio.c   aw87519 pa module
 *
 * Version: v1.0.3
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 * Copyright (C) 2021 XiaoMi, Inc.
 *
 *  Author: Joseph <zhangzetao@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <aw87519_audio.h>

/*******************************************************************************
 * aw87519 marco
 ******************************************************************************/
#define AW87519_I2C_NAME    "AW87519_PA"
#define AW87519_DRIVER_VERSION  "v1.0.3"

/*******************************************************************************
 * aw87519 functions
 ******************************************************************************/
unsigned char xiaomi_sdm439_aw87519_audio_receiver(void);
unsigned char xiaomi_sdm439_aw87519_audio_speaker(void);
unsigned char xiaomi_sdm439_aw87519_audio_off(void);

/*******************************************************************************
 * aw87519 variable
 ******************************************************************************/
static struct aw87519 *aw87519;
static struct aw87519_container *aw87519_spk_cnt;
static struct aw87519_container *aw87519_rcv_cnt;

static char *aw87519_spk_name = "aw87519_spk.bin";
static char *aw87519_rcv_name = "aw87519_rcv.bin";


/*******************************************************************************
 * i2c write and read
 ******************************************************************************/
 static int aw87519_i2c_write(struct aw87519 *aw87519,
          unsigned char reg_addr, unsigned char reg_data)
 {
     int ret = -1;
     unsigned char cnt = 0;

     while(cnt < AW_I2C_RETRIES) {
         ret = i2c_smbus_write_byte_data(aw87519->i2c_client, reg_addr, reg_data);
         if(ret < 0) {
             pr_err("%s: i2c_write cnt=%d error=%d\n", __func__, cnt, ret);
         } else {
             break;
         }
         cnt ++;
         msleep(AW_I2C_RETRY_DELAY);
     }

     return ret;
 }

 static int aw87519_i2c_read(struct aw87519 *aw87519,
         unsigned char reg_addr, unsigned char *reg_data)
 {
     int ret = -1;
     unsigned char cnt = 0;

     while(cnt < AW_I2C_RETRIES) {
         ret = i2c_smbus_read_byte_data(aw87519->i2c_client, reg_addr);
         if(ret < 0) {
             pr_err("%s: i2c_read cnt=%d error=%d\n", __func__, cnt, ret);
         } else {
             *reg_data = ret;
             break;
         }
         cnt ++;
         msleep(AW_I2C_RETRY_DELAY);
     }

     return ret;
 }

/*******************************************************************************
 * aw87519 hardware control
 ******************************************************************************/
static unsigned int aw87519_hw_on(struct aw87519 *aw87519)
{
    pr_info("%s enter\n", __func__);

    if (aw87519 && gpio_is_valid(aw87519->reset_gpio)) {
        gpio_set_value_cansleep(aw87519->reset_gpio, 0);
        msleep(2);
        gpio_set_value_cansleep(aw87519->reset_gpio, 1);
        msleep(2);
        aw87519->hwen_flag = 1;
        aw87519_i2c_write(aw87519,0x64,0x2C);
    } else {
        dev_err(&aw87519->i2c_client->dev, "%s:  failed\n", __func__);
    }

    return 0;
}

static unsigned int aw87519_hw_off(struct aw87519 *aw87519)
{
    pr_info("%s enter\n", __func__);

    if (aw87519 && gpio_is_valid(aw87519->reset_gpio)) {
        gpio_set_value_cansleep(aw87519->reset_gpio, 0);
        msleep(2);
        aw87519->hwen_flag = 0;
    } else {
        dev_err(&aw87519->i2c_client->dev, "%s:  failed\n", __func__);
    }
    return 0;
}


/*******************************************************************************
 * aw87519 control interface
 ******************************************************************************/
unsigned char xiaomi_sdm439_aw87519_audio_receiver(void)
{
    unsigned int i;
    unsigned int length;

    pr_info("%s enter\n", __func__);
    if(NULL == aw87519) {
        return 2;
    }

    if(!aw87519->hwen_flag) {
        aw87519_hw_on(aw87519);
    }

    length = sizeof(aw87519_spk_cfg_default)/sizeof(char);
    if(aw87519->rcv_cfg_update_flag==0){ /*update array data*/
        for(i=0;i<length;i=i+2){
            aw87519_i2c_write(aw87519, aw87519_rcv_cfg_default[i], aw87519_rcv_cfg_default[i+1]);
        }
    }
    if(aw87519->rcv_cfg_update_flag==1){  /*update bin data*/
        for(i=0;i<aw87519_rcv_cnt->len;i=i+2){
            aw87519_i2c_write(aw87519, aw87519_rcv_cnt->data[i], aw87519_rcv_cnt->data[i+1]);
        }
    }

    return 0;
}

unsigned char xiaomi_sdm439_aw87519_audio_speaker(void)
{
    unsigned int i;
    unsigned int length;

    pr_info("%s enter\n", __func__);

    if(NULL == aw87519) {
        return 2;
    }

    if(!aw87519->hwen_flag) {
        aw87519_hw_on(aw87519);
    }

    length = sizeof(aw87519_spk_cfg_default)/sizeof(char);
    if(aw87519->spk_cfg_update_flag==0){ /*send array data*/
        for(i=0;i<length;i=i+2){
            aw87519_i2c_write(aw87519, aw87519_spk_cfg_default[i], aw87519_spk_cfg_default[i+1]);
        }
    }
    if(aw87519->spk_cfg_update_flag==1){  /*send bin data*/
        for(i=0;i<aw87519_spk_cnt->len;i=i+2){
            aw87519_i2c_write(aw87519, aw87519_spk_cnt->data[i], aw87519_spk_cnt->data[i+1]);
        }
    }

    return 0;
}

unsigned char xiaomi_sdm439_aw87519_audio_off(void)
{
    if(NULL == aw87519) {
        return 2;
    }

    if(aw87519->hwen_flag) {
        aw87519_i2c_write(aw87519, 0x01, 0x00);        // CHIP Disable
    }
    aw87519_hw_off(aw87519);

    return 0;
}

/*******************************************************************************
 * aw87519 firmware cfg update
 ******************************************************************************/
 static void aw87519_receiver_cfg_loaded(const struct firmware *cont, void *context)
 {
      int i = 0;

      pr_info("%s enter\n", __func__);

      if (!cont) {
          pr_err("%s: failed to read %s\n", __func__, aw87519_rcv_name);
          release_firmware(cont);
          return;
      }

      pr_info("%s: loaded %s - size: %zu\n", __func__, aw87519_rcv_name,
                      cont ? cont->size : 0);

      for(i=0; i<cont->size; i=i+2) {
          pr_info("%s: addr:0x%04x, data:0x%02x\n", __func__, *(cont->data+i), *(cont->data+i+1));
      }

      /* aw87519 ram update */
      aw87519_rcv_cnt = kzalloc(cont->size+sizeof(int), GFP_KERNEL);
      if (!aw87519_rcv_cnt) {
          release_firmware(cont);
          pr_err("%s: Error allocating memory\n", __func__);
          return;
      }
      aw87519_rcv_cnt->len = cont->size;
      memcpy(aw87519_rcv_cnt->data, cont->data, cont->size);
      release_firmware(cont);

      printk("%s: fw update complete\n", __func__);
 }

 static int aw87519_rcv_update(struct aw87519 *aw87519)
 {
     pr_info("%s enter\n", __func__);
     return request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
                 aw87519_rcv_name, &aw87519->i2c_client->dev,GFP_KERNEL,
                 aw87519, aw87519_receiver_cfg_loaded);
 }

 static void aw87519_speaker_cfg_loaded(const struct firmware *cont, void *context)
 {
   int i = 0;

   pr_info("%s enter\n", __func__);

   if (!cont) {
       pr_err("%s: failed to read %s\n", __func__, aw87519_spk_name);
       release_firmware(cont);
       return;
   }

   pr_info("%s: loaded %s - size: %zu\n", __func__, aw87519_spk_name,
                   cont ? cont->size : 0);

   for(i=0; i<cont->size; i=i+2) {
       pr_info("%s: addr:0x%04x, data:0x%02x\n", __func__, *(cont->data+i), *(cont->data+i+1));
   }

   /* aw87519 ram update */
   aw87519_spk_cnt = kzalloc(cont->size+sizeof(int), GFP_KERNEL);
   if (!aw87519_spk_cnt) {
       release_firmware(cont);
       pr_err("%s: Error allocating memory\n", __func__);
       return;
   }
   aw87519_spk_cnt->len = cont->size;
   memcpy(aw87519_spk_cnt->data, cont->data, cont->size);
   release_firmware(cont);

   printk("%s: fw update complete\n", __func__);

   aw87519_rcv_update(aw87519);
 }


#ifdef AWINIC_CFG_UPDATE_DELAY
static int aw87519_cfg_update(struct aw87519 *aw87519)
{
    pr_info("%s enter\n", __func__);

    return request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
                aw87519_spk_name, &aw87519->i2c_client->dev,GFP_KERNEL,
                aw87519, aw87519_speaker_cfg_loaded);
}

static void aw87519_cfg_work_routine(struct work_struct *work)
{
    pr_info("%s enter\n", __func__);
    aw87519_cfg_update(aw87519);
}
#endif


static int aw87519_cfg_init(struct aw87519 *aw87519)
{
    int ret = -1;
#ifdef AWINIC_CFG_UPDATE_DELAY
    int cfg_timer_val = 5000;
    INIT_DELAYED_WORK(&aw87519->ram_work, aw87519_cfg_work_routine);
    schedule_delayed_work(&aw87519->ram_work, msecs_to_jiffies(cfg_timer_val));
    ret = 0;
#endif
    return ret;
}

/*******************************************************************************
 * aw87519 attribute
 ******************************************************************************/
static ssize_t aw87519_get_reg(struct device* dev,struct device_attribute *attr, char* buf)
{
    ssize_t len = 0;
    unsigned int i = 0;
    unsigned char reg_val = 0;
    for(i = 0; i < AW87519_REG_MAX; i++) {
        aw87519_i2c_read(aw87519, i, &reg_val);
        len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%02x=0x%02x\n", i, reg_val);
    }
     for(i = 0x60; i <= 0x69; i++) {
        aw87519_i2c_read(aw87519, i, &reg_val);
        len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%02x=0x%02x\n", i, reg_val);
    }
    return len;
}

static ssize_t aw87519_set_reg(struct device* dev, struct device_attribute *attr, const char* buf, size_t len)
{
    unsigned int databuf[2] = {0, 0};
    if(2 == sscanf(buf,"%x %x",&databuf[0], &databuf[1]))
    {
        aw87519_i2c_write(aw87519, databuf[0], databuf[1]);
    }
    return len;
}


static ssize_t aw87519_get_hwen(struct device* dev,struct device_attribute *attr, char* buf)
{
    ssize_t len = 0;
    len += snprintf(buf+len, PAGE_SIZE-len, "hwen: %d\n", aw87519->hwen_flag);

    return len;
}

static ssize_t aw87519_set_hwen(struct device* dev, struct device_attribute *attr, const char* buf, size_t len)
{
    unsigned int databuf[16];
    sscanf(buf,"%d",&databuf[0]);
    if(databuf[0] == 0) {        // OFF
        aw87519_hw_off(aw87519);
    } else {                    // ON
        aw87519_hw_on(aw87519);
    }

    return len;
}

static ssize_t aw87519_get_update(struct device* dev,struct device_attribute *attr, char* buf)
{
    ssize_t len = 0;

    return len;
}

static ssize_t aw87519_set_update(struct device* dev, struct device_attribute *attr, const char* buf, size_t len)
{
    unsigned int databuf[16];
    int ret;

    sscanf(buf,"%d",&databuf[0]);
    if(databuf[0] == 0) {
    } else {
        aw87519->spk_cfg_update_flag = 1;
        aw87519->rcv_cfg_update_flag = 1;
        ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG, aw87519_spk_name,
                &aw87519->i2c_client->dev, GFP_KERNEL, aw87519, aw87519_speaker_cfg_loaded);
        if(ret) {
            pr_err("%s: request_firmware_nowait failed with read %s", __func__, aw87519_spk_name);
        }
    }

    return len;
}

static ssize_t aw87519_get_mode(struct device* cd,struct device_attribute *attr, char* buf)
{
    ssize_t len = 0;

    len += snprintf(buf+len, PAGE_SIZE-len, "0: off mode\n");
    len += snprintf(buf+len, PAGE_SIZE-len, "1: spk mode\n");
    len += snprintf(buf+len, PAGE_SIZE-len, "2: rcv mode\n");


    return len;
}

static ssize_t aw87519_set_mode(struct device* dev, struct device_attribute *attr, const char* buf, size_t len)
{
    unsigned int databuf[16];

    sscanf(buf,"%d",&databuf[0]);
    if(databuf[0] == 0) {
        xiaomi_sdm439_aw87519_audio_off();
    } else if(databuf[0] == 1) {
        xiaomi_sdm439_aw87519_audio_speaker();
    } else if(databuf[0] == 2) {
        xiaomi_sdm439_aw87519_audio_receiver();

    } else {
        xiaomi_sdm439_aw87519_audio_off();
    }

    return len;
}

static DEVICE_ATTR(reg, 0660, aw87519_get_reg,  aw87519_set_reg);
static DEVICE_ATTR(hwen, 0660, aw87519_get_hwen,  aw87519_set_hwen);
static DEVICE_ATTR(update, 0660, aw87519_get_update,  aw87519_set_update);
static DEVICE_ATTR(mode, 0660, aw87519_get_mode,  aw87519_set_mode);

static struct attribute *aw87519_attributes[] = {
    &dev_attr_reg.attr,
    &dev_attr_hwen.attr,
    &dev_attr_update.attr,
    &dev_attr_mode.attr,
    NULL
};

static struct attribute_group aw87519_attribute_group = {
    .attrs = aw87519_attributes
};

/*****************************************************
 * device tree
 *****************************************************/
static int aw87519_parse_dt(struct device *dev, struct device_node *np) {

    aw87519->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
    if (aw87519->reset_gpio < 0) {
        dev_err(dev, "%s: no reset gpio provided, will not HW reset device\n", __func__);
        return -1;
    } else {
        dev_info(dev, "%s: reset gpio provided ok\n", __func__);
    }
    return 0;
}

static int aw87519_hw_reset(struct aw87519 *aw87519)
{
    pr_info("%s enter\n", __func__);

    if (aw87519 && gpio_is_valid(aw87519->reset_gpio)) {
        gpio_set_value_cansleep(aw87519->reset_gpio, 0);
        msleep(2);
        gpio_set_value_cansleep(aw87519->reset_gpio, 1);
        msleep(2);
        aw87519->hwen_flag = 1;
    } else {
        aw87519->hwen_flag = 0;
        dev_err(&aw87519->i2c_client->dev, "%s:  failed\n", __func__);
    }
    return 0;
}

/*****************************************************
 * check chip id
 *****************************************************/
static int aw87519_read_chipid(struct aw87519 *aw87519)
{
    unsigned int cnt = 0;
    int ret = -1;
    unsigned char reg_val = 0;

    while(cnt < AW_READ_CHIPID_RETRIES) {
        aw87519_i2c_write(aw87519,0x64,0x2C);
        ret = aw87519_i2c_read(aw87519, REG_CHIPID, &reg_val);
        if(reg_val== AW87519_CHIPID){
            pr_info("%s This Chip is  AW87519 chipid=0x%x\n", __func__, reg_val);
            return 0;
        } else {
            pr_info("%s: aw87519 chipid=0x%x error\n", __func__, reg_val);
        }
        cnt ++;
        msleep(AW_READ_CHIPID_RETRY_DELAY);
    }

    return -EINVAL;
}

/*******************************************************************************
 * aw87519 i2c driver
 ******************************************************************************/
static int aw87519_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct device_node *np = client->dev.of_node;
    int ret = -1;
    pr_info("%s Enter\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "%s: check_functionality failed\n", __func__);
        ret = -ENODEV;
        goto exit_check_functionality_failed;
    }

    aw87519 = devm_kzalloc(&client->dev, sizeof(struct aw87519), GFP_KERNEL);
    if (aw87519 == NULL) {
        ret = -ENOMEM;
        goto exit_devm_kzalloc_failed;
    }

    aw87519->i2c_client = client;
    i2c_set_clientdata(client, aw87519);

    /* aw87519 rst */
    if (np) {
        ret = aw87519_parse_dt(&client->dev, np);
        if (ret) {
            dev_err(&client->dev, "%s: failed to parse device tree node\n", __func__);
            goto exit_gpio_get_failed;
        }
    } else {
        aw87519->reset_gpio = -1;
    }

    if (gpio_is_valid(aw87519->reset_gpio)) {
        ret = devm_gpio_request_one(&client->dev, aw87519->reset_gpio,
            GPIOF_OUT_INIT_LOW, "aw87519_rst");
        if (ret){
            dev_err(&client->dev, "%s: rst request failed\n", __func__);
            goto exit_gpio_request_failed;
        }
    }

    /* hardware reset */
    aw87519_hw_reset(aw87519);

    /* aw87519 chip id */
    ret = aw87519_read_chipid(aw87519);
    if (ret < 0) {
        dev_err(&client->dev, "%s: aw87519_read_chipid failed ret=%d\n", __func__, ret);
        goto exit_i2c_check_id_failed;
    }

    ret = sysfs_create_group(&client->dev.kobj, &aw87519_attribute_group);
    if (ret < 0) {
        dev_info(&client->dev, "%s error creating sysfs attr files\n", __func__);
    }

    /* aw87519 cfg update */
    aw87519->spk_cfg_update_flag = 1;
    aw87519->rcv_cfg_update_flag = 1;
    aw87519_cfg_init(aw87519);

    /* aw87519 hardware off */
    aw87519_hw_off(aw87519);

    return 0;

exit_i2c_check_id_failed:
    devm_gpio_free(&client->dev, aw87519->reset_gpio);
exit_gpio_request_failed:
exit_gpio_get_failed:
    devm_kfree(&client->dev, aw87519);
    aw87519 = NULL;
exit_devm_kzalloc_failed:
exit_check_functionality_failed:
    return ret;
}

static int aw87519_i2c_remove(struct i2c_client *client)
{
    struct aw87519 *aw87519 = i2c_get_clientdata(client);
    if(gpio_is_valid(aw87519->reset_gpio)) {
        devm_gpio_free(&client->dev, aw87519->reset_gpio);
    }

    return 0;
}

static const struct i2c_device_id aw87519_i2c_id[] = {
    { AW87519_I2C_NAME, 0 },
    { }
};

static const struct of_device_id extpa_of_match[] = {
    {.compatible = "awinic,aw87519"},
    {},
};

static struct i2c_driver aw87519_i2c_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = AW87519_I2C_NAME,
        .of_match_table = extpa_of_match,
    },
    .probe = aw87519_i2c_probe,
    .remove = aw87519_i2c_remove,
    .id_table    = aw87519_i2c_id,
};

static int __init aw87519_pa_init(void) {
    int ret;

    pr_info("%s enter\n", __func__);
    pr_info("%s: driver version: %s\n", __func__, AW87519_DRIVER_VERSION);

    ret = i2c_add_driver(&aw87519_i2c_driver);
    if (ret) {
        pr_info("****[%s] Unable to register driver (%d)\n", __func__, ret);
        return ret;
    }
    return 0;
}

static void __exit aw87519_pa_exit(void) {
    pr_info("%s enter\n", __func__);
    i2c_del_driver(&aw87519_i2c_driver);
}

EXPORT_SYMBOL(xiaomi_sdm439_aw87519_audio_speaker);
EXPORT_SYMBOL(xiaomi_sdm439_aw87519_audio_off);
EXPORT_SYMBOL(xiaomi_sdm439_aw87519_audio_receiver);

module_init(aw87519_pa_init);
module_exit(aw87519_pa_exit);

MODULE_AUTHOR("<zhangzeta@awinic.com.cn>");
MODULE_DESCRIPTION("AWINIC AW87519 PA driver");
MODULE_LICENSE("GPL v2");
