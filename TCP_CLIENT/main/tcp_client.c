#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "protocol_examples_common.h"

#define HOST_IP_ADDR "192.168.4.88"  // Cambia esta dirección IP al IP de tu servidor
#define PORT 3333                    // Cambia este número al puerto que quieras usar

static const char *TAG = "TCP_CLIENT";
static const char *MESSAGE = "Hola estoy vivo";

static void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static void tcp_client(void *pvParameters)
{
    char rx_buffer[128];
    struct sockaddr_in dest_addr;
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    // Configuración de la dirección del servidor
    inet_pton(AF_INET, HOST_IP_ADDR, &dest_addr.sin_addr);
    dest_addr.sin_family = addr_family;
    dest_addr.sin_port = htons(PORT);

    while (1) {
        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reintentar
            continue;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reintentar
            continue;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            int err = send(sock, MESSAGE, strlen(MESSAGE), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGW(TAG, "Connection closed by server");
                break;
            } else {
                rx_buffer[len] = 0; // Null-terminate the received data
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            }
            delayMs(10000);
        }

        if (sock != -1) {
            ESP_LOGI(TAG, "Shutting down socket and reconnecting...");
            shutdown(sock, 0);
            close(sock);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reconectar
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    // Crear la tarea del cliente TCP
    xTaskCreate(tcp_client, "tcp_client", 4096, NULL, 5, NULL);
}