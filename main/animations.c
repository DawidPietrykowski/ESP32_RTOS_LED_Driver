#include "animations.h"
#include "led_driver.h"
#include <utils.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_random.h"

#include <sys/types.h>

static bool light_pos_taken[LED_NUM];
static pixel basic_colors_s[BASIC_COLOR_S_COUNT];
static const uint8_t color_count_s = BASIC_COLOR_S_COUNT;
static pixel basic_colors_police_s[4];

static const uint8_t color_count_police_s = 4;
static uint8_t anim_selected = DEFAULT_ANIMATION;
static uint8_t brightness=255;
static uint8_t step = 4;

static float animation_clock = 0;

extern Config config;
extern pixel strip[LED_NUM];
extern pixel strip_temp[LED_NUM];
extern SemaphoreHandle_t strip_data_semphr;

uint8_t next_animation(){
    anim_selected+=1;
    anim_selected%=7;
    return anim_selected;
}
uint8_t next_brightness(){
    step++;
    step%=5;
    uint8_t b = step == 4 ? 255 : step*64;
    set_brightness(b);
    return b;
}

void update_animation_config(){
    anim_selected = mode_to_num(&config);
    set_brightness((uint8_t)config.brightness);
}

void animation_switcher_task(void *pvParameters){
    uint8_t dir = -1;
    while(true){
        if(brightness==0)
            dir = 1;
        else if(brightness==255)
            dir = -1;
        set_brightness(brightness+dir);
        vTaskDelay(pdMS_TO_TICKS(40));
    }
    vTaskDelete( NULL );
}


void set_color(pixel* p, uint8_t r, uint8_t g, uint8_t b){ 
    p->r = r;
    p->g = g;
    p->b = b;
}
void set_brightness(uint8_t b){
    brightness = b;
}
void set_solid_color(pixel* p, uint8_t r, uint8_t g, uint8_t b){
    uint8_t arr[3] = {g,r,b};
    for(int i = 0; i < LED_NUM; i++){
        memcpy(p+i, arr, 3);
    }
}
void clear_strip(pixel *strip_ptr){
    memset(strip_ptr, 0, LED_NUM * sizeof(pixel));
}

void apply_brightness(pixel *strip_ptr){
    uint8_t *p_src = (uint8_t*)strip_ptr;
    uint8_t *p_dest = (uint8_t*)strip_temp;
    for(int i = 0; i < LED_NUM*3; i++){
        *p_dest = (uint8_t)(((float)(*p_src) * brightness)/255.0f);
        p_src++;
        p_dest++;
    }
}

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}


bool pol_free_spot(int pos){
    if(light_pos_taken[pos])
        return false;

    for(int i = -POLICE_LIGHTS_PADDING; i <= POLICE_LIGHTS_PADDING; i++){
        if((pos+i>=0)&&(pos+i<=LED_NUM)){
            if(light_pos_taken[pos+i])
                return false;
        }
    }
    return true;
}

void pol_reset(struct pol_light *p) {
    set_color(&strip[p->pos], 0, 0, 0);
    light_pos_taken[p->pos] = false;
    do{
        p->stall_counter++;
        p->pos = esp_random()%(LED_NUM);
    }while((!pol_free_spot(p->pos))&&(p->stall_counter<100));
    if(p->stall_counter>=100)
        printf("pol_light stall\n");
    p->stall_counter = 0;
    light_pos_taken[p->pos] = true;

    memcpy(&(p->main_color), &basic_colors_police_s[esp_random()%color_count_police_s], sizeof(pixel));

    p->step = 0;
    pol_update(p);
}

float quadratic_transition(float val){
    return (-4.0f*val*val)+(4.0f*val);
}

void pol_update(struct pol_light *p) {
    if(p->step<=p->steps){
        float mp = quadratic_transition((float)p->step/(float)p->steps);
        set_color(&strip[p->pos], 
        (uint8_t)(p->main_color.r*mp),
        (uint8_t)(p->main_color.g*mp),
        (uint8_t)(p->main_color.b*mp));
        p->step++;
    }
    else{
        pol_reset(p);
    }
}

void pol_reset_step(struct pol_light *p){
    p->step = esp_random()%(p->steps/2);
}

void pol_init_light(struct pol_light *p){
    p->pos = 0;
    p->step = 0;
    p->steps = 500;
    p->stall_counter = 0;
    
    set_color(&strip[p->pos], 0, 0, 0);
    light_pos_taken[p->pos] = false;
    
    do{
        p->stall_counter++;
        p->pos = esp_random()%(LED_NUM);
    }while((!(pol_free_spot(p->pos)))&&(p->stall_counter<100));
    p->stall_counter = 0;
    light_pos_taken[p->pos] = true;
    memcpy(&(p->main_color), &basic_colors_police_s[esp_random()%color_count_police_s], sizeof(pixel));
    p->step = esp_random()%(p->steps/2);
}


void sprinkle_update(){
    for(int i=0;i<LED_NUM;i++){

        float luminance = (0.2126f*strip[i].r + 0.7152f*strip[i].g + 0.0722f*strip[i].b);

        if(luminance > SPRINKLE_TRESHOLD){
            if(strip[i].r > SPRINKLE_DIMMING_SPEED)
                strip[i].r -= SPRINKLE_DIMMING_SPEED;
            else
                strip[i].r = 0;

            if(strip[i].g > SPRINKLE_DIMMING_SPEED)
                strip[i].g -= SPRINKLE_DIMMING_SPEED;
            else
                strip[i].g = 0;

            if(strip[i].b > SPRINKLE_DIMMING_SPEED)
                strip[i].b -= SPRINKLE_DIMMING_SPEED;
            else
                strip[i].b = 0;
            }
        else{
            int id = random()%color_count_s;
            strip[i] = basic_colors_s[id];
            strip[i].r -= random()%15;
            strip[i].g -= random()%15;
            strip[i].b -= random()%15;
        }
    }
}

