#ifndef WIFI_H
#define WIFI_H

#define WIFI_SSID "my-esp32s3"
#define WIFI_PASSWORD "123124578"
#define WIFI_CHANNEL 1
#define MAX_STA_CONN 1

esp_err_t init_wifi();

#endif