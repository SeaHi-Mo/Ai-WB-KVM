/**
 * @file device.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef DEVICE_H
#define DECVICE_H

#define DEVICE_HDMI_ADC_PIN 14

#define DEVICE_KEY_CHANNEL1_PIN  11  //通道1按键
#define DEVICE_KEY_CHANNEL2_PIN  17  //通道2按键
#define DEVICE_KEY_CHANNEL3_PIN  2  //通道3按键
#define DEVICE_CHANNEL1_IN_PIN   20  //通道1 识别
#define DEVICE_CHANNEL2_IN_PIN   21 //通道2 识别
#define DEVICE_CHANNEL3_IN_PIN   22  //通道3 识别

#define DEVICE_USB_SWITCH_PIN 12      //USB 控制
#define DEVICE_USB_IN0_PIN 5
#define DEVICE_USB_IN1_PIN 1
#define DEVICE_USB_IN_EN 3

#define DEVICE_CHANNEL1_VALUE 200000 
#define DEVICE_CHANNEL2_VALUE 900000
#define DEVICE_CHANNEL3_VALUE 1800000

extern TaskHandle_t key_check_task;
void device_HDMI_ctlr(void* arg);

#endif