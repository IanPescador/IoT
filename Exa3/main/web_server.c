#include "web_server.h"

static const char *TAG = "WEB_SERVER";

// Instancia global de la configuración del dispositivo
device_config_t device_config;

// Contenido de la página HTML para enviar datos sin JSON
const char *index_html = "<!DOCTYPE HTML>"
                         "<html>"
                         "<head>"
                         "<title>Device Configuration</title>"
                         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                         "<style>"
                         "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background-color: #f5f5f5; }"
                         ".container { width: 100%; max-width: 400px; background-color: #ffffff; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); text-align: center; }"
                         "h1 { color: #333; font-size: 24px; margin-bottom: 20px; }"
                         "label { display: block; color: #666; margin-top: 10px; font-size: 14px; }"
                         "input[type=\"text\"], input[type=\"password\"] { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ccc; border-radius: 5px; font-size: 16px; }"
                         "button { margin-top: 20px; width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; font-size: 16px; cursor: pointer; }"
                         "button:hover { background-color: #45a049; }"
                         "</style>"
                         "</head>"
                         "<body>"
                         "<div class=\"container\">"
                         "<h1>Device Configuration</h1>"
                         "<form action=\"/config\" method=\"POST\">"
                         "  WiFi Name: <input type=\"text\" name=\"wifiName\" id=\"wifiName\"><br>"
                         "  WiFi Password: <input type=\"password\" name=\"wifiPassword\" id=\"wifiPassword\"><br>"
                         "  Device Name: <input type=\"text\" name=\"deviceName\" id=\"deviceName\"><br>"
                         "  Username: <input type=\"text\" name=\"username\" id=\"username\"><br>"
                         "  <button type=\"submit\">Save Configuration</button>"
                         "</form>"
                         "</div>"
                         "</body>"
                         "</html>";

// Función para analizar datos URL-encoded
static void parse_data(const char *content) {
    char *wifi_name = strstr(content, "wifiName=");
    char *wifi_password = strstr(content, "wifiPassword=");
    char *device_name = strstr(content, "deviceName=");
    char *username = strstr(content, "username=");

    if (wifi_name) sscanf(wifi_name, "wifiName=%31[^&]", device_config.wifi_name);
    if (wifi_password) sscanf(wifi_password, "wifiPassword=%63[^&]", device_config.wifi_password);
    if (device_name) sscanf(device_name, "deviceName=%31[^&]", device_config.device_name);
    if (username) sscanf(username, "username=%31[^&]", device_config.username);
}

// Handler para servir la página de configuración
esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

// Manejador para el endpoint POST /config
esp_err_t config_post_handler(httpd_req_t *req) {
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';

    ESP_LOGI(TAG, "Contenido recibido: %s", content);

    // Procesar datos URL-encoded
    parse_data(content);

    ESP_LOGI(TAG, "Configuración actualizada: WiFi: %s, Password: %s, Device Name: %s, Username: %s",
             device_config.wifi_name, device_config.wifi_password, device_config.device_name, device_config.username);

    httpd_resp_send(req, "Configuración recibida", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


// Configuración del servidor y registro de handlers
httpd_handle_t start_web_server(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 20480;

    ESP_LOGI(TAG, "Iniciando el servidor en el puerto: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registrando manejadores de URI");
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri = "/index", .method = HTTP_GET, .handler = index_handler});
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri = "/config", .method = HTTP_POST, .handler = config_post_handler});
        return server;
    }

    ESP_LOGI(TAG, "¡Error al iniciar el servidor!");
    return NULL;
}