void animation_task(void *pvParameters){

    strcpy(config.mode, "police");
    config.anim_refresh_rate = 120;
    config.rmt_refresh_rate = 150;
    config.brightness = 255;
    config.brightness_step = 50;
    config.default_animation = 1;
    config.enable_touch_control = true;

    config.selected_color.r = 255;
    config.selected_color.g = 0;
    config.selected_color.b = 12;

    // config.selected_color.r = 255;
    // config.selected_color.g = 255;
    // config.selected_color.b = 255;

    strip_data_semphr = xSemaphoreCreateBinary();
    xSemaphoreGive(strip_data_semphr);

    set_color(&basic_colors_s[0], 255,0,0);
    set_color(&basic_colors_s[1],255,60,12);
    set_color(&basic_colors_s[2],255,65,0);
    set_color(&basic_colors_s[3],0,0,255);
    set_color(&basic_colors_s[4],255,0,2);
    set_color(&basic_colors_s[5],255,0,12);
    set_color(&basic_colors_s[6],0,255,0);
    set_color(&basic_colors_s[7],0,255,255);
    set_color(&basic_colors_s[8],0,255,100);
    set_color(&basic_colors_s[9],255,200,128);

    set_color(&basic_colors_police_s[0],255,0,0);
    set_color(&basic_colors_police_s[1],0,0,255);
    set_color(&basic_colors_police_s[2],0,255,255);
    set_color(&basic_colors_police_s[3],255,0,12);

    clear_strip(strip);

    // init lights
    struct pol_light lights[POLICE_SPRINKLE_LIGHT_COUNT];
    for(int i = 0; i < POLICE_SPRINKLE_LIGHT_COUNT; i++){
        pol_init_light(&lights[i]);
    }
    for(int i = 0; i < LED_NUM; i++)
        light_pos_taken[i] = false;
    printf("pol_lights initiated with count: %d\n", POLICE_SPRINKLE_LIGHT_COUNT);

    uint32_t counter = 0;
    uint8_t color = 0;
    while (true){
        switch (anim_selected)
        {
        // Police sprinkle
        case 0:
            clear_strip(strip);
            xSemaphoreTake(strip_data_semphr, portMAX_DELAY);
            for(int i = 0; i < POLICE_SPRINKLE_LIGHT_COUNT; i++)
                pol_update(&lights[i]);
            xSemaphoreGive(strip_data_semphr);

            vTaskDelay(pdMS_TO_TICKS(1000/100));  
            break;
        // Rainbow
        case 1:
            clear_strip(strip);
            for(int i = 0; i < LED_NUM; i++){
                led_strip_hsv2rgb((uint32_t)(counter+360.0f*((float)i/LED_NUM))%360, 100, 100, &(strip[i].r), &(strip[i].g), &(strip[i].b));
            }
            counter+=1;
            counter%=360;
            vTaskDelay(pdMS_TO_TICKS(1000/120));  
            break;
        // Chase
        case 2:
            clear_strip(strip);
            if(color == 0)
                set_color(&strip[counter], 255, 0, 0);
            else if(color == 1)
                set_color(&strip[counter], 0, 255, 0);
            else
                set_color(&strip[counter], 0, 0, 255);
            counter+=1;
            if(counter == LED_NUM){
                color++;
                color%=3;
            }
            counter%=LED_NUM;
            vTaskDelay(pdMS_TO_TICKS(1000/60));   
            break;
        // White blinking
        case 3:
            for(int i = 0; i < LED_NUM; i++){
                if((i+counter)%2 == 0)
                    set_color(&strip[i],
                     config.selected_color.r,
                     config.selected_color.g,
                     config.selected_color.b);
                else
                    set_color(&strip[i], 0, 0, 0);
            }           
            counter+=1;
            vTaskDelay(pdMS_TO_TICKS(1000/2));   
            break;
        // Static 
        case 4:
            set_solid_color(strip,
                config.selected_color.r,
                config.selected_color.g,
                config.selected_color.b);
            vTaskDelay(pdMS_TO_TICKS(30));    
            break;
        // Off
        case 5:
            memset(strip, 0, 3*LED_NUM);
            vTaskDelay(pdMS_TO_TICKS(30));     
            break;
        // Breathing
        case 6: ;
            float sine_val = (1.0f + (sin1(animation_clock * 32767) / 32767.0f)) / 2.0f;

            set_solid_color(strip, 
            config.selected_color.r * sine_val, 
            config.selected_color.g * sine_val, 
            config.selected_color.b * sine_val);

            animation_clock += 0.001f;

            vTaskDelay(pdMS_TO_TICKS(1000/60));   
            break;
        case 7:
            sprinkle_update();
            
            vTaskDelay(pdMS_TO_TICKS(1000/60));   

            break;
        default:
            vTaskDelay(pdMS_TO_TICKS(1000/250));  
            break;
        }
    }
    vTaskDelete( NULL );
}