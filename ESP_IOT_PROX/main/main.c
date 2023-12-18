#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "FreeRTOSConfig.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#define WIFI_SUCCESS BIT0
#define WIFI_FAILURE BIT1
#define MAX_FAILURES 10

#define MACHINE_1 GPIO_NUM_35
#define MACHINE_2 GPIO_NUM_32

int STATE = 0;

static EventGroupHandle_t wifi_event_group;

static int s_retry_num = 0;

static const char *WIFI_TAG = "WIFI_CONNECTION", *MQTT_TAG = "MQTT_CONNECTION";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(WIFI_TAG, "Connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAX_FAILURES)
        {
            ESP_LOGI(WIFI_TAG, "Retrying to connect AP...");
            esp_wifi_connect();
            s_retry_num++;
        }
        else
        {
            xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
        }
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "LOCAL IP -> " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "Succesfully Connected to MQTT Broker");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "Successfully Published");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "TOPIC => %s", event->topic);
        ESP_LOGI(MQTT_TAG, "MESSAGE => %s", event->data);
    default:
        break;
    }
}

void connect_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_handler_event_instance));

    esp_event_handler_instance_t ip_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, &ip_handler_event_instance));

    wifi_config_t wificfg = {
        .sta = {
            .ssid = "KARDEVA2.4",
            .password = "Messi@27",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wificfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_SUCCESS | WIFI_FAILURE, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_SUCCESS)
    {
        ESP_LOGI(WIFI_TAG, "Connected to ap");
    }
    else if (bits & WIFI_FAILURE)
    {
        ESP_LOGI(WIFI_TAG, "Failed to connect to ap");
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_handler_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);
}

void app_main(void)
{
    gpio_set_direction(MACHINE_1, GPIO_MODE_INPUT);
    gpio_set_direction(MACHINE_2, GPIO_MODE_INPUT);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(WIFI_TAG, "Wifi_Mode_Station");
    connect_wifi();

    ESP_LOGI(MQTT_TAG, "MQTT Started");
    const esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtt://192.168.0.118"};
    ESP_LOGI(MQTT_TAG, "Initializing MQTT Client");
    esp_mqtt_client_handle_t mqttClient = esp_mqtt_client_init(&cfg);
    ESP_LOGI(MQTT_TAG, "Registering MQTT Client event");
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqttClient, ESP_EVENT_ANY_ID, mqtt_event_handler, mqttClient));
    ESP_LOGI(MQTT_TAG, "Starting MQTT Client");
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqttClient));

    while (1)
    {
        if (gpio_get_level(MACHINE_1) == 0 && STATE == 0)
        {
            ESP_LOGI(MQTT_TAG, "Publishing Message");
            int _id = esp_mqtt_client_publish(mqttClient, "/machine/topic/1", "1", 0, 1, 0);
            ESP_LOGI(MQTT_TAG, "MSG ID = %d", _id);
            STATE = 1;
        }
        else if (gpio_get_level(MACHINE_1) == 1)
        {
            STATE = 0;
        }

        vTaskDelay(1);
    }
}