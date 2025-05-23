#include "main.h"
#include "web_server.h"

// Delay ms
static void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

// // Cifrar palabra
// static void cifrar(char *cif, const char *string, int size_str, const char *clave, int size_clave) {
//     uint8_t i = 0, j = 0;
//     for (i = 0; i < size_str; i++) {
//         cif[i] = string[i] ^ clave[j];
//       
//         j++;
//         if (j >= size_clave)
//             j = 0;
//     }
//     cif[i] = '\0';  // Terminar la cadena con '\0'
// }

// // Descifrar
// static void descifrar (char *string, char *clave){
//     int8_t i = 0;
//     while (string[i] != '\r')
//     {
//         clave[i] = string[i] ^ 0x55; 
//         i++;
//     }
//     clave[i] = 0;
// }

// //GET PASSWORD
// static void get_password(char *password, char *message){
//     char *token;
//
//     //First token verify with password ACK
//     token = strtok(message, ":");
//     if (token != NULL && !strcmp(token,"ACK")){
//         token = strtok(NULL, ":"); //Take password from server
//         descifrar(token, password);
//         ESP_LOGI(TAG, "Received %s Len %d", password, strlen(password));
//     }
// }

//Init led PWM
static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
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
    //gpio_reset_pin(BUTTON);
    
    //LEDS
    gpio_set_direction(LED1, GPIO_MODE_OUTPUT);

    //BUTTON
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLDOWN_ONLY);
    
    //ADC
    ADC1_Ch0_Ini();

    //LED PWM
    ledc_init();
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

// static void log_error_if_nonzero(const char *message, int error_code)
// {
//     if (error_code != 0) {
//         ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
//     }
// }

//MQTT HANDLER EVENT
// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
// {
//     ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
//     esp_mqtt_event_handle_t event = event_data;
//     esp_mqtt_client_handle_t client = event->client;
//     int msg_id;
//     switch ((esp_mqtt_event_id_t)event_id) {
//     case MQTT_EVENT_CONNECTED:
//         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//         break;
//     case MQTT_EVENT_DISCONNECTED:
//         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//         break;
//     case MQTT_EVENT_SUBSCRIBED:
//         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_UNSUBSCRIBED:
//         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_PUBLISHED:
//         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_DATA:
//         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//         printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//         printf("DATA=%.*s\r\n", event->data_len, event->data);
//         break;
//     case MQTT_EVENT_ERROR:
//         ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//         if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
//             log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
//             log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
//             log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
//             ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
//
//         }
//         break;
//     default:
//         ESP_LOGI(TAG, "Other event id:%d", event->event_id);
//         break;
//     }
// }

//MQTT START
// static void mqtt_app_start(void)
// {
//     esp_mqtt_client_config_t mqtt_cfg = {
//         .broker.address.uri = CONFIG_BROKER_URL,
//         .credentials.username = USERNAME,
//         .credentials.authentication.password = PASSWORD,
//     };
//
//     mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
//     /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
//     esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
//     esp_mqtt_client_start(mqtt_client);
// }

