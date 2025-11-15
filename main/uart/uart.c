#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "esp_camera.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "uart.h"

#define CHUNK_SIZE 4096
#define BUF_SIZE 256

esp_timer_handle_t timer13 = NULL;

void timer13_callback(void *arg) {
    gpio_set_level(13, 0);
}

static void uart_task(void *pvParameters) {
    uint8_t data[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE - 1, 10 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = 0; // Null terminate
            if (strstr((char*)data, "face_detected")) {
                gpio_set_level(13, 1);
                // Start one-shot timer: 3 seconds = 3,000,000 us
                esp_timer_start_once(timer13, 3000000);
            }else if (strstr((char*)data, "unauth")) {
                // gpio_set_level(16, 1);
                // esp_timer_start_once(timer14, 5000000);
            }else{
                // printf("UNKOWN CMD RECIEVED");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


esp_err_t enable_uart(){
    uart_config_t uart_config = {
        .baud_rate = 115200, //921600,     
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    esp_err_t res = uart_driver_install(UART_NUM_0, BUF_SIZE, 0, 0, NULL, 0);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
    return res;
}

static void send_data(uint8_t* buffer, size_t len) {
    // Send raw 16-bit PCM as bytes
    uart_write_bytes(UART_NUM_0, (const char*)buffer, len);
}



void start_streaming_via_uart(void *pvParameters) {
    while (1) {
        camera_fb_t *pic = esp_camera_fb_get();
        if (pic) {
            uint32_t frame_len = pic->len;

            send_data((uint8_t *)&frame_len, sizeof(frame_len));

            for (size_t i = 0; i < frame_len; i += CHUNK_SIZE) {
                size_t chunk_size = (frame_len - i < CHUNK_SIZE) ? (frame_len - i) : CHUNK_SIZE;
                send_data(pic->buf + i, chunk_size);
                // Small delay to avoid UART congestion
                vTaskDelay(1);
            }

            esp_camera_fb_return(pic);
        } else {
            printf("Camera capture failed");
        }

        // Adjust frame rate (100 ms ≈ 10 fps max)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}