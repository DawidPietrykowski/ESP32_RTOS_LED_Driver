#include "led_driver.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/rmt.h"
#include "esp_system.h"
#include "esp_random.h"
#include <string.h>
//#include "driver/spi_master.h"
#include <string.h>
#include <utils.h>
#include "driver/i2s_std.h"

pixel strip[LED_NUM];
pixel strip_temp[LED_NUM];
uint8_t *strip_i2s_data;
static uint8_t zero_buffer[ZERO_BUFFER_SIZE];

SemaphoreHandle_t strip_data_semphr;

Config config;

uint8_t* i2s_data;
static i2s_chan_handle_t                tx_chan;        // I2S tx channel handler
static i2s_chan_handle_t                rx_chan;        // I2S rx channel handler


void led_task_rmt(void *pvParameters){
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_GPIO, RMT_CHANNEL_0);
    // set counter clock to 40MHz
    config.clk_div = 2;
    uint32_t clock_hz = 0;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    ESP_ERROR_CHECK(rmt_get_counter_clock(config.channel, &clock_hz));
    const rmt_item32_t rmt_items[LED_NUM*24+1];
    printf("rmt_clock_hz: %ld\n", clock_hz);

    if(strip_data_semphr == NULL)
        printf("led_task_rmt: strip_data_semphr: NULL\n");

    TickType_t current_ticks;
    TickType_t last_ticks = 0;
    TickType_t period_ticks = pdMS_TO_TICKS(1000/RMT_REFRESH_RATE);


    while (true) {
        current_ticks = xTaskGetTickCount();
        if(current_ticks - last_ticks >= period_ticks){
            update_strip(&config, rmt_items, strip, LED_NUM);
            last_ticks = current_ticks;
        }
    }
    vTaskDelete( NULL );
}

void write_i2s(){
    apply_brightness(strip);
    const uint8_t bit0 = 0b11000000;
    const uint8_t bit1 = 0b11111000;

    uint8_t *p = strip_i2s_data;

    // if(strip_data_semphr == NULL){
    //     printf("led_task_i2s: strip_data_semphr: NULL\n");
    //     return;
    // }

//    xSemaphoreTake(strip_data_semphr, portMAX_DELAY);
    for(int i = 0; i < LED_NUM; i++) {

        int loc = i * 24;
        for (int j = 0; j < 8; j++) {
            int shift = j + 4;
            if(j > 3) shift = j - 4;
            if ((strip_temp[i].g >> shift) & 1)
                strip_i2s_data[loc + j] = bit1;
            else
                strip_i2s_data[loc + j] = bit0;
        }
        for (int j = 0; j < 8; j++) {
            int shift = j + 4;
            if(j > 3) shift = j - 4;
            if ((strip_temp[i].r >> shift) & 1)
                strip_i2s_data[loc + j + 8] = bit1;
            else
                strip_i2s_data[loc + j + 8] = bit0;
        }
        for (int j = 0; j < 8; j++) {
            int shift = j + 4;
            if(j > 3) shift = j - 4;
            if ((strip_temp[i].b >> shift) & 1)
                strip_i2s_data[loc + j + 16] = bit1;
            else
                strip_i2s_data[loc + j + 16] = bit0;
        }
    }
    // xSemaphoreGive(strip_data_semphr);

    size_t w_bytes = 0;

    if (i2s_channel_write(tx_chan, strip_i2s_data, LED_NUM*24, &w_bytes, 1000) != ESP_OK) {
        printf("Write Task: i2s write failed\n");
    }
}


void led_task_i2s(void *pvParameters){
    strip_i2s_data = (uint8_t *)calloc(1, LED_NUM*24);

    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

    i2s_std_config_t tx_std_cfg = {
            .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(100000),\
            // NEED TO REVERSE First 4 bits and last 4 bits
            .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                    .mclk = I2S_GPIO_UNUSED,
                    .bclk = I2S_GPIO_UNUSED,
                    .ws   = I2S_GPIO_UNUSED,
                    .dout = LED_GPIO,
                    .din  = I2S_GPIO_UNUSED,
                    .invert_flags = {
                            .mclk_inv = false,
                            .bclk_inv = false,
                            .ws_inv   = false,
                    },
            },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));


    TickType_t current_ticks;
    TickType_t last_ticks = 0;
    TickType_t period_ticks = pdMS_TO_TICKS(1000/I2S_REFRESH_RATE);

    vTaskDelay(1);

    while (true) {
        current_ticks = xTaskGetTickCount();
        if(current_ticks - last_ticks >= period_ticks){
            ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
            write_i2s();
            ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
            last_ticks = current_ticks;
        }
        // vTaskSwitchContext();
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void update_strip(rmt_config_t* config, const rmt_item32_t *rmt_items, pixel strip[], uint32_t led_num){
    apply_brightness(strip);
    const rmt_item32_t bit0 = {{{WS2812_T0H_NS/25, 1, WS2812_T0L_NS/25, 0}}};
    const rmt_item32_t bit1 = {{{WS2812_T1H_NS/25, 1, WS2812_T1L_NS/25, 0}}};
    const rmt_item32_t end = {{{0, 1, 0, 0}}};
    rmt_item32_t* p = (rmt_item32_t*)rmt_items;
    if(strip_data_semphr == NULL){
        printf("strip_data_semphr: NULL\n");
        return;
    }
    xSemaphoreTake(strip_data_semphr, portMAX_DELAY);
    for(int i = 0; i < led_num*3; i++){
        for(int j = 0; j < 8; j++){
            if(*((uint8_t*)strip_temp+i) & (1 << (7-j)))
                p->val = bit1.val;
            else{
                p->val = bit0.val;
            }
            p++;
        }
    }
    xSemaphoreGive(strip_data_semphr);
    p->val = end.val;
    rmt_write_items(config->channel, rmt_items, led_num*24+1, true);
}

int mode_to_num(Config* config){
    if(strcmp(config->mode, "rainbow") == 0)
        return 1;
    else if(strcmp(config->mode, "static") == 0)
        return 4;
    else if(strcmp(config->mode, "chase") == 0)
        return 2;
    else if(strcmp(config->mode, "police") == 0)
        return 0;
    else if(strcmp(config->mode, "received") == 0)
        return -1;
    else if(strcmp(config->mode, "off") == 0)
        return 5;
    else if(strcmp(config->mode, "breathing") == 0)
        return 6;
    else if(strcmp(config->mode, "sparkles") == 0)
        return 7;
    else
        return 3;
}

void print_config(){
    printf("Config:\n");
    printf("mode: %s\n", config.mode);
    printf("brightness: %d\n", config.brightness);
    printf("selected_color: %d %d %d\n\n", config.selected_color.r, config.selected_color.g, config.selected_color.b);
}
