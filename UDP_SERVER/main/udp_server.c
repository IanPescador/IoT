#include "udp_server.h"

// Inititilization of ADC1 Channel 0 (VP on ESP32)
void ADC1_Ch0_Ini(void){
    //ADC1_0 config
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
}

// Reading from ADC1 Channel 0 (VP on ESP32)
uint16_t ADC1_Ch0_Read(void){
    return adc1_get_raw(ADC1_CHANNEL_0);
}

// Uses ADC1_Ch0_Read to convert to mV,
uint16_t ADC1_Ch0_Read_mV(void){
    //Make the appropiate convertion of the RAW ADC value to mV by calling ADC1_Ch0_Read()
    return (ADC1_Ch0_Read() * 3300) / 4095;
}

static void InitIO(void){
    ESP_LOGI(TAG, "CONFIGURE GPIO'S!");

    //RESETS
    gpio_reset_pin(LED1);
    
    //LEDS
    gpio_set_direction(LED1, GPIO_MODE_OUTPUT);

    //ADC
    ADC1_Ch0_Ini();
}

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char message[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in dest_addr;
    char *token;
    bool led_state = false;

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

            // Initialize message with NACK
            strcpy(message, "NACK");

            //First token verify with command uabc
            token = strtok(rx_buffer, ":");
            if (token != NULL && !strcmp(token,"UABC")){
                token = strtok(NULL, ":");//Verify operation read or write
                if (token != NULL){
                    if (!strcmp(token, "W")){ //Only led
                        token = strtok(NULL, ":"); //Element
                        if (token != NULL && !strcmp(token, "L")){ //Element is LED
                            token = strtok(NULL, ":"); //Value
                            if(token != NULL){
                                if (!strcmp(token, "1")){ //Turn on LED
                                    led_state = true;
                                    gpio_set_level(LED1, led_state);
                                    snprintf(message, sizeof(message), "ACK:%d", led_state);
                                }else if (!strcmp(token, "0")){ //Turn off LED
                                    led_state = false;
                                    gpio_set_level(LED1, led_state);
                                    snprintf(message, sizeof(message), "ACK:%d", led_state);
                                }  
                            }   
                        } 
                    }else if(!strcmp(token, "R")){ //Led and ADC 
                        token = strtok(NULL, ":"); //Element
                        if (token != NULL){
                            if (!strcmp(token, "L")){ //Element is LED
                                snprintf(message, sizeof(message), "ACK:%d", led_state);
                            }else if (!strcmp(token, "A")){ //Element is ADC
                                snprintf(message, sizeof(message), "ACK:%d", ADC1_Ch0_Read_mV());
                            }
                        }
                    }
                } 
            }
            int err = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            if (err < 0) 
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
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

void app_main(void)
{
    //Inicializacion de GPIOs y ADC
    InitIO();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET, 5, NULL);
}