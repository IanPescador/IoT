#include <esp_wifi.h>
#include <esp_event.h>
#include <string.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t index_handler(httpd_req_t *req);
esp_err_t data_get_handler(httpd_req_t *req);
httpd_handle_t start_webserver(void);