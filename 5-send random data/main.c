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

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "mqtt_app.h"

/* Set the SSID and Password via project configuration, or can set directly here */
#define DEFAULT_SSID "FREELAB"
#define DEFAULT_PWD "HgaloengKbnKcg22"

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

// MQTT and Payload
const char mqtt_url[] = "mqtt://mqtt.iotera.io";
const char mqtt_username[] = "mqtt_1000000181_eaaeb0b6-d102-4180-8e5d-9ea2f1f78501";
const char mqtt_password[] = "rzkc0ex70w46x70l";
uint32_t mqtt_port = 1883;
char payload[512];
const char ID[] = "0001";
uint32_t pulse1 = 0;
uint32_t pulse2 = 0;
float battery;

/*
 *  Application for Regular mode
 */
void init_regular_mode(void)
{
	printf("standard app init device\r\n");

	// init mqtt
	mqtt_init(mqtt_url,mqtt_port,mqtt_username,mqtt_password);

	vTaskDelay(pdMS_TO_TICKS(100));
}

void pack_data(void)
{
	bzero(payload,512);

	// Pack data
	sprintf(payload,"{\"payload\":["
	"{\"sensor\":\"wifi_node\",\"param\":\"pulse_counter\",\"value\":{"
	"\"ID\":\"%s\",\"CH1\":%i,\"CH2\":%i}},"

	"{\"sensor\":\"wifi_node\",\"param\":\"battery\",\"value\":%f}]}",

	ID,pulse1,pulse2,
	battery);
}

void get_sensor_data_regular_mode(void)
{
	// get random battery value 
    battery = ((float) esp_random() / (float) UINT32_MAX) * 5.0;

    // get pulse counter value
	// pulse1 = pulse_ch1_count();
	// pulse2 = pulse_ch2_count();
    pack_data();
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void send_sensor_data_regular_mode(void)
{
	int mqtt_stat = -1;
	int mqtt_pub_stat = -1;
	// check wifi
	if (wifi_global_stat != 0)
	{
		printf("wifi connected\r\n");
		mqtt_stat = mqtt_conn_stat(); // check mqtt connection first
		if (mqtt_stat == ESP_OK){
			printf("connected to mqtt server\r\n");
			mqtt_pub_stat = mqtt_publish_iotera(payload); // try to publish to iotera server
			if ((mqtt_pub_stat != 0)||(mqtt_pub_stat != -1)){
				printf("publish succeed:%d\r\n",mqtt_pub_stat);
			}
			else{
				printf("publish failed:%d\r\n",mqtt_pub_stat);
			}
		}
		else{
			printf("disconnected to mqtt server\r\n");
		}

	}
	else {
		printf("wifi disconnected\r\n");
	}
}

/*
 * regular mode:
 * sampling data: every 1 minute
 * direct sending data every sampling
 * not using deepsleep
 *
 */
void regular_mode (void *arg)
{
	init_regular_mode();

	while(1)
	{
		get_sensor_data_regular_mode();
		send_sensor_data_regular_mode();
		vTaskDelay(pdMS_TO_TICKS(1000));
		printf("regular mode, adaptor OK\r\n");

		vTaskDelay(pdMS_TO_TICKS(60000));
	}
}

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

    // Start sending to Iotera Platform
    xTaskCreate(regular_mode,"Iotera regular task",4096,NULL,2,NULL);
	vTaskDelay(pdMS_TO_TICKS(1000));

	// main loop
    while(1)
    {
    	vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