//CREATE TCP SOCKET
int create_tcp_socket(char* IP, int Port) {
    int sock, err;
    struct addrinfo hints, *res;
    char port_str[6];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    snprintf(port_str, sizeof(port_str), "%d", Port); // Convertir puerto a cadena

    // Resolver nombre de dominio a dirección IP
    err = getaddrinfo(IP, port_str, &hints, &res);
    if (err != 0) {
        ESP_LOGE(TAG, "getaddrinfo failed");
        return -1;
    }

    while (1) {
        // Crear el socket
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reintentar
            continue; // Reintentar creación de socket
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", IP, Port);

        // Intentar conectar
        err = connect(sock, res->ai_addr, res->ai_addrlen);
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

    freeaddrinfo(res); // Liberar la memoria asignada por getaddrinfo

    return sock; // Regresar el socket creado y configurado
}

//SEND TO SERVER
static void send_server_tcp(int sock, char* message){
    int err;
    ESP_LOGI(TAG, "Message size: %d", strlen(message));
    err = send(sock, message, strlen(message), 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending LOGIN: errno %d", errno);
        close(sock);
    }
    ESP_LOGI(TAG, "Message sent: %s", message);
}

//RECEIVE SERVER
int receive_server_tcp(int sock, char* receive, int buf_size) {
    ESP_LOGI(TAG, "Waiting for data");
    int len = recv(sock, receive, buf_size - 1, 0);  // Usamos buf_size - 1 para dejar espacio para el terminador null
    if (len < 0) {
        if(errno != EWOULDBLOCK){
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            return -2;
        }else{
            ESP_LOGI(TAG, "Timeout");
        }
    } else if (len == 0) {
        ESP_LOGW(TAG, "Connection closed by server");
    } else {
        ESP_LOGI(TAG, "Received %d bytes: %s", len, receive);
        receive[len] = 0; // Null-terminate the received data
    }
    return len;
}

//TCP CLIENT
static void tcp_client(void *pvParameters)
{
    char rx_buffer[128];
    char message[128];
    //char mqtt_message[128];
    //char cifrado[128];
    //char password[20];
    char *token;

    int sock;
    int len;
    //int msg_id;

    while (1)
    {
        if(internet){
            sock = create_tcp_socket(HOST_IP_ADDR, PORT);
        }else{
            sock = create_tcp_socket(ALTERNATIVE_IP, PORT);
        }
        
        // Enviar mensaje de login
        send_server_tcp(sock, CONNECT);

        delayMs(100);

        //Esperar Contraseña del servidor
        //len = receive_server_tcp(sock, rx_buffer, sizeof(rx_buffer));  
        
        //Get password from server
        //get_password(password, rx_buffer);

        //Bucle de comunicacion
        while (1) {
            //Esperar Comando del servidor
            len = receive_server_tcp(sock, rx_buffer, sizeof(rx_buffer));

            if (len == -2)
            {
                ESP_LOGI(TAG, "Shutting down socket and reconnecting...");
                shutdown(sock, 0);
                close(sock);
                
                vTaskDelay(pdMS_TO_TICKS(1000)); // Retraso antes de reconectar
                break;
            }else if(len > 0){
                //Descrifrar mensaje recibido
                //cifrar(cifrado, rx_buffer, len, password, strlen(password));
                //ESP_LOGI(TAG, "Receive cifrado: %s", cifrado);

                // Initialize message with NACK
                strcpy(message, "NACK");

                //First token verify with command uabc
                token = strtok(rx_buffer, ":");
                if (token != NULL && !strcmp(token,"UABC")){
                    token = strtok(NULL, ":");
                    if (token != NULL && !strcmp(token,user)){
                        token = strtok(NULL, ":");
                        if (token != NULL && !strcmp(token,dev)){
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
                                            //snprintf(mqtt_message, sizeof(mqtt_message), "%d:IPR", led_state);
                                            //msg_id = esp_mqtt_client_publish(mqtt_client, "device/led", mqtt_message, 0, 1, 0);
                                            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                                        }   
                                    }else if (token != NULL && !strcmp(token, "P"))
                                    {
                                        token = strtok(NULL, ":"); //Value
                                        if(token != NULL){
                                            if (!strcmp(token, "0"))
                                            {
                                                PWM = 0;
                                                DUTY_VALUE = 0; //Valor en 0
                                                // Establecer el nuevo valor de intensidad del LED
                                                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, DUTY_VALUE));
                                                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

                                                snprintf(message, sizeof(message), "ACK:%d", PWM);
                                            }else{
                                                PWM = atoi(token);
                                                DUTY_VALUE = LED_MAX_DUTY * PWM / 100; //Sacar porcentaje
                                                // Establecer el nuevo valor de intensidad del LED
                                                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, DUTY_VALUE));
                                                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

                                                snprintf(message, sizeof(message), "ACK:%d", PWM);
                                            }
                                        }
                                    }
                                }else if(!strcmp(token, "R")){ //Led and ADC 
                                    token = strtok(NULL, ":"); //Element
                                    if (token != NULL){
                                        if (!strcmp(token, "L")){ //Element is LED
                                            snprintf(message, sizeof(message), "ACK:%d", led_state);

                                            //snprintf(mqtt_message, sizeof(mqtt_message), "%d:IPR", led_state);
                                            //msg_id = esp_mqtt_client_publish(mqtt_client, "device/led", mqtt_message, 0, 1, 0);
                                            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                                        }else if (!strcmp(token, "A")){ //Element is ADC
                                            snprintf(message, sizeof(message), "ACK:%d", ADC1_Ch0_Read_mV());
                                        }else if (!strcmp(token, "P")){ //Element is PWM
                                            snprintf(message, sizeof(message), "ACK:%d", PWM);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // Cifrar mensaje
                    //cifrar(cifrado, message, strlen(message), password, strlen(password));

                    // Enviar Mensaje
                    send_server_tcp(sock, message);
                }
            }else if (button_pressed)
            {
                ESP_LOGI(TAG, "Botón presionado, enviando mensaje al servidor");
                //Cifrar el mensaje antes de enviarlo
                //cifrar(cifrado, SMS, strlen(SMS), password, strlen(password));

                //Enviar mensaje cifrado al servidor
                send_server_tcp(sock, SMS);

                if (xSemaphoreTake(xMutex, portMAX_DELAY))
                {
                button_pressed = false;
                cooldown_message = true;
                xSemaphoreGive(xMutex);
                }

                
                _millisCooldown = 0;
            }else if (_millis >= 10000) //Mandar keep alive si no hay mensaje.
            {
                // Enviar mensaje Keep-Alive cada 10 segundos
                //cifrar(cifrado, KEEP_ALIVE, strlen(KEEP_ALIVE), password, strlen(password));

                send_server_tcp(sock, KEEP_ALIVE);
                _millis=0;
            }
        }
        delayMs(100);
    }
    // Eliminar la tarea antes de retornar
    vTaskDelete(NULL);
}

