#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "cam/cam.h"
#include "uart/uart.h"
#include "wifi/wifi.h"
#include "http/http.h"
#include "esp_timer.h"

void app_main(void)
{
    // set GPIO 13 as output for LED, Buzzer and Door lock control
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 13),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // create one-shot timer for GPIO 13, which will automatically turn off the LED/Buzzer/Door lock after 3 seconds
    const esp_timer_create_args_t t13_args = {
        .callback = &timer13_callback, // this funtion will be called when timer expires
        .name = "timer13"
    };
    esp_timer_create(&t13_args, &timer13);

  
#ifdef ESP_CAMERA_SUPPORTED
    esp_log_level_set("*", ESP_LOG_NONE); // disable all logs, for cleaner output and seemless UART communication

    if(enable_uart() != ESP_OK) return; // enable UART communication between python and esp
    if(init_camera() != ESP_OK) return; // initilize camera module
    if(init_wifi() != ESP_OK) return;   // initilize wifi module
    if(http_server_init() != ESP_OK) return; // start HTTP server to stream camera feed

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

#else
    ESP_LOGE("ESP-CAM", "Camera support is not available for this chip");
    return;
#endif
}
