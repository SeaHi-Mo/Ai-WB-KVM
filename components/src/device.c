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

#define DAC_KEY "HDMIDAC"

typedef enum {
    CHANNEL_BIT_1 = 0X01,
    CHANNEL_BIT_2,
    CHANNEL_BIT_3,
}channel_bit_t;

static hosal_dac_dev_t HDMI_dac = {
    .config = {
        .dma_enable = false,
        .freq = 8000,
        .pin = DEVICE_HDMI_ADC_PIN,
    },
    .cb = NULL,
    .port = 0,
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
    device_key_init();
    device_usbHub_pin_init();
    device_adc_init();
    while (1) {
        //进行按键检测
        if (!bl_gpio_input_get_value(DEVICE_CHANNEL1_IN_PIN)) {
            while (!bl_gpio_input_get_value(DEVICE_CHANNEL1_IN_PIN)) vTaskDelay(50/portTICK_PERIOD_MS);
            hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL1_VALUE);
            ef_set_u32(DAC_KEY, DEVICE_CHANNEL1_VALUE);
            blog_info("HDMI channel is 1");
        }

        if (!bl_gpio_input_get_value(DEVICE_CHANNEL2_IN_PIN)) {
            while (!bl_gpio_input_get_value(DEVICE_CHANNEL2_IN_PIN)) vTaskDelay(50/portTICK_PERIOD_MS);
            hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL2_VALUE);
            ef_set_u32(DAC_KEY, DEVICE_CHANNEL2_VALUE);
            blog_info("HDMI channel is 2");
        }

        if (!bl_gpio_input_get_value(DEVICE_CHANNEL3_IN_PIN)) {
            while (!bl_gpio_input_get_value(DEVICE_CHANNEL3_IN_PIN)) vTaskDelay(50/portTICK_PERIOD_MS);
            hosal_dac_set_value(&HDMI_dac, DEVICE_CHANNEL3_VALUE);
            ef_set_u32(DAC_KEY, DEVICE_CHANNEL3_VALUE);
            blog_info("HDMI channel is 3");
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
    bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1);//先关闭USB HUB
    //切换设备
    bl_gpio_output_set(DEVICE_USB_IN0_PIN, 0);
    bl_gpio_output_set(DEVICE_USB_IN1_PIN, 0);
    bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB
}
/**
 * @brief  HDMI 通道2 识别中断
 *
 * @param arg
 */
static void device_channel2_irq(void* arg)
{
    bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1); //先关闭USB HUB
    //切换设备
    bl_gpio_output_set(DEVICE_USB_IN0_PIN, 0);
    bl_gpio_output_set(DEVICE_USB_IN1_PIN, 1);
    bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB
}
/**
 * @brief  HDMI 通道3 识别中断
 *
 * @param arg
 */
static void device_channel3_irq(void* arg)
{
    bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 1);//先关闭USB HUB
    //切换设备
    bl_gpio_output_set(DEVICE_USB_IN0_PIN, 1);
    bl_gpio_output_set(DEVICE_USB_IN1_PIN, 0);
    bl_gpio_output_set(DEVICE_USB_SWITCH_PIN, 0);//打开USB HUB
}
/**
 * @brief
 *
 */
static void device_key_init(void)
{
    bl_gpio_enable_output(DEVICE_KEY_CHANNEL1_PIN, true, false);
    bl_gpio_enable_output(DEVICE_KEY_CHANNEL2_PIN, true, false);
    bl_gpio_enable_output(DEVICE_KEY_CHANNEL3_PIN, true, false);
    hosal_gpio_init(&device_channel1);
    hosal_gpio_init(&device_channel2);
    hosal_gpio_init(&device_channel3);

    hosal_gpio_irq_set(&device_channel1, HOSAL_IRQ_TRIG_POS_PULSE, device_channel1_irq, NULL);
    hosal_gpio_irq_set(&device_channel2, HOSAL_IRQ_TRIG_POS_PULSE, device_channel2_irq, NULL);
    hosal_gpio_irq_set(&device_channel3, HOSAL_IRQ_TRIG_POS_PULSE, device_channel3_irq, NULL);
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