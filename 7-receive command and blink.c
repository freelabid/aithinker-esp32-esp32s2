/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
    This example shows how to use the All Channel Scan or Fast Scan to connect
    to a Wi-Fi network.

    In the Fast Scan mode, the scan will stop as soon as the first network matching
    the SSID is found. In this mode, an application can set threshold for the
    authentication mode and the Signal strength. Networks that do not meet the
    threshold requirements will be ignored.

    In the All Channel Scan mode, the scan will end only after all the channels
    are scanned, and connection will start with the best network. The networks
    can be sorted based on Authentication Mode or Signal Strength. The priority
    for the Authentication mode is:  WPA2 > WPA > WEP > Open
*/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "driver/gpio.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

/* Set the SSID and Password via project configuration, or can set directly here */
#define DEFAULT_SSID "SSID"
#define DEFAULT_PWD "PASSWORD"

#if CONFIG_EXAMPLE_WIFI_ALL_CHANNEL_SCAN
#define DEFAULT_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#elif CONFIG_EXAMPLE_WIFI_FAST_SCAN
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#else
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#endif /*CONFIG_EXAMPLE_SCAN_METHOD*/

#if CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SECURITY
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#else
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#endif /*CONFIG_EXAMPLE_SORT_METHOD*/

#if CONFIG_EXAMPLE_FAST_SCAN_THRESHOLD
#define DEFAULT_RSSI CONFIG_EXAMPLE_FAST_SCAN_MINIMUM_SIGNAL
#if CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_OPEN
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#elif CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_WEP
#define DEFAULT_AUTHMODE WIFI_AUTH_WEP
#elif CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_WPA
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA_PSK
#elif CONFIG_EXAMPLE_FAST_SCAN_WEAKEST_AUTHMODE_WPA2
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK
#else
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#endif
#else
#define DEFAULT_RSSI -127
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#endif /*CONFIG_EXAMPLE_FAST_SCAN_THRESHOLD*/

static const char *TAG = "iotera";
uint8_t wifi_global_stat;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        wifi_global_stat = 1;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_global_stat = 0;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_global_stat = 2;
    }
}


/* Initialize Wi-Fi as sta and set scan method */
static void fast_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize and start WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
            .scan_method = DEFAULT_SCAN_METHOD,
            .sort_method = DEFAULT_SORT_METHOD,
            .threshold.rssi = DEFAULT_RSSI,
            .threshold.authmode = DEFAULT_AUTHMODE,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// START OF MQTT & SENDING DATA
const char mqtt_url[] = "mqtt://mqtt.iotera.io";
const char mqtt_username[] = ;
const char mqtt_password[] = ;
uint32_t mqtt_port = 1883;

uint8_t turnon = 0;

int mqtt_global_stat;
void mqtt_init(const char* mqtt_server, int mqtt_port, const char* username, const char* pass);
int mqtt_publish_iotera(char* payload);
int mqtt_subscribe_iotera(void);
void mqtt_data_handling(esp_mqtt_event_handle_t event);
void generate_topic(const char* username);

esp_mqtt_client_handle_t client;
char confirm_payload[512];	
char mqtt_online_topic[256];
char mqtt_lastwill_topic[256];
char mqtt_topic[256];
char mqtt_confirm_topic[256];
int msg_id;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, mqtt_online_topic, NULL, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            mqtt_global_stat = 1;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_global_stat = 2;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, mqtt_topic, "Device subscribed", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            mqtt_global_stat = 3;
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            mqtt_global_stat = 4;
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            mqtt_global_stat = 5;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            mqtt_data_handling(event);
            mqtt_global_stat = 6;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            mqtt_global_stat = 0;
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }

    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}


