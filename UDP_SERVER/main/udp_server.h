#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_mac.h"
#include "esp_eth.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

bool config_complete = false;

static const char *TAG = "UDP_SERVER";

#define WIFI_SSID       "WifiConfig"
#define WIFI_PASS       "Password"
#define MAX_STA_CONN    10

// var for ssid, pass and dev
size_t ssid_len, pass_len, dev_len;
char w_ssid[32], w_pass[64], dev[32];

#define WIFI_CHANNEL    1
#define PORT 3333

#define LED1 GPIO_NUM_5

void ADC1_Ch0_Init(void);
uint16_t ADC1_Ch0_Read(void);
uint16_t ADC1_Ch0_Read_mV(void);

#endif /* MAIN_H */