#include "touch.h"
#include "animations.h"

#include "freertos/FreeRTOS.h"
#include "driver/touch_pad.h"

static uint16_t s_pad_init_val[4];
void (*functions[4]) ();
static bool leds_on = true;

void init_pad(int n){
    touch_pad_config(n, TOUCH_THRESH_NO_USE);
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    uint16_t touch_value;
    touch_pad_read_filtered(n, &touch_value);
    s_pad_init_val[n] = touch_value;
    ESP_ERROR_CHECK(touch_pad_set_thresh(n, touch_value * 2 / 3));
    printf("T: %d; init val: %d\n", n, s_pad_init_val[n]);
}

void initialize_touch_sensors(){
    ESP_ERROR_CHECK(touch_pad_init());
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    
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
    functions[0] = next_animation;
    functions[2] = switch_on_state;
    functions[3] = change_brightness;

    initialize_touch_sensors();

    while(true){
        update_pad(0);
        update_pad(2);
        update_pad(3);

        // uint16_t value = 0;
        // touch_pad_clear_status();
        // touch_pad_read_filtered(0, &value);
        // if (value < s_pad_init_val0 * TOUCH_THRESH_PERCENT / 100) {
        //     printf("T%d activated!\n", 0);
        //     printf("value: %d; init val: %d\n", value, s_pad_init_val0);
        //     uint8_t a = next_animation();
        //     printf("new animation: %d\n", a);
        //     vTaskDelay(200 / portTICK_PERIOD_MS);
        // }
        // //vTaskDelay(5 / portTICK_PERIOD_MS);

        // touch_pad_clear_status();
        // touch_pad_read_filtered(2, &value);
        // if (value < s_pad_init_val2 * TOUCH_THRESH_PERCENT / 100) {
        //     printf("T%d activated!\n", 2);
        //     printf("value: %d; init val: %d\n", value, s_pad_init_val2);
        //     if(leds_on)
        //         set_brightness(0);
        //     else
        //         set_brightness(255);
        //     printf("led %s\n", (leds_on ? "off" : "on"));
        //     leds_on = !leds_on;
        //     vTaskDelay(200 / portTICK_PERIOD_MS);
        // }
        // //vTaskDelay(5 / portTICK_PERIOD_MS);

        // touch_pad_clear_status();
        // touch_pad_read_filtered(3, &value);
        // if (value < s_pad_init_val3 * TOUCH_THRESH_PERCENT / 100) {
        //     printf("T%d activated!\n", 3);
        //     printf("value: %d; init val: %d\n", value, s_pad_init_val3);
        //     uint8_t b = next_brightness();
        //     //uint32_t c = update_clk_rate();
        //     printf("new brightness: %d\n", b);
        //     //printf("new clk: %d\n", c);
        //     vTaskDelay(200 / portTICK_PERIOD_MS);
        // }
        //vTaskDelay(5 / portTICK_PERIOD_MS);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

