#pragma once

#include "freertos/FreeRTOS.h"
#include "driver/rmt.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


// Animations
#define DEFAULT_ANIMATION   4
#define BRIGHTNESS_STEP     15
#define BASIC_COLOR_S_COUNT 10

// Sprinkling
#define GENERATE_RANDOM_FOR_SPRINKLE    false
#define SPRINKLE_UPDATE_INTERVAL        10
#define SPRINKLE_TRESHOLD               5
#define SPRINKLE_DIMMING_SPEED          1

// Police lights
#define POLICE_SPRINKLE_LIGHT_COUNT     ((LED_NUM/(POLICE_LIGHTS_PADDING*2+1))-1)
// #define POLICE_SPRINKLE_LIGHT_COUNT     1
#define POLICE_LIGHTS_PADDING           2
#define POLICE_SPRINKLE_UPDATE_INTERVAL 5

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

void animation_task(void *pvParameters);

void update_animation_config();
void animation_switcher_task(void *pvParameters);
float quadratic_transition(float val);
uint8_t next_animation();
uint8_t next_brightness();

void set_color(pixel* p, uint8_t r, uint8_t g, uint8_t b);
void set_brightness(uint8_t b);
void set_solid_color(pixel* p, uint8_t r, uint8_t g, uint8_t b);
void clear_strip(pixel *strip_ptr);
void apply_brightness(pixel *strip_ptr);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b);


void pol_update(struct pol_light *p);
bool pol_free_spot(int pos);
void pol_reset(struct pol_light *p);
void pol_update(struct pol_light *p);
void pol_reset_step(struct pol_light *p);
void pol_init_light(struct pol_light *p);
