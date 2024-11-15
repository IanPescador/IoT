#include "main.h"

// Delay ms
static void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

//handler event wifi sta
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    if (event_id == WIFI_EVENT_STA_START) 
    {
        ESP_LOGI(TAG, "Connecting to the master's Wi-Fi network...");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) 
    {
        ESP_LOGI(TAG, "Connected to the master's Wi-Fi network");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGE(TAG, "Disconnected from the master's Wi-Fi network, reason: %d", event->reason);
        esp_wifi_connect();

    } else if (event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

//Connect wifi sta
void wifi_connection_sta() 
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&wifi_initiation);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    // Definir la configuración Wi-Fi
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = WIFI_SSID,          // Inicializa el arreglo a cero
            .password = WIFI_PASS,      // Inicializa el arreglo a cero
            .channel = WIFI_CHANNEL, // Puedes eliminar esta línea si no es necesaria
        }
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi initialization complete. SSID: %s  Password: %s", WIFI_SSID, WIFI_PASS);
}

//UDP SERVER
static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char message[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in dest_addr;
    char *token;

    if (addr_family != AF_INET) {
        ESP_LOGE(TAG, "Invalid address family, must be AF_INET");
        vTaskDelete(NULL);
    }

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;

    while (1) {
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (1) {
            ESP_LOGI(TAG, "Waiting for data");
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }

            // Data received
            // Get the sender's ip address as string
            if (source_addr.ss_family == AF_INET) {
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            }

            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
            ESP_LOGI(TAG, "%s", rx_buffer);
            strcpy(message, rx_buffer);
            
            //MAIN RESPONSE
            //First token verify with command uabc
            token = strtok(rx_buffer, ":");
            if (token != NULL && !strcmp(token,"UABC")){
                token = strtok(NULL, ":");//Verify operation read or write
                if (token != NULL){
                    if (!strcmp(token, "IPR") || !strcmp(token, "LAN")){ //Verify User
                        printf("MENSAJE: %s\n",message);
                        printf("MENSAJE VALIDO\n");
                        if (tcp_socket)
                        {
                            send(tcp_socket, message , strlen(message), 0);
                        }
                    }
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

static void do_retransmit(const int sock)
{
    char *token;
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
            return;
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            
            validServer = false;

            //First token verify with command uabc
            token = strtok(rx_buffer, ":");
            if (token != NULL && !strcmp(token,"UABC"))
            {
                token = strtok(NULL, ":");
                if (!strcmp(token,"LAN") || !strcmp(token,"IPR")){
                    token = strtok(NULL, ":");
                    token = strtok(NULL, ":");
                    if (!strcmp(token,"L")){
                        token = strtok(NULL, ":");
                        if (!strcmp(token,"S")){
                            send(sock, RESPONSE , strlen(RESPONSE), 0);
                            validServer = true;
                            connected = true;
                        }
                    }else if (!strcmp(token,"K") && connected){
                        token = strtok(NULL, ":");
                        if (!strcmp(token,"S")){
                            send(sock, RESPONSE , strlen(RESPONSE), 0);
                            validServer = true;
                        }
                    }
                }
            }else if (token != NULL && !strcmp(token,"ACK") && connected){
                printf("ACK\n");
                validServer = true;
            }
            
            if (!validServer)
            {
                return;
            }
        }
    } while (1);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {
        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }else{
            printf("SOCKET TCP: %d", sock);
            tcp_socket = sock;
        }

        // Set TCP keepalive options
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // Convert IP address to string
        inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted IP address: %s", addr_str);

        do_retransmit(sock);

        printf("ADIOS SOCK\n");
        tcp_socket = NULL;

        connected = false;
        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

//Main proccess
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Init wifi sta
    wifi_connection_sta();

    //Server UDP
    xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET, 4, NULL);

    //Server TCP
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL); 

    while (1)
    {
        delayMs(1);
    }
}
