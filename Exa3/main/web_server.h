// web_server.h
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <esp_http_server.h>
#include <esp_log.h>
#include <string.h>
#include "cJSON.h"

// Estructura para almacenar la configuración del dispositivo
typedef struct {
    char wifi_name[32];
    char wifi_password[64];
    char device_name[32];
    char username[32];
} device_config_t;

extern device_config_t device_config;

// Inicializar el servidor web
httpd_handle_t start_web_server(void);

#endif // WEB_SERVER_H
