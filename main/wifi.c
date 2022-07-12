#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "freertos/event_groups.h"  
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "wifi.h"
#include "cJSON.h"

#include "led_driver.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
static const char *TAG = "wifi station";

static int s_retry_num = 0;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t init_spiffs(){
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 8,
        .format_if_mount_failed = false
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    } else {
        printf("Partition size: total: %d, used: %d\n", total, used);
    }
    return ESP_OK;
}

void wifi_init_sta(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
            
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
    vTaskDelete( NULL );
}


static EventGroupHandle_t s_wifi_event_group_new;

static int s_retries = 0;
void wifi_events(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    }
    else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED){
        if(s_retries < EXAMPLE_ESP_MAXIMUM_RETRY){
            esp_wifi_connect();
            s_retries++;
            printf("connection retry %d to AP\n", s_retries+1);
        }else{
            xEventGroupSetBits(s_wifi_event_group_new, WIFI_FAIL_BIT);
        }
    }
    else if(base == IP_EVENT && id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        printf("IP received: %d.%d.%d.%d\n", IP2STR(&event->ip_info.ip));
        s_retries=0;
        xEventGroupSetBits(s_wifi_event_group_new, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta_new(void)
{
    ESP_ERROR_CHECK(init_spiffs());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group_new = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        }
    };
    esp_event_handler_instance_t wifi_event_handler;
    esp_event_handler_instance_t ip_event_handler;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_events,
        NULL,
        &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_events,
        NULL,
        &ip_event_handler));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());


    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group_new, 
    WIFI_FAIL_BIT | WIFI_CONNECTED_BIT,
    pdFALSE,
    pdFALSE,
    portMAX_DELAY);

    if(bits == WIFI_CONNECTED_BIT){
        printf("Wifi connected\n");
        http_server_init();
    }
    else if(bits == WIFI_FAIL_BIT){
        printf("Wifi connection fail\n");
    }
    else{
        printf("Unexpected event\n");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        ip_event_handler));
    vEventGroupDelete(s_wifi_event_group_new);
    vTaskDelete( NULL );
}
static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    //return dest + base_pathlen;
    return dest;
}

esp_err_t get_handler(httpd_req_t *req){
    printf("GET Request received: \n%s\n", req->uri);
    #define HTTP_CHUNK_BUFSIZE 8192/2
    #define FILE_PATH_MAX 128
    char buffer[HTTP_CHUNK_BUFSIZE];

    const char *base_path = "/spiffs";
    char filepath[FILE_PATH_MAX];
    const char *filename = get_path_from_uri(filepath, base_path, req->uri, sizeof(filepath));
    const char *spiffs_empty = "/spiffs/";
    //printf("Filename: \n%s\n slen1: %d slen2: %d\n", filename, strlen(filename), strlen(spiffs_empty));
    FILE * fd;
    if(!strcmp(spiffs_empty, filename))
        fd = fopen ("/spiffs/index.html", "r");
    else
        fd = fopen (filename, "r");

    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filename);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }


    const char *fileextension = filename+strlen(filename)-3;
    ESP_LOGI(TAG, "File extension : %s", fileextension);
    if(strcmp(fileextension, "css")==0){
        ESP_LOGI(TAG, "File is css");
        httpd_resp_set_type(req, "text/css");
    }
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    char *chunk = buffer;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, HTTP_CHUNK_BUFSIZE, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;

    return ESP_OK;
}

uint8_t byte_from_hex(char l, char r){
    uint8_t numbers[2] = {r, l};
    uint8_t sum = 0;
    for(int i = 1; i >= 0; i--){
        if(numbers[i] >= 48 && numbers[i] <= 57){
            sum += (numbers[i] - 48)*(i == 1 ? 16 : 1);
        }else if(numbers[i] >= 97 && numbers[i] <= 102){
            sum += (numbers[i] - 97 + 10)*(i == 1 ? 16 : 1);
        }
    }
    return sum;
}

esp_err_t post_handler(httpd_req_t *req){
    printf("POST Request received: \n%s\n", req->uri);



    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[512];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content)-1);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    content[sizeof(content)-1] = '\0';


    extern Config config;
	cJSON *root = cJSON_Parse(content);
	if (cJSON_GetObjectItem(root, "mode")) {
		char *mode = cJSON_GetObjectItem(root,"mode")->valuestring;
		ESP_LOGI(TAG, "mode=%s",mode);
        strcpy(config.mode, mode);
	}
	if (cJSON_GetObjectItem(root, "brightness")) {
		int brightness = cJSON_GetObjectItem(root,"brightness")->valueint;
		ESP_LOGI(TAG, "brightness=%d",brightness);
        config.brightness = brightness;
	}
    if (cJSON_GetObjectItem(root, "selected_color")){
        char* data = cJSON_GetObjectItem(root, "selected_color")->valuestring;
		ESP_LOGI(TAG, "selected_color=%s",data);
        
        int char_length = strlen(data);
        if(char_length == 6){
            pixel receivedPixel;
            receivedPixel.r = byte_from_hex(data[0], data[1]);
            receivedPixel.g = byte_from_hex(data[2], data[3]);
            receivedPixel.b = byte_from_hex(data[4], data[5]);
            config.selected_color = receivedPixel;
        }
    }
    if (cJSON_GetObjectItem(root, "rgb_data")){
        char* data = cJSON_GetObjectItem(root, "rgb_data")->valuestring;
        extern pixel strip[LED_NUM];
        int char_length = strlen(data);
        int byte_length = char_length/2;
        int pxl_length = char_length/6;
        for(int i = 0; i < pxl_length; i++){
            strip[i].g = byte_from_hex(data[i*6+0], data[i*6+1]);
            strip[i].b = byte_from_hex(data[i*6+2], data[i*6+3]);
            strip[i].r = byte_from_hex(data[i*6+4], data[i*6+5]);
        }
        //memcpy((void*)strip, data, strlen(data));
    }
	cJSON_Delete(root);

    print_config();

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}
httpd_uri_t get_uri = {
    .uri      = "/*",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};

httpd_uri_t post_uri = {
    .uri      = "/",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL
};

void http_server_init(){
    // Generate default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size=20480;
    // Empty handle to http_server
    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    // Register URI handlers
    httpd_register_uri_handler(server, &post_uri);
    httpd_register_uri_handler(server, &get_uri);
}