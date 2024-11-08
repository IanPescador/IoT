#include "web_server.h"

static const char *TAG = "WEB_SERVER";

// HTML page content
const char *index_html = "<!DOCTYPE HTML>"
                         "<html>"
                         "<head>"
                         "<title>Device Configuration</title>"
                         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                         "<style>"
                         "body {"
                         "  font-family: Arial, sans-serif;"
                         "  display: flex;"
                         "  justify-content: center;"
                         "  align-items: center;"
                         "  height: 100vh;"
                         "  margin: 0;"
                         "  background-color: #f5f5f5;"
                         "}"
                         ".container {"
                         "  width: 100%;"
                         "  max-width: 400px;"
                         "  background-color: #ffffff;"
                         "  padding: 20px;"
                         "  border-radius: 10px;"
                         "  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);"
                         "  text-align: center;"
                         "}"
                         "h1 {"
                         "  color: #333;"
                         "  font-size: 24px;"
                         "  margin-bottom: 20px;"
                         "}"
                         "label {"
                         "  display: block;"
                         "  color: #666;"
                         "  margin-top: 10px;"
                         "  font-size: 14px;"
                         "}"
                         "input[type=\"text\"], input[type=\"password\"] {"
                         "  width: 100%;"
                         "  padding: 10px;"
                         "  margin-top: 5px;"
                         "  border: 1px solid #ccc;"
                         "  border-radius: 5px;"
                         "  font-size: 16px;"
                         "}"
                         "button {"
                         "  margin-top: 20px;"
                         "  width: 100%;"
                         "  padding: 10px;"
                         "  background-color: #4CAF50;"
                         "  color: white;"
                         "  border: none;"
                         "  border-radius: 5px;"
                         "  font-size: 16px;"
                         "  cursor: pointer;"
                         "}"
                         "button:hover {"
                         "  background-color: #45a049;"
                         "}"
                         ".reboot-button {"
                         "  margin-top: 10px;"
                         "  background-color: #FF6347;"
                         "}"
                         ".reboot-button:hover {"
                         "  background-color: #FF4500;"
                         "}"
                         "</style>"
                         "</head>"
                         "<body>"
                         "<div class=\"container\">"
                         "  <h1>Device Configuration</h1>"
                         "  <form onsubmit=\"submitConfig(event)\">"
                         "    <label for=\"wifiName\">WiFi Network Name:</label>"
                         "    <input type=\"text\" id=\"wifiName\" name=\"wifiName\" required>"
                         "    <label for=\"wifiPassword\">WiFi Password:</label>"
                         "    <input type=\"password\" id=\"wifiPassword\" name=\"wifiPassword\" required>"
                         "    <label for=\"deviceName\">Device Name:</label>"
                         "    <input type=\"text\" id=\"deviceName\" name=\"deviceName\" required>"
                         "    <label for=\"username\">Username:</label>"
                         "    <input type=\"text\" id=\"username\" name=\"username\" required>"
                         "    <button type=\"submit\">Save Configuration</button>"
                         "  </form>"
                         "  <button class=\"reboot-button\" onclick=\"rebootDevice()\">Reboot Device</button>"
                         "</div>"
                         "<script>"
                         "async function submitConfig(event) {"
                         "  event.preventDefault();"
                         "  const config = {"
                         "    wifiName: document.getElementById('wifiName').value,"
                         "    wifiPassword: document.getElementById('wifiPassword').value,"
                         "    deviceName: document.getElementById('deviceName').value,"
                         "    username: document.getElementById('username').value"
                         "  };"
                         "  try {"
                         "    const response = await fetch('/config', {"
                         "      method: 'POST',"
                         "      headers: { 'Content-Type': 'application/json' },"
                         "      body: JSON.stringify(config)"
                         "    });"
                         "    if (response.ok) {"
                         "      alert('Configuration saved successfully');"
                         "    } else {"
                         "      alert('Error saving configuration');"
                         "    }"
                         "  } catch (error) {"
                         "    console.error('Request error:', error);"
                         "    alert('Request error');"
                         "  }"
                         "}"
                         "function rebootDevice() {"
                         "  fetch('/reboot', { method: 'POST' })"
                         "    .then(response => {"
                         "      if (response.ok) {"
                         "        alert('Device is rebooting...');"
                         "      } else {"
                         "        alert('Error rebooting device');"
                         "      }"
                         "    })"
                         "    .catch(error => console.error('Request error:', error));"
                         "}"
                         "</script>"
                         "</body>"
                         "</html>";

// Handler para servir la página de configuración
esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

static const httpd_uri_t index_uri = {
    .uri = "/index",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
};

// Handler para recibir los datos de configuración
esp_err_t config_post_handler(httpd_req_t *req) {
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0) { 
        return ESP_FAIL;
    }
    content[ret] = '\0';
    ESP_LOGI(TAG, "Datos de configuración recibidos: %s", content);
    httpd_resp_send(req, "Configuración recibida", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler para el botón de reboot
esp_err_t reboot_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "rebootting device...");
    // Lógica para reiniciar el dispositivo
    httpd_resp_send(req, "Device rebootting...", HTTPD_RESP_USE_STRLEN);
    // Aquí se puede agregar el código para reiniciar el dispositivo
    return ESP_OK;
}

static const httpd_uri_t config_post_uri = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = config_post_handler,
    .user_ctx = NULL
};

static const httpd_uri_t reboot_post_uri = {
    .uri = "/reboot",
    .method = HTTP_POST,
    .handler = reboot_post_handler,
    .user_ctx = NULL
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 20480;

    ESP_LOGI(TAG, "Iniciando el servidor en el puerto: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registrando manejadores de URI");
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &config_post_uri);
        httpd_register_uri_handler(server, &reboot_post_uri);  // Registrar el handler de reboot
        return server;
    }

    ESP_LOGI(TAG, "¡Error al iniciar el servidor!");
    return NULL;
}