#include "led_driver.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/rmt.h"
#include "esp_system.h"
#include <string.h>
#include "driver/i2s.h"
//#include "driver/spi_master.h"
#include <string.h>
#include <utils.h>

static bool light_pos_taken[LED_NUM];
pixel strip[LED_NUM];
static pixel strip_temp[LED_NUM];
uint8_t *strip_i2s_data;
static uint8_t zero_buffer[ZERO_BUFFER_SIZE];

static pixel basic_colors_s[10];
//static const uint8_t color_count_s;
static pixel basic_colors_police_s[4];
// static const uint8_t color_count_police_s;
// static uint8_t anim_selected;
// static uint8_t brightness;
// static uint8_t step;

//static const uint8_t color_count_s = 10;
static const uint8_t color_count_police_s = 4;
static uint8_t anim_selected = DEFAULT_ANIMATION;
static uint8_t brightness=255;
static uint8_t step = 4;
static SemaphoreHandle_t strip_data_semphr;
//static uint32_t clock_rate = 55555;
static uint32_t clock_rate = 93750;

static float animation_clock = 0;

Config config;

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

uint32_t update_clk_rate(){
    clock_rate+=1000;
    //uint32_t bits_cfg = (I2S_BITS_PER_CHAN_32BIT << 16) | I2S_BITS_PER_SAMPLE_16BIT;
    uint32_t bits_cfg = (I2S_BITS_PER_CHAN_16BIT << 16) | I2S_BITS_PER_SAMPLE_8BIT;
    i2s_set_clk(I2S_NUM_1, clock_rate, bits_cfg, I2S_CHANNEL_FMT_RIGHT_LEFT);
    return clock_rate;
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

void led_task(void *pvParameters){
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_GPIO, RMT_CHANNEL_0);
    // set counter clock to 40MHz
    config.clk_div = 2;
    uint32_t clock_hz = 0;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    ESP_ERROR_CHECK(rmt_get_counter_clock(config.channel, &clock_hz));
    const rmt_item32_t rmt_items[LED_NUM*24+1];
    printf("rmt_clock_hz: %d\n", clock_hz);

    if(strip_data_semphr == NULL)
        printf("led_task: strip_data_semphr: NULL\n");

    while (true) {
        update_strip(&config, rmt_items, strip, LED_NUM);
        vTaskDelay(pdMS_TO_TICKS(1000/RMT_REFRESH_RATE));
    }
    vTaskDelete( NULL );
} 