void mqtt_init(const char* mqtt_server, int mqtt_port, const char* username, const char* pass)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    generate_topic(username);// generate topic for iotera platform

    esp_mqtt_client_config_t mqtt_cfg = {
    		.uri = mqtt_server,
    		.port = mqtt_port,
    		.username = username,
    		.password = pass,
            .lwt_topic = mqtt_lastwill_topic
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

/*
 *
 * APPLICATION FOR MQTT
 *
 *
 */
int mqtt_publish_iotera(char* payload)
{
	int pub_stat = 0;
	if ((mqtt_global_stat != 0) && (mqtt_global_stat != 2)){
		// publish payload
		pub_stat = esp_mqtt_client_publish(client,mqtt_confirm_topic,payload,0, 1, 0);
	}

	return pub_stat;
}

int mqtt_subscribe_iotera()
{
    int subs_stat = 0;
	if ((mqtt_global_stat != 0) && (mqtt_global_stat != 2)){
		// subscribe
		subs_stat = esp_mqtt_client_subscribe(client,mqtt_topic, 0);
	}

	return subs_stat;
}

void mqtt_data_handling(esp_mqtt_event_handle_t data_event)
{
    printf("TOPIC=%.*s\r\n", data_event->topic_len, data_event->topic);
    printf("DATA=%.*s\r\n", data_event->data_len, data_event->data);

    cJSON *root = cJSON_Parse(data_event->data);
    if (cJSON_IsObject(root)) {
        int n = cJSON_GetArraySize(root);
        printf("Num JSON element: %d\n", n);
        char *id;
        char *param;
        uint8_t value;
        id = cJSON_GetStringValue(cJSON_GetObjectItem(root, "id"));
        param = cJSON_GetStringValue(cJSON_GetObjectItem(root, "param"));
        value = cJSON_GetObjectItem(root, "value")->valueint;
        printf("%s %s %d\n", id, param, value);

        if (strcmp(param, "turnon") == 0) {
            turnon = value;
            bzero(confirm_payload,512);
            // Pack data
            sprintf(confirm_payload,"{\"result\":0,\"id\":\"%s\",\"payload\":["
            "{\"sensor\":\"led_onboard\",\"param\":\"ledstat\",\"configtype\":\"data\",\"value\":%d}]}",
            id, turnon);

            mqtt_publish_iotera(confirm_payload);
        }
    }
}

void generate_topic(const char* username)
{
	int len = 0;
	int counter = 0;
	int i = 0;
	char tmp1[64];
	char tmp2[128];

	bzero(tmp1,sizeof(tmp1));
	bzero(tmp2,sizeof(tmp2));

	len = strlen(username);
	printf("username len = %d\r\n",len);

	while(*username != '_'){
		username++;
		counter++;
	}
	username++;
	counter++;
	while(*username != '_'){
		tmp1[i] = *username;
		username++;
		counter++;
		i++;
	}
	//printf("data tmp1: %s\r\n",tmp1);
	username++;
	counter++;
	i = 0;
	while(counter<len){
		tmp2[i] = *username;
		username++;
		counter++;
		i++;
	}
	//printf("data tmp2: %s\r\n",tmp2);

	sprintf(mqtt_topic,"iotera/sub/%s/%s/command",tmp1,tmp2);
    sprintf(mqtt_confirm_topic,"iotera/pub/%s/%s/command_result",tmp1,tmp2);
    sprintf(mqtt_online_topic,"iotera/pub/%s/%s/online",tmp1,tmp2);
    sprintf(mqtt_lastwill_topic,"iotera/pub/%s/%s/offline",tmp1,tmp2);

	printf("%s\r\n",mqtt_topic);
	printf("%s\r\n",mqtt_confirm_topic);
	printf("%s\r\n",mqtt_online_topic);
	printf("%s\r\n",mqtt_lastwill_topic);
}
// END OF MQTT & SENDING DATA

// START OF BLINK GPIO

#define BLINK_GPIO 2

static uint8_t s_led_state = 0;

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

// END OF BLINK GPIO

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    fast_scan();

    // Start setting onboard LED GPIO Blink
    configure_led();

	// init mqtt
	mqtt_init(mqtt_url,mqtt_port,mqtt_username,mqtt_password);

	// main loop
    while(1)
    {
        if (turnon == 1) {
            ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
            blink_led();
            /* Toggle the LED state */
            s_led_state = !s_led_state;            
        } else {
            gpio_set_level(BLINK_GPIO, 0);
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
