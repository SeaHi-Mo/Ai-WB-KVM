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
#include "easyflash_common.h"

static hosal_dac_dev_t HDIM_dac = {
    .config = {
        .dma_enable = false,
        .freq = 8000,
        .pin = DEVICE_HDMI_ADC_PIN,
    },
    .cb = NULL,
    .port = 0,
};

static void device_key_init(void);
static void device_adc_init(void);
/**
 * @brief HDMI 控制任务
 *
 * @param arg
 */
void device_HDMI_ctlr(void* arg)
{

    while (1) {

        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}
/**
 * @brief
 *
 */
static void device_key_init(void)
{

}
/**
 * @brief
 *
 */
static void device_adc_init(void)
{
    int ret = 0;
    ret = hosal_dac_init(&HDIM_dac);
    if (ret!=0) {
        hosal_dac_finalize(&HDIM_dac);
        blog_info("adc init false");
        return;
    }
    
    blog_info("adc init success");
}