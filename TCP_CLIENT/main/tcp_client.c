#include "tcp_client.h"

// Delay ms
static void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

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

//Init GPIOs and utilities
static void InitIO(void){
    ESP_LOGI(TAG, "CONFIGURE GPIO'S!");

    //RESETS
    gpio_reset_pin(LED1);
    
    //LEDS
    gpio_set_direction(LED1, GPIO_MODE_OUTPUT);

    //ADC
    ADC1_Ch0_Ini();
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
            .ssid = {0},          // Inicializa el arreglo a cero
            .password = {0},      // Inicializa el arreglo a cero
            .channel = WIFI_CHANNEL, // Puedes eliminar esta línea si no es necesaria
        }
    };

    // Copiar el SSID y la contraseña a la configuración
    strncpy((char*)wifi_configuration.sta.ssid, w_ssid, sizeof(wifi_configuration.sta.ssid) - 1);
    strncpy((char*)wifi_configuration.sta.password, w_pass, sizeof(wifi_configuration.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi initialization complete. SSID: %s  Password: %s", w_ssid, w_pass);
}

//WIFI SOFTAP
void wifi_init_softap(void) 
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = 
    {
        .ap = 
        {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) 
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    ESP_LOGI(TAG, "AP IP Address: " IPSTR, IP2STR(&ip_info.ip));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialization complete.");
    ESP_LOGI(TAG, "SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "Password: %s", WIFI_PASS);
}

// function to save in nvs
void save_to_nvs(const char* key, const char* value) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abrir NVS
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error opening NVS handle! %s", esp_err_to_name(err));
        return;
    }

    // Guardar valor
    err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error setting NVS value! %s", esp_err_to_name(err));
    }

    // Confirmar cambios
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error committing NVS changes! %s", esp_err_to_name(err));
    }

    // Cerrar NVS
    nvs_close(nvs_handle);
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

            // Initialize message with NACK
            strcpy(message, "NACK");
            
            //CONFIG RESPONSE
            //First token verify with command uabc
            token = strtok(rx_buffer, ":");
            if (token != NULL && !strcmp(token,"UABC")){
                token = strtok(NULL, ":");//Verify operation read or write
                if (token != NULL){
                    if (!strcmp(token, "W")){ 
                        token = strtok(NULL, ":"); //Element
                        if (token != NULL && !strcmp(token, "WIFI")){ //Element is WIFI or SSID
                            token = strtok(NULL, ":"); //Value
                            if(token != NULL){
                                strcpy(w_ssid, token);
                                //SAVE IN NVS
                                save_to_nvs("SSID", w_ssid);
                                //Send ssid with ack
                                snprintf(message, sizeof(message), "ACK:%s", token);
                            }   
                        }else if (token != NULL && !strcmp(token, "PASS")){ //Element is PASS
                            token = strtok(NULL, ":"); //Value
                            if(token != NULL){
                                strcpy(w_pass, token);
                                //SAVE IN NVS
                                save_to_nvs("PASS", w_pass);
                                //Send password with ack
                                snprintf(message, sizeof(message), "ACK:%s", w_pass);
                            }
                        }else if (token != NULL && !strcmp(token, "DEV")){ //Element is DEV
                            token = strtok(NULL, ":"); //Value
                            if(token != NULL){
                                strcpy(dev, token);
                                //SAVE IN NVS
                                save_to_nvs("DEV", dev);
                                //Send device name with ack
                                snprintf(message, sizeof(message), "ACK:%s", dev);
                            }
                        }
                    }else if(!strcmp(token, "R")){  
                        token = strtok(NULL, ":"); //Element
                        if (token != NULL){
                            if (!strcmp(token, "WIFI")){ //Element is WIFI
                                snprintf(message, sizeof(message), "ACK:%s", w_ssid);
                            }else if (!strcmp(token, "PASS")){ //Element is PASS
                                snprintf(message, sizeof(message), "ACK:%s", w_pass);
                            }else if (!strcmp(token, "DEV")){ //Element is dev
                                snprintf(message, sizeof(message), "ACK:%s", dev);
                            }
                        }
                    }else if(!strcmp(token, "RESET")){  
                        // restart ESP32
                        esp_restart();
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

//TCP CLIENT
static void tcp_client(void *pvParameters)
{
    char rx_buffer[128];
    char message[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    char *token;

    // Configuración de la dirección del servidor
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;

    int sock;
    int err;

    // Intentar la conexión hasta que sea exitosa
    while (1) {
        // Crear el socket
        sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reintentar
            continue; // Reintentar creación de socket
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

        err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err == 0) {
            ESP_LOGI(TAG, "Successfully connected");
            break; // Conexión exitosa, salir del bucle de reintento
        } else {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reintentar
        }
    }

    struct timeval timeout;
    timeout.tv_sec = 2;  // Tiempo en segundos
    timeout.tv_usec = 0; // Tiempo en microsegundos
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Enviar mensaje de login
    err = send(sock, CONNECT, strlen(CONNECT), 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending LOGIN: errno %d", errno);
        close(sock);
    }
    ESP_LOGI(TAG, "Login message sent: %s", CONNECT);

    delayMs(100);

    //Bucle de comunicacion
    while (1) {
        //Esperar Comando del servidor
        ESP_LOGI(TAG, "Waiting for data");
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            if(errno != EWOULDBLOCK){
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }else{
                ESP_LOGI(TAG, "Timeout");
            }
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed by server");
            break;
        } else {
            rx_buffer[len] = 0; // Null-terminate the received data
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
        }

        if (strcmp(rx_buffer,"ACK")) //Verificar que es un comando
        {
            if (_millis >= 10000 && len < 0) //Mandar keep alive si no hay mensaje.
            {
                // Enviar mensaje Keep-Alive cada 10 segundos
                err = send(sock, KEEP_ALIVE, strlen(KEEP_ALIVE), 0);
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending KEEP_ALIVE: errno %d", errno);
                    break;
                }
                ESP_LOGI(TAG, "Keep-Alive message sent: %s", KEEP_ALIVE);
                _millis=0;
            }else{
                // Initialize message with NACK
                strcpy(message, "NACK");

                //First token verify with command uabc
                token = strtok(rx_buffer, ":");
                if (token != NULL && !strcmp(token,"UABC")){
                    token = strtok(NULL, ":");
                    if (token != NULL && !strcmp(token,"IPR")){
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
                            }else if (!strcmp(token, "FACTORY")) //Reset Factory
                            {
                                ESP_ERROR_CHECK(nvs_flash_erase());
                                ESP_ERROR_CHECK(nvs_flash_init());

                                ESP_LOGI(TAG, "NVS borrado exitosamente. Reiniciando el ESP32...");
                                esp_restart();
                            } 
                        }
                    }
                }
                // Enviar Mensaje
                if (strcmp(message,"NACK"))
                {
                    err = send(sock, message, strlen(message), 0);
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending message: errno %d", errno);
                        break;
                    }
                    ESP_LOGI(TAG, "message sent: %s", message);
                }
            }
        }
    }
    
    if (sock != -1) {
        ESP_LOGI(TAG, "Shutting down socket and reconnecting...");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reconectar
    
    // Eliminar la tarea antes de retornar
    vTaskDelete(NULL);
}


