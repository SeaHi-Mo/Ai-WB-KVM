/**
 * @file device.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "blog.h"
#include "bl_gpio.h"
#include "device.h"
#include "hosal_dac.h"
#include "hosal_gpio.h"
#include "easyflash_common.h"
#include <limits.h>
#define DAC_KEY "HDMIDAC"

typedef enum {
    CHANNEL_BIT_1 = 0X01,
    CHANNEL_BIT_2,
    CHANNEL_BIT_3,
}channel_bit_t;

static TaskHandle_t sub_ctlr_task;

static hosal_dac_dev_t HDMI_dac = {
    .config = {
        .dma_enable = false,
        .freq = 8000,
        .pin = DEVICE_HDMI_ADC_PIN,
    },
    .cb = NULL,
    .port = 0
};

static hosal_gpio_dev_t device_channel1 = {
    .config = INPUT_PULL_DOWN,
    .port = DEVICE_CHANNEL1_IN_PIN,
};
static hosal_gpio_dev_t device_channel2 = {
    .config = INPUT_PULL_DOWN,
    .port = DEVICE_CHANNEL2_IN_PIN,
};
static hosal_gpio_dev_t device_channel3 = {
    .config = INPUT_PULL_DOWN,
    .port = DEVICE_CHANNEL3_IN_PIN,
};
static void device_key_init(void);
static void device_usbHub_pin_init(void);
static void device_adc_init(void);


/**
 * @brief HDMI 控制任务
 *
 * @param arg
 */
void device_HDMI_ctlr(void* arg)
{
    easyflash_init();
    device_usbHub_pin_init();
    device_key_init();
    device_adc_init();
    blog_info("key checking ......");
    while (1) {
        //进行按键检测
        if (!bl_gpio_input_get_value(DEVICE_KEY_CHANNEL1_PIN)) {
            while (!bl_gpio_input_get_value(DEVICE_KEY_CHANNEL1_PIN)) vTaskDelay(50/portTICK_PERIOD_MS);
            hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL1_VALUE);
            ef_set_u32(DAC_KEY, DEVICE_CHANNEL1_VALUE);
            blog_info("HDMI channel is 1, adc:%d", hosal_dac_get_value(&HDMI_dac));
        }

        if (!bl_gpio_input_get_value(DEVICE_KEY_CHANNEL2_PIN)) {
            while (!bl_gpio_input_get_value(DEVICE_KEY_CHANNEL2_PIN)) vTaskDelay(50/portTICK_PERIOD_MS);
            hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL2_VALUE);
            ef_set_u32(DAC_KEY, DEVICE_CHANNEL2_VALUE);
            blog_info("HDMI channel is 2,adc:%d", hosal_dac_get_value(&HDMI_dac));
        }

        if (!bl_gpio_input_get_value(DEVICE_KEY_CHANNEL3_PIN)) {
            while (!bl_gpio_input_get_value(DEVICE_KEY_CHANNEL3_PIN)) vTaskDelay(50/portTICK_PERIOD_MS);
            hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL3_VALUE);
            ef_set_u32(DAC_KEY, DEVICE_CHANNEL3_VALUE);
            blog_info("HDMI channel is 3,adc:%d", hosal_dac_get_value(&HDMI_dac));
        }

        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}
/**
 * @brief HDMI 通道1 识别中断
 *
 * @param arg
 */
static void device_channel1_irq(void* arg)
{
    blog_info("HDMI channel 1 ");
    // bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1);//先关闭USB HUB
    // //切换设备
    // bl_gpio_output_set(DEVICE_USB_IN0_PIN, 0);
    // bl_gpio_output_set(DEVICE_USB_IN1_PIN, 0);

    // bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB
    xTaskNotifyFromISR(sub_ctlr_task, CHANNEL_BIT_1, eSetBits, NULL);
}
/**
 * @brief  HDMI 通道2 识别中断
 *
 * @param arg
 */
static void device_channel2_irq(void* arg)
{
    blog_info("HDMI channel 2 ");
    // bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1); //先关闭USB HUB
    // //切换设备
    // bl_gpio_output_set(DEVICE_USB_IN0_PIN, 0);
    // bl_gpio_output_set(DEVICE_USB_IN1_PIN, 1);

    // bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB

    xTaskNotifyFromISR(sub_ctlr_task, CHANNEL_BIT_2, eSetBits, NULL);
}
/**
 * @brief  HDMI 通道3 识别中断
 *
 * @param arg
 */
static void device_channel3_irq(void* arg)
{
    blog_info("HDMI channel 3 ");

    // bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1);//先关闭USB HUB
    // //切换设备
    // bl_gpio_output_set(DEVICE_USB_IN0_PIN, 1);
    // bl_gpio_output_set(DEVICE_USB_IN1_PIN, 0);

    // bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//先关闭USB HUB

    xTaskNotifyFromISR(sub_ctlr_task, CHANNEL_BIT_3, eSetBits, NULL);
}