//CLOCK TASK
static void clockTask(void *pvParameters)
{
    uint8_t millis = 0, sec = 0, min = 0, hour = 0, api_millis = 0, api_redondeo = 0, api_sec = 0, api_min = 0, api_hour = 0, lastHour = 0, lastMin = 0, lastSec = 0;
    bool checkApi = true;
    int sock_api, len;
    char api_response[1024], timeNew[9], *newHour;

    // Formatear GET request
    const char *get_request = 
    "GET /api/timezone/America/Tijuana.txt HTTP/1.1\r\n"
    "Host: worldtimeapi.org\r\n"
    "Connection: close\r\n"
    "\r\n";

    while (1) {
        if (checkApi)
        {
            checkApi = false;
            sock_api = create_tcp_socket("worldtimeapi.org", 80);
            if (sock_api < 0) {
                ESP_LOGE(TAG, "Error creando socket");
                internet = false;
                continue;
            }

            send_server_tcp(sock_api, (char *)get_request);
            len = receive_server_tcp(sock_api, api_response, sizeof(api_response));
            close(sock_api);

            if(len > 0){
                internet = true;
                // Si se reciben menos datos que el tamaño del buffer, copiar la respuesta
                newHour = strstr(api_response, "datetime");
                if(newHour){
                    newHour = strstr(newHour+1,"datetime");
                    if (newHour) {
                        newHour = strstr(newHour, "T");
                        if (newHour) {
                            // Copiar los primeros 8 caracteres después de la "T" (formato hora)
                            sscanf(newHour + 1, "%8s", timeNew); 
                            sscanf(timeNew, "%2hhd:%2hhd:%2hhd.%2hhd%1hhd", &api_hour, &api_min, &api_sec, &api_millis, &api_redondeo);
                            if (api_redondeo > 5){
                                api_millis++;
                            }
                            ESP_LOGI(TAG, "Hora API: %02d:%02d:%02d.%d\n", api_hour, api_min, api_sec, api_millis);

                            if(hour != api_hour || min != api_min){
                                ESP_LOGE(TAG, "CORRECCION DE HORA");
                                hour = api_hour;
                                min = api_min;
                                sec = api_sec;
                                millis = api_millis;
                            }
                            lastHour = hour;
                            lastMin = min;
                            lastSec = sec;
                        }
                    }
                } 
            }else{
                ESP_LOGE(TAG, "No response API no internet");
                internet = false;
                continue;
            }
        }
        
        // Incrementar segundos
        millis++;
        if (millis >= 100){
            millis = 0;
            sec++;
            // Mostrar el tiempo en consola
            ESP_LOGE(TAG, "Hora actual: %02d:%02d:%02d.%d", hour, min, sec, millis);
            if (sec == 60) {
                sec = 0;
                min++;
                if (min == 60) {
                    min = 0;
                    hour++;
                    if (hour == 24) {
                        hour = 0;
                    }
                }
            }
        }

        if ((hour - lastHour + 24) % 24 >= 1 && min == lastMin && sec == lastSec){
            checkApi = true;
        }

        // Pausar la tarea por 100 millis
        delayMs(10);
    }

    vTaskDelete(NULL);
}

