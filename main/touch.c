#include "touch.h"
#include "animations.h"

#include "freertos/FreeRTOS.h"
#include "driver/touch_pad.h"

static uint16_t s_pad_init_val0;
static uint16_t s_pad_init_val2; 
static uint16_t s_pad_init_val3; 

static bool leds_on = true;

void touch_task(void *pvParameters){
    ESP_ERROR_CHECK(touch_pad_init());
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(0, TOUCH_THRESH_NO_USE);
    touch_pad_config(2, TOUCH_THRESH_NO_USE);
    touch_pad_config(3, TOUCH_THRESH_NO_USE);
    
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    uint16_t touch_value;
    touch_pad_read_filtered(0, &touch_value);
    s_pad_init_val0 = touch_value;
    ESP_ERROR_CHECK(touch_pad_set_thresh(0, touch_value * 2 / 3));
    printf("T: %d; init val: %d\n", 0, s_pad_init_val0);

    vTaskDelay(TOUCHPAD_FILTER_TOUCH_PERIOD / portTICK_PERIOD_MS);
    touch_pad_read_filtered(2, &touch_value);
    s_pad_init_val2 = touch_value;
    ESP_ERROR_CHECK(touch_pad_set_thresh(2, touch_value * 2 / 3));
    printf("T: %d; init val: %d\n", 2, s_pad_init_val3);
    
    vTaskDelay(TOUCHPAD_FILTER_TOUCH_PERIOD / portTICK_PERIOD_MS);
    touch_pad_read_filtered(3, &touch_value);
    s_pad_init_val3 = touch_value;
    ESP_ERROR_CHECK(touch_pad_set_thresh(3, touch_value * 2 / 3));
    printf("T: %d; init val: %d\n", 3, s_pad_init_val3);

    while(1){
        uint16_t value = 0;
        touch_pad_clear_status();
        touch_pad_read_filtered(0, &value);
        if (value < s_pad_init_val0 * TOUCH_THRESH_PERCENT / 100) {
            printf("T%d activated!\n", 0);
            printf("value: %d; init val: %d\n", value, s_pad_init_val0);
            uint8_t a = next_animation();
            printf("new animation: %d\n", a);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        //vTaskDelay(5 / portTICK_PERIOD_MS);

        touch_pad_clear_status();
        touch_pad_read_filtered(2, &value);
        if (value < s_pad_init_val2 * TOUCH_THRESH_PERCENT / 100) {
            printf("T%d activated!\n", 2);
            printf("value: %d; init val: %d\n", value, s_pad_init_val2);
            if(leds_on)
                set_brightness(0);
            else
                set_brightness(255);
            printf("led %s\n", (leds_on ? "off" : "on"));
            leds_on = !leds_on;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        //vTaskDelay(5 / portTICK_PERIOD_MS);

        touch_pad_clear_status();
        touch_pad_read_filtered(3, &value);
        if (value < s_pad_init_val3 * TOUCH_THRESH_PERCENT / 100) {
            printf("T%d activated!\n", 3);
            printf("value: %d; init val: %d\n", value, s_pad_init_val3);
            uint8_t b = next_brightness();
            //uint32_t c = update_clk_rate();
            printf("new brightness: %d\n", b);
            //printf("new clk: %d\n", c);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        //vTaskDelay(5 / portTICK_PERIOD_MS);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