static void usb_out_en_task(void* arg)
{
    channel_bit_t channel_bit;
    while (1) {
        xTaskNotifyWait(ULONG_MAX, 0, &channel_bit, portMAX_DELAY);
        blog_info("get notify :%d", channel_bit);
        switch (channel_bit) {
            case CHANNEL_BIT_1:
                blog_info("set USB for channel1 device");
                bl_gpio_output_set(DEVICE_USB_IN_EN, 1); //关闭输入
                bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1);//先关闭USB HUB
                //切换设备
                bl_gpio_output_set(DEVICE_USB_IN0_PIN, 0);
                bl_gpio_output_set(DEVICE_USB_IN1_PIN, 0);
                bl_gpio_output_set(DEVICE_USB_IN_EN, 0); //使能USB 输入
                vTaskDelay(500/portTICK_PERIOD_MS);
                bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB
                break;
            case CHANNEL_BIT_2:
                blog_info("set USB for channel2 device");
                bl_gpio_output_set(DEVICE_USB_IN_EN, 1); //关闭输入
                bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1); //先关闭USB HUB
                //切换设备
                bl_gpio_output_set(DEVICE_USB_IN0_PIN, 1);
                bl_gpio_output_set(DEVICE_USB_IN1_PIN, 0);
                bl_gpio_output_set(DEVICE_USB_IN_EN, 0); //使能USB 输入
                vTaskDelay(500/portTICK_PERIOD_MS);
                bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB

                bl_gpio_output_set(DEVICE_USB_IN_EN, 0); //使能USB 输入
                break;
            case CHANNEL_BIT_3:
                blog_info("set USB for channel3 device");
                bl_gpio_output_set(DEVICE_USB_IN_EN, 1); //关闭输入
                bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1);//先关闭USB HUB
                //切换设备
                bl_gpio_output_set(DEVICE_USB_IN0_PIN, 0);
                bl_gpio_output_set(DEVICE_USB_IN1_PIN, 1);
                bl_gpio_output_set(DEVICE_USB_IN_EN, 0); //使能USB 输入
                vTaskDelay(500/portTICK_PERIOD_MS);
                bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//先关闭USB HUB

                break;
        }
    }
}
/**
 * @brief
 *
 */
static void device_key_init(void)
{
    int ret = 0;
    ret = bl_gpio_enable_input(DEVICE_KEY_CHANNEL1_PIN, true, false);
    if (!ret) {
        blog_info("key pin%d init success", DEVICE_KEY_CHANNEL1_PIN);
    }
    else blog_error("key pin%d init fail", DEVICE_KEY_CHANNEL1_PIN);

    ret = bl_gpio_enable_input(DEVICE_KEY_CHANNEL2_PIN, true, false);
    if (!ret) {
        blog_info("key pin%d init success", DEVICE_KEY_CHANNEL2_PIN);
    }
    else blog_error("key pin%d init fail", DEVICE_KEY_CHANNEL2_PIN);

    ret = bl_gpio_enable_input(DEVICE_KEY_CHANNEL3_PIN, true, false);
    if (!ret) {
        blog_info("key pin%d init success", DEVICE_KEY_CHANNEL3_PIN);
    }
    else blog_error("key pin%d init fail", DEVICE_KEY_CHANNEL3_PIN);

    ret = hosal_gpio_init(&device_channel1);
    if (!ret) {
        blog_info("key pin%d init success", device_channel1.port);
    }
    else blog_error("key pin%d init fail", device_channel1.port);
    ret = hosal_gpio_init(&device_channel2);
    if (!ret) {
        blog_info("key pin%d init success", device_channel2.port);
    }
    else blog_error("key pin%d init fail", device_channel2.port);

    ret = hosal_gpio_init(&device_channel3);
    if (!ret) {
        blog_info("key pin%d init success", device_channel3.port);
    }
    else blog_error("key pin%d init fail", device_channel3.port);

    ret = hosal_gpio_irq_set(&device_channel1, HOSAL_IRQ_TRIG_POS_PULSE, device_channel1_irq, NULL);
    if (!ret) {
        blog_info("key pin%d set irq success", device_channel1.port);
    }
    else blog_error("key pin%d set irq fail", device_channel1.port);

    ret = hosal_gpio_irq_set(&device_channel2, HOSAL_IRQ_TRIG_POS_PULSE, device_channel2_irq, NULL);
    if (!ret) {
        blog_info("key pin%d set irq success", device_channel2.port);
    }
    else blog_error("key pin%d set irq fail", device_channel2.port);
    ret = hosal_gpio_irq_set(&device_channel3, HOSAL_IRQ_TRIG_POS_PULSE, device_channel3_irq, NULL);
    if (!ret) {
        blog_info("key pin%d set irq success", device_channel3.port);
    }
    else blog_error("key pin%d set irq fail", device_channel3.port);

    xTaskCreate(usb_out_en_task, "usb out", 1024, NULL, 3, &sub_ctlr_task);

}
/**
 * @brief
 *
 */
static void device_usbHub_pin_init(void)
{
    bl_gpio_enable_output(DEVICE_USB_SWITCH_PIN, true, false);
    bl_gpio_enable_output(DEVICE_USB_IN0_PIN, true, false);
    bl_gpio_enable_output(DEVICE_USB_IN1_PIN, true, false);
    bl_gpio_enable_output(DEVICE_USB_IN_EN, true, false);
    bl_gpio_output_set(DEVICE_USB_IN_EN, 1);//关闭输入
}
/**
 * @brief
 *
 */
static void device_adc_init(void)
{
    int ret = 0;
    uint32_t dac_value = 0;
    ret = hosal_dac_init(&HDMI_dac);
    if (ret!=0) {
        hosal_dac_finalize(&HDMI_dac);
        blog_info("adc init false");
        return;
    }
    blog_info("adc init success");
    //上电读取上次设置的值，并设置它
    if (ef_get_u32(DAC_KEY, &dac_value)) {
        ret = hosal_dac_set_value(&HDMI_dac, dac_value);
        blog_info("set dac value:%d", dac_value);
    }
    else {
        ret = hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL1_VALUE);//输出0.15V 150 mV
    }
    if (ret!=0) {
        hosal_dac_finalize(&HDMI_dac);
        blog_error("dac vlaue set fail");
        return;
    }
    blog_info("adc set value success");
    ret = hosal_dac_start(&HDMI_dac);
    if (ret!=0) {
        hosal_dac_finalize(&HDMI_dac);
        blog_error("dac start fail");
        return;
    }
    blog_info("adc satrt success");
}