//Main proccess
void app_main(void)
{
    bool config_complete = false;
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
    
    ret = nvs_get_str(nvs_handle, "USER", NULL, &user_len);
    if (ret == ESP_OK) 
        nvs_get_str(nvs_handle, "USER", user, &user_len);

    //printf("SSID: %s    PASS: %s    DEV: %s    USER: %s\n",w_ssid, w_pass, dev, user);
    //printf("LEN SSID: %d    PASS: %d    DEV: %d    USER: %d\n",ssid_len, pass_len, dev_len, user_len);

    // varify all var exist
    if (ssid_len > 0 && pass_len > 0 && dev_len > 0 && user_len > 0) {
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

        //Init comandos staticos
        snprintf(KEEP_ALIVE, sizeof(KEEP_ALIVE), "UABC:%s:%s:K:S:KeepAlive", user, dev);
        snprintf(CONNECT, sizeof(CONNECT), "UABC:%s:%s:L:S:LoginServer", user, dev);
        snprintf(SMS, sizeof(SMS), "UABC:%s:%s:M:S:6641896966:SILKSONG", user, dev);     

        //Init wifi sta
        wifi_connection_sta();

        delayMs(5000);

        //Init MQTT
        //mqtt_app_start();

        xMutex = xSemaphoreCreateMutex();

        //CLOCK
        xTaskCreate(clockTask, "clockTask", 8192, NULL, 4, NULL); 

        //WAIT CLOCK RESPONSE
        delayMs(2000);

        //CLIENT TCP
        xTaskCreate(tcp_client, "tcp_client", 4096, (void*)AF_INET, 5, NULL); 

        while (1)
        {
            static bool button_state = true; 
            static bool last_button_state = true; 

            // Leer el estado actual del botón
            button_state = gpio_get_level(BUTTON);

            // Detectar el flanco ascendente (transición de 0 a 1)
            if (!button_state && last_button_state && !cooldown_message) {
                ESP_LOGI(TAG, "Botón presionado");
                if (xSemaphoreTake(xMutex, portMAX_DELAY))
                {
                button_pressed = true;
                xSemaphoreGive(xMutex);
                }
            }

            // Guardar el estado actual como el último estado para la próxima detección
            last_button_state = button_state;

            _millis++;
            if (cooldown_message)
                _millisCooldown++;
            if (cooldown_message && _millisCooldown > 1000 * 60){
                if (xSemaphoreTake(xMutex, portMAX_DELAY))
                {
                cooldown_message = false;
                xSemaphoreGive(xMutex);
                }
            } 
    
            delayMs(1);
        }
    }else{
        // Config Code
        ESP_LOGI(TAG, "Variables no encontradas, ejecutando el código de configuracion.");
        ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
        wifi_init_softap();

        // Start the web server
        start_web_server();

        while(1){
            if (strlen(device_config.device_name) > 0 && strlen(device_config.username) > 0 && strlen(device_config.wifi_name) > 0 && strlen(device_config.wifi_password) > 0)
            {
                //SAVE IN NVS
                save_to_nvs("SSID", device_config.wifi_name);
                save_to_nvs("PASS", device_config.wifi_password);
                save_to_nvs("DEV", device_config.device_name);
                save_to_nvs("USER", device_config.username);

                delayMs(1000);

                esp_restart();
            }
            delayMs(1);
        }
    }
}
