#pragma once

#include "esp_system.h"

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "Orange_Swiatlowod_6E9A"
#define EXAMPLE_ESP_WIFI_PASS      "natalka15"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
#define WIFI_MAXIMUM_RETRY 10
#define WIFI_SSID      "Orange_Swiatlowod_6E9A"
#define WIFI_PASS      "natalka15"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_init_sta(void);
void wifi_init_sta_new(void);
void http_server_init();