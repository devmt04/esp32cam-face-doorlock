#include "http.h"

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char TAG[] = "HTTPD";

/* ------------------ STREAM HANDLER ------------------ */
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    char part_buf[64];

    // Set HTTP content type for MJPEG stream
    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    if (res != ESP_OK) return res;

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        // Write JPEG header
        int header_len = snprintf(part_buf, sizeof(part_buf),
                                  "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                                  fb->len);

        res = httpd_resp_send_chunk(req, part_buf, header_len);
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            break;
        }

        vTaskDelay(30 / portTICK_PERIOD_MS);
    }

    return res;
}

/* ------------------ INDEX (HTML) HANDLER ------------------ */
static const char INDEX_HTML[] = 
"<!DOCTYPE html>"
"<html>"
"<head><meta charset='utf-8'><title>ESP32-CAM Stream</title></head>"
"<body style='margin:0; background:#000; display:flex; justify-content:center; align-items:center; height:100vh;'>"
"<img id='stream' src='/stream' style='max-width:100%; height:auto; border:3px solid #333;' />"
"</body>"
"</html>";

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static const httpd_uri_t uri_index = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
};

static const httpd_uri_t uri_stream = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
};



/* ------------------ START SERVER ------------------ */
esp_err_t http_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_index);
        httpd_register_uri_handler(server, &uri_stream);
        ESP_LOGI(TAG, "HTTP server started");
    }
    // return server;
    return ESP_OK;
}

