#pragma once

#include "freertos/FreeRTOS.h"
#include "driver/rmt.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "animations.h"

// #define USING_RMT

// WS2812B strip config
#define LED_GPIO 17
#define LED_NUM 60
#define RMT_REFRESH_RATE (120)
#define I2S_REFRESH_RATE (60)
#define WS2812_T0H_NS (400)
#define WS2812_T0L_NS (800)
#define WS2812_T1H_NS (850)
#define WS2812_T1L_NS (450)
#define WS2812_RESET_US (25)
#define ZERO_BUFFER_SIZE 48

struct config_t {
  int mode;
  int anim_refresh_rate;
  int refresh_rate;
  int brightness;
  int brightness_step;
  int default_animation;
  int anim_speed;
  bool enable_touch_control;
  pixel selected_color;
};

typedef struct config_t Config;

int mode_to_num(char* mode);
char* num_to_mode(int mode);

void led_task_rmt(void *pvParameters);
void led_task_i2s(void *pvParameters);

uint32_t update_clk_rate();
void print_config();

void update_strip(rmt_config_t* config, const rmt_item32_t *rmt_items, pixel strip[], uint32_t led_num);
void encode_strip_i2s(pixel strip[], uint32_t led_num);