//Main proccess
void app_main(void)
{
    esp_err_t ret;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Open NVS storage
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    // Try to read vars
    ret = nvs_get_str(nvs_handle, "SSID", NULL, &ssid_len);
    if (ret == ESP_OK) 
        nvs_get_str(nvs_handle, "SSID", w_ssid, &ssid_len);

    ret = nvs_get_str(nvs_handle, "PASS", NULL, &pass_len);
    if (ret == ESP_OK) 
        nvs_get_str(nvs_handle, "PASS", w_pass, &pass_len);

    ret = nvs_get_str(nvs_handle, "DEV", NULL, &dev_len);
    if (ret == ESP_OK) 
        nvs_get_str(nvs_handle, "DEV", dev, &dev_len);

    // varify all var exist
    if (ssid_len > 0 && pass_len > 0 && dev_len > 0) {
        config_complete = true;
    } else {
        config_complete = false;
    }

    // Cerrar el almacenamiento NVS
    nvs_close(nvs_handle);

    if (config_complete){
        // Normal code
        ESP_LOGI(TAG, "Configuracion completa, ejecutando codigo principal.");

        //Inicializacion de GPIOs y ADC
        InitIO();

        //Init wifi sta
        wifi_connection_sta();

        //CLIENT TCP
        xTaskCreate(tcp_client, "tcp_client", 4096, (void*)AF_INET, 5, NULL); 
    }else{
        // Config Code
        ESP_LOGI(TAG, "Variables no encontradas, ejecutando el código de configuracion.");
        ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
        wifi_init_softap();

        //Server UDP
        xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET, 5, NULL);
    }

    while (1)
    {
        _millis++;
        delayMs(1);
    }
    
}