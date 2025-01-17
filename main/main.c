#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"  
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
//#include "driver/rmt.h"
#include "driver/touch_pad.h"

#include "led_driver.h"
#include "touch.h"
#include "wifi.h"
#include "animations.h"

void app_main(void)
{
    xTaskCreatePinnedToCore(led_task_i2s, "led_task_i2s", 16384 * 6, (void *)1, configMAX_PRIORITIES - 1, NULL, 1);

    xTaskCreatePinnedToCore(animation_task, "animation_task", 16384, (void *)1, configMAX_PRIORITIES - 3, NULL, 1);
    
    xTaskCreatePinnedToCore(touch_task, "touch_task", 2048, (void *)1, 1, NULL, 0);

    xTaskCreatePinnedToCore(wifi_init_sta_new, "wifi_init_sta_task", 16384, (void *)1, 1, NULL, 0);
}
