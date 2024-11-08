#include "web_server.h"

static const char *TAG = "WEB_SERVER";

// Instancia global de la configuración del dispositivo
device_config_t device_config;

// Contenido de la página HTML con JavaScript para enviar JSON
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
                         "<form id=\"configForm\">"
                         "  WiFi Name: <input type=\"text\" name=\"wifiName\" id=\"wifiName\"><br>"
                         "  WiFi Password: <input type=\"password\" name=\"wifiPassword\" id=\"wifiPassword\"><br>"
                         "  Device Name: <input type=\"text\" name=\"deviceName\" id=\"deviceName\"><br>"
                         "  Username: <input type=\"text\" name=\"username\" id=\"username\"><br>"
                         "  <button type=\"button\" onclick=\"submitForm()\">Save Configuration</button>"
                         "</form>"
                         "</div>"
                         "<script>"
                         "function submitForm() {"
                         "    const data = {"
                         "        wifiName: document.getElementById('wifiName').value,"
                         "        wifiPassword: document.getElementById('wifiPassword').value,"
                         "        deviceName: document.getElementById('deviceName').value,"
                         "        username: document.getElementById('username').value"
                         "    };"
                         "    fetch('/config', {"
                         "        method: 'POST',"
                         "        headers: { 'Content-Type': 'application/json' },"
                         "        body: JSON.stringify(data)"
                         "    })"
                         "    .then(response => response.text())"
                         "    .then(data => alert(data))"
                         "    .catch(error => console.error('Error:', error));"
                         "}"
                         "</script>"
                         "</body>"
                         "</html>";

// Handler para servir la página de configuración
esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

// Manejador para el endpoint POST /config
esp_err_t config_post_handler(httpd_req_t *req) {
    char content[512];  // Aumento del tamaño del buffer para recibir contenido
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    ESP_LOGI(TAG, "Contenido recibido: %s", content);

    // Parsear JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "Error al analizar JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Obtener y guardar los campos en device_config
    cJSON *wifi_name = cJSON_GetObjectItem(json, "wifiName");
    cJSON *wifi_password = cJSON_GetObjectItem(json, "wifiPassword");
    cJSON *device_name = cJSON_GetObjectItem(json, "deviceName");
    cJSON *username = cJSON_GetObjectItem(json, "username");

    if (cJSON_IsString(wifi_name) && (wifi_name->valuestring != NULL)) {
        strncpy(device_config.wifi_name, wifi_name->valuestring, sizeof(device_config.wifi_name) - 1);
    }
    if (cJSON_IsString(wifi_password) && (wifi_password->valuestring != NULL)) {
        strncpy(device_config.wifi_password, wifi_password->valuestring, sizeof(device_config.wifi_password) - 1);
    }
    if (cJSON_IsString(device_name) && (device_name->valuestring != NULL)) {
        strncpy(device_config.device_name, device_name->valuestring, sizeof(device_config.device_name) - 1);
    }
    if (cJSON_IsString(username) && (username->valuestring != NULL)) {
        strncpy(device_config.username, username->valuestring, sizeof(device_config.username) - 1);
    }

    cJSON_Delete(json);
    httpd_resp_send(req, "Configuración recibida", HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "Configuración actualizada: WiFi: %s, Password: %s, Device Name: %s, Username: %s", 
             device_config.wifi_name, device_config.wifi_password, device_config.device_name, device_config.username);

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