void led_task_i2s(void *pvParameters){
    printf("\r\nFree mem=%d\n", esp_get_free_heap_size());
    //uint8_t strip_i2s_data[LED_NUM*24];
    strip_i2s_data = heap_caps_malloc(LED_NUM*12, MALLOC_CAP_DMA); // Critical to be DMA memory.
    printf("strip_i2s_data addr=%d\n", (uint32_t)strip_i2s_data);

    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = clock_rate,
        //.sample_rate = 44100, 
        //.sample_rate = 1,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        //.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
        .dma_desc_num = 4,
        .dma_frame_num = 720,
        .use_apll = false,
        .intr_alloc_flags = 0  // Interrupt level 1, default 0
    };
    static const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = LED_GPIO,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL));
    printf("I2S driver initiated\n");
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_1, &pin_config));
    printf("I2S pins initiated\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    memset(zero_buffer, 0, ZERO_BUFFER_SIZE);

    while(true){
        encode_strip_i2s(strip, LED_NUM);
        vTaskDelay(pdMS_TO_TICKS(1000/10));
    }
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

static const uint16_t bitpatterns[4] = {0x88, 0x8e, 0xe8, 0xee};
void encode_strip_i2s(pixel strip[], uint32_t led_num){
    apply_brightness(strip);
    // const uint8_t bit0 = 0b11000000;
    // const uint8_t bit1 = 0b11111000;
    const uint8_t bit0 = 0b11000000;
    const uint8_t bit1 = 0b11111100;
    
    if(strip_data_semphr == NULL){
        printf("strip_data_semphr: NULL\n");
        return;
    }
    uint8_t *p = strip_i2s_data;

    //uint16_t bitpatterns[4] = {0b10001000, 0b10001110, 0b11101000, 0b11101110};

    xSemaphoreTake(strip_data_semphr, portMAX_DELAY);
    for(int i = 0; i < LED_NUM; i++){

        int loc = i * 12;
        strip_i2s_data[loc+0] = bitpatterns[strip_temp[i].g >> 6 & 0x03];
        strip_i2s_data[loc+1] = bitpatterns[strip_temp[i].g >> 4 & 0x03];
        strip_i2s_data[loc+2] = bitpatterns[strip_temp[i].g >> 2 & 0x03];
        strip_i2s_data[loc+3] = bitpatterns[strip_temp[i].g & 0x03];

        strip_i2s_data[loc+4] = bitpatterns[strip_temp[i].r >> 6 & 0x03];
        strip_i2s_data[loc+5] = bitpatterns[strip_temp[i].r >> 4 & 0x03];
        strip_i2s_data[loc+6] = bitpatterns[strip_temp[i].r >> 2 & 0x03];
        strip_i2s_data[loc+7] = bitpatterns[strip_temp[i].r & 0x03];

        strip_i2s_data[loc+8] = bitpatterns[strip_temp[i].b >> 6 & 0x03];
        strip_i2s_data[loc+9] = bitpatterns[strip_temp[i].b >> 4 & 0x03];
        strip_i2s_data[loc+10] = bitpatterns[strip_temp[i].b >> 2 & 0x03];
        strip_i2s_data[loc+11] = bitpatterns[strip_temp[i].b & 0x03];


        // p+=12;

        // for(int j = 0; j < 8; j++){
        //     if(*((uint8_t*)strip_temp+i) & (1 << (7-j)))
        //         *p = bit1;
        //     else{
        //         *p = bit0;
        //     }
        // }
    }
    xSemaphoreGive(strip_data_semphr);

    size_t bytes_written;
    ESP_ERROR_CHECK(i2s_write(I2S_NUM_1, strip_i2s_data, LED_NUM * 12, &bytes_written, portMAX_DELAY));
    ESP_ERROR_CHECK(i2s_write(I2S_NUM_1, zero_buffer, ZERO_BUFFER_SIZE, &bytes_written, portMAX_DELAY));
    i2s_zero_dma_buffer(I2S_NUM_1);
    //printf(bytes_written);
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
    else
        return 3;
}

void print_config(){
    printf("Config:\n");
    printf("mode: %s\n", config.mode);
    printf("brightness: %d\n", config.brightness);
    printf("selected_color: %d %d %d\n\n", config.selected_color.r, config.selected_color.g, config.selected_color.b);
}

void animation_task(void *pvParameters){

    strcpy(config.mode, "police");
    config.anim_refresh_rate = 120;
    config.rmt_refresh_rate = 150;
    config.brightness = 255;
    config.brightness_step = 50;
    config.default_animation = 1;
    config.enable_touch_control = true;

    strip_data_semphr = xSemaphoreCreateBinary();
    xSemaphoreGive(strip_data_semphr);
    printf("Entered animation_task\n");
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
            //memset(strip, config.selected_color, LED_NUM);
            // for(int i = 0; i < LED_NUM; i++){
            //     strip[i].r = config.selected_color.r;
            //     strip[i].g = config.selected_color.g;
            //     strip[i].b = config.selected_color.b;
            // }
            vTaskDelay(pdMS_TO_TICKS(30));    
            break;
        // Off
        case 5:
            memset(strip, 0, 3*LED_NUM);
            //vTaskDelay(pdMS_TO_TICKS(1000/60));   
            vTaskDelay(pdMS_TO_TICKS(30));     
            break;
        // Breathing
        case 6: ;
            float sine_val = (1.0f + (sin1(animation_clock * 32767) / 32767.0f)) / 2.0f;

            printf("sine_val: %f\n", sine_val);

            set_solid_color(strip, 
            config.selected_color.r * sine_val, 
            config.selected_color.g * sine_val, 
            config.selected_color.b * sine_val);

            animation_clock += 0.001f;

            vTaskDelay(pdMS_TO_TICKS(1000/60));   
            break;
        default:
            vTaskDelay(pdMS_TO_TICKS(1000/250));  
            break;
        }
    }
    vTaskDelete( NULL );
}