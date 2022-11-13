#include "touch.h"
#include "animations.h"
#include "led_driver.h"

#include "freertos/FreeRTOS.h"
#include "driver/touch_pad.h"

static uint16_t s_pad_init_val[4];
void (*functions[4]) ();
static bool leds_on = true;
extern Config config;

void init_pad(int n){
    vTaskDelay(TOUCHPAD_FILTER_TOUCH_PERIOD / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(touch_pad_read_filtered(n, &s_pad_init_val[n]));
    printf("T: %d; init val: %d\n", n, s_pad_init_val[n]);
}

void initialize_touch_sensors(){
    ESP_ERROR_CHECK(touch_pad_init());
    ESP_ERROR_CHECK(touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V));
    
    ESP_ERROR_CHECK(touch_pad_config(0, TOUCH_THRESH_NO_USE));
    ESP_ERROR_CHECK(touch_pad_config(2, TOUCH_THRESH_NO_USE));
    ESP_ERROR_CHECK(touch_pad_config(3, TOUCH_THRESH_NO_USE));
    
    ESP_ERROR_CHECK(touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD));

    init_pad(0);
    init_pad(2);
    init_pad(3);
}

void switch_animation(){
    uint8_t a = next_animation();
    printf("new animation: %d\n", a);
}

void switch_on_state(){
    if(leds_on)
        set_brightness(0);
    else
        set_brightness(255);
    printf("led %s\n", (leds_on ? "off" : "on"));
    leds_on = !leds_on;
}


void change_brightness(){
    uint8_t b = next_brightness();
    printf("new brightness: %d\n", b);
}

void update_pad(int n){
            uint16_t value = 0;
        touch_pad_clear_status();
        touch_pad_read_filtered(n, &value);
        if (value < s_pad_init_val[n] * TOUCH_THRESH_PERCENT / 100) {
            printf("T%d activated!\n", n);
            printf("value: %d; init val: %d\n", value, s_pad_init_val[n]);
            functions[n]();
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

}

void touch_task(void *pvParameters){
    functions[0] = switch_animation;
    functions[2] = switch_on_state;
    functions[3] = change_brightness;

    initialize_touch_sensors();

    while(true){
        if(config.enable_touch_control){
            update_pad(0);
            update_pad(2);
            update_pad(3);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

