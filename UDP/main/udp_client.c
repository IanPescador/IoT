#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include "protocol_examples_common.h"

#define TAG "UDP_SENDER"
#define LOCAL_PORT 3333
#define REMOTE_IP "82.180.173.228"
#define REMOTE_PORT 2877
#define BUFFER_SIZE 128

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[BUFFER_SIZE];
    char response_buffer[BUFFER_SIZE];
    struct sockaddr_in local_addr, remote_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int sock, len;
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_UDP;

    // Create a UDP socket
    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket created");

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    // Bind to local port
    local_addr.sin_family = addr_family;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket bound to port %d", LOCAL_PORT);

    // Initialize remote address
    remote_addr.sin_family = addr_family;
    remote_addr.sin_port = htons(REMOTE_PORT);
    inet_pton(addr_family, REMOTE_IP, &remote_addr.sin_addr);

    while (1) {
        ESP_LOGI(TAG, "Waiting for data");
        len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            continue;
        }

        rx_buffer[len] = 0; // Null-terminate the received data
        ESP_LOGI(TAG, "Received %d bytes from client", len);
        ESP_LOGI(TAG, "Data: %s", rx_buffer);

        // Send data to remote server
        int sent_len = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        if (sent_len < 0) {
            ESP_LOGE(TAG, "sendto failed: errno %d", errno);
            continue;
        }

        // Receive response from remote server
        ESP_LOGI(TAG, "Waiting response for server");
        len = recvfrom(sock, response_buffer, sizeof(response_buffer) - 1, 0, NULL, NULL);
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            continue;
        }
        response_buffer[len] = 0; // Null-terminate the response data
        ESP_LOGI(TAG, "Received %d bytes from remote server", len);
        ESP_LOGI(TAG, "Response: %s", response_buffer);

        // Send response back to client
        sent_len = sendto(sock, response_buffer, len, 0, (struct sockaddr *)&client_addr, client_addr_len);
        if (sent_len < 0) {
            ESP_LOGE(TAG, "sendto failed: errno %d", errno);
            continue;
        }
    }

    close(sock);
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}