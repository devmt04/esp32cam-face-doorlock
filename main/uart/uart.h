#ifndef MAIN_UART_UART_H_
#define MAIN_UART_UART_H_

#include "esp_err.h"
#include "esp_timer.h"

esp_err_t enable_uart();
void start_streaming_via_uart(void *pvParameters);
void timer13_callback(void *arg);
void timer14_callback(void *arg);

extern esp_timer_handle_t timer13;
extern esp_timer_handle_t timer14;
#endif 