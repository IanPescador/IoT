#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
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
#include "driver/ledc.h"
#include <lwip/netdb.h>

bool config_complete = false;

char *TAG = "TCP_CLIENT";
static const char *KEEP_ALIVE = "UABC:IPR:K:S:KeepAlive"; //Enviar cada 10 seg
static const char *CONNECT = "UABC:IPR:L:S:LoginServer"; //Enviar al conectar
//static const char *SMS = "UABC:IPR:M:S:6641896966:SILKSONG"; //Enviar Mensaje SMS

#define WIFI_SSID       "WifiConfig"
#define WIFI_PASS       "Password"
#define MAX_STA_CONN    10

// var for ssid, pass and dev
size_t ssid_len, pass_len, dev_len;
char w_ssid[32], w_pass[64], dev[32];
uint32_t _millis=0;

#define WIFI_CHANNEL    1

#define HOST_IP_ADDR "82.180.173.228" //iot-uabc.site  
#define PORT 8277                  

bool led_state;
#define LED1 GPIO_NUM_17 //LED NORMAL

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT 
#define LEDC_FREQUENCY          (5000) // Set frequency at 5 kHz

uint8_t PWM = 0;   //Valor entre 0 y 100 %
uint32_t DUTY_VALUE = 0; //Valor del duty para el PWM
#define LED_MAX_DUTY 8191     // Duty cycle máximo para LEDC

void ADC1_Ch0_Init(void);
uint16_t ADC1_Ch0_Read(void);
uint16_t ADC1_Ch0_Read_mV(void);
static void ledc_init(void);

#endif /* MAIN_H */