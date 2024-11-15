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
#include "esp_mac.h"
#include "esp_eth.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

char *TAG = "SERVER";

bool validServer = false;
bool connected = false;

#define WIFI_SSID       "Pescador"
#define WIFI_PASS       "Ianilan0205"
#define MAX_STA_CONN    10

#define WIFI_CHANNEL    1

#define PORT 8266  //Servidor Normal
//#define PORT 8277  //Servidor Cifrado

#define KEEPALIVE_IDLE              120
#define KEEPALIVE_INTERVAL          10
#define KEEPALIVE_COUNT             4

// Define responses for specific messages
#define RESPONSE "ACK"

int tcp_socket = NULL;

#endif /* MAIN_H */