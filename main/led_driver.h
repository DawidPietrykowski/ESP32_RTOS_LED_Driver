#pragma once

#include "freertos/FreeRTOS.h"
#include "driver/rmt.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// WS2812B strip config
#define LED_GPIO 17
#define LED_NUM 60
#define RMT_REFRESH_RATE (120)
#if 0
#define WS2812_T0H_NS (350)
#define WS2812_T0L_NS (1000)
#define WS2812_T1H_NS (1000)
#define WS2812_T1L_NS (350)
//#define WS2812_RESET_US (280)
#define WS2812_RESET_US (25)
#else
#define WS2812_T0H_NS (400)
#define WS2812_T0L_NS (800)
#define WS2812_T1H_NS (850)
#define WS2812_T1L_NS (450)
#define WS2812_RESET_US (25)
#endif
#define ZERO_BUFFER_SIZE 48

// Animations
#define DEFAULT_ANIMATION   1
#define BRIGHTNESS_STEP     15
#define BASIC_COLOR_S_COUNT 10

// Sprinkling
#define GENERATE_RANDOM_FOR_SPRINKLE    false
#define SPRINKLE_UPDATE_INTERVAL        10
#define SPRINKLE_TRESHOLD               5
#define SPRINKLE_DIMMING_SPEED          1

// Police lights
#define POLICE_SPRINKLE_LIGHT_COUNT     ((LED_NUM/(POLICE_LIGHTS_PADDING*2+1))-1)
//#define POLICE_SPRINKLE_LIGHT_COUNT     1
#define POLICE_LIGHTS_PADDING           2
#define POLICE_SPRINKLE_UPDATE_INTERVAL 5

// Color stream
#define COLOR_SLIDE_IN_INTERVAL 2
#define CENTER_LED_ID (LED_NUM/2)

typedef struct{
    uint8_t g;
    uint8_t r;
    uint8_t b;
}pixel;

struct pol_light {
    pixel main_color;
    uint16_t pos;
    uint16_t step;
    uint16_t steps;
    uint8_t stall_counter;
};

typedef struct {
  char mode[10];
  int anim_refresh_rate;
  int rmt_refresh_rate;
  int brightness;
  int brightness_step;
  int default_animation;
  bool enable_touch_control;
  pixel selected_color;

} Config;

int mode_to_num(Config* config);

void update_animation_config();

void animation_task(void *pvParameters);
void led_task(void *pvParameters);
void animation_switcher_task(void *pvParameters);
void led_task_i2s(void *pvParameters);

void print_config();
float quadratic_transition(float val);
uint8_t next_animation();
uint8_t next_brightness();
uint32_t update_clk_rate();
void set_color(pixel* p, uint8_t r, uint8_t g, uint8_t b);
void set_brightness(uint8_t b);
void set_solid_color(pixel* p, uint8_t r, uint8_t g, uint8_t b);
void clear_strip(pixel *strip_ptr);
void apply_brightness(pixel *strip_ptr);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b);

void update_strip(rmt_config_t* config, const rmt_item32_t *rmt_items, pixel strip[], uint32_t led_num);
void encode_strip_i2s(pixel strip[], uint32_t led_num);

void pol_update(struct pol_light *p);
bool pol_free_spot(int pos);
void pol_reset(struct pol_light *p);
void pol_update(struct pol_light *p);
void pol_reset_step(struct pol_light *p);
void pol_init_light(struct pol_light *p);
