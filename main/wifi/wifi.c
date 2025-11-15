#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include <string.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_log.h"


void init_nvs(){
    // init NVS (Non-Volatile Storage), to read/write system configuration data
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void init_netif(){
    // init network interface, prepares the TCP/IP stack and internal network interfaces. Must be called before any Wi-Fi or Ethernet functions.
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    ESP_LOGI("NETIF", "ESP32 IP: " IPSTR, IP2STR(&ip_info.ip));
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI("WIFI","station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI("WIFI", "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

esp_err_t init_wifi(){
    init_nvs(); // init NVS (Non-Volatile Storage)
    init_netif(); // init network interface
   
    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                    ESP_EVENT_ANY_ID,
                                    &wifi_event_handler,
                                    NULL,
                                    NULL));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASSWORD,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .gtk_rekey_interval = 600,
            .pmf_cfg = {
                    .required = true,
            },
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}


