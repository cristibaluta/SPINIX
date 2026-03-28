#include "task_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ssd1681.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"
#include "img_bitmap.h"
#include "lvgl.h"

#include "data.h"
#include "pins.h"

static const char *TAG = "task_display";
esp_lcd_panel_handle_t panel_handle = NULL;
uint8_t *converted_buffer_black = NULL;
lv_display_t *disp_buf;
lv_fs_drv_t *disp_drv;

// SPI Bus
#define EPD_PANEL_SPI_CLK           1000000
#define EPD_PANEL_SPI_CMD_BITS      8
#define EPD_PANEL_SPI_PARAM_BITS    8
#define EPD_PANEL_SPI_MODE          0
#define EXAMPLE_LCD_H_RES 200
#define EXAMPLE_LCD_V_RES 200

static uint8_t *converted_buffer_black;
static uint8_t *converted_buffer_red;
static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // Used to vertical traverse lvgl framebuffer
    // int len_x = abs(offsetx1 - offsetx2) + 1;
    // int len_y = abs(offsety1 - offsety2) + 1;
    // --- Convert buffer from color to monochrome bitmap
    int len_bits = (abs(offsetx1 - offsetx2) + 1) * (abs(offsety1 - offsety2) + 1);

    memset(converted_buffer_black, 0x00, len_bits / 8);
    memset(converted_buffer_red, 0x00, len_bits / 8);
    for (int i = 0; i < len_bits; i++) {
        // NOTE: Set bits of converted_buffer[] FROM LOW ADDR TO HIGH ADDR, FROM HSB TO LSB
        // NOTE: 1 means BLACK/RED, 0 means WHITE
        // Horizontal traverse lvgl framebuffer (by row)
        converted_buffer_black[i / 8] |= (((lv_color_brightness(color_map[i])) < 251) << (7 - (i % 8)));
        converted_buffer_red[i / 8] |= ((((color_map[i].ch.red) > 3) && ((lv_color_brightness(color_map[i])) < 251)) << (7 - (i % 8)));
        // Vertical traverse lvgl framebuffer (by column), needs to uncomment len_x and len_y
        // NOTE: If your screen rotation requires setting the pixels vertically, you could use the code below
        // converted_buffer[i/8] |= (((lv_color_brightness(color_map[((i*len_x)%len_bits) + i/len_y])) > 250) << (7-(i % 8)));
    }
    // --- Draw bitmap

    ESP_ERROR_CHECK(epaper_panel_set_bitmap_color(panel_handle, SSD1681_EPAPER_BITMAP_BLACK));
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, converted_buffer_black));
    ESP_ERROR_CHECK(epaper_panel_set_bitmap_color(panel_handle, SSD1681_EPAPER_BITMAP_RED));
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, converted_buffer_red));
    ESP_ERROR_CHECK(epaper_panel_refresh_screen(panel_handle));
}

static void example_lvgl_wait_cb(struct _lv_disp_drv_t *disp_drv)
{
    // xSemaphoreTake(panel_refreshing_sem, portMAX_DELAY);
}

void init_ssd1681() {
    esp_err_t ret;
    // --- Init SPI Bus
    ESP_LOGI(TAG, "Initializing SPI Bus...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SPI_CLK,
        .mosi_io_num = PIN_SPI_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // --- Init ESP_LCD IO
    ESP_LOGI(TAG, "Initializing panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_SPI_DC,
        .cs_gpio_num = PIN_SPI_CS,
        .pclk_hz = EPD_PANEL_SPI_CLK,
        .lcd_cmd_bits = EPD_PANEL_SPI_CMD_BITS,
        .lcd_param_bits = EPD_PANEL_SPI_PARAM_BITS,
        .spi_mode = EPD_PANEL_SPI_MODE,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI2_HOST, &io_config, &io_handle));
    // --- Create esp_lcd panel
    ESP_LOGI(TAG, "Creating SSD1681 panel...");
    esp_lcd_ssd1681_config_t epaper_ssd1681_config = {
        .busy_gpio_num = PIN_SPI_BUSY,
        // NOTE: Enable this to reduce one buffer copy if you do not use swap-xy, mirror y or invert color
        // since those operations are not supported by ssd1681 and are implemented by software
        // Better use DMA-capable memory region, to avoid additional data copy
        .non_copy_mode = false,
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_SPI_RST,
        .flags.reset_active_high = false,
        .vendor_config = &epaper_ssd1681_config
    };
    // NOTE: Please call gpio_install_isr_service() manually before esp_lcd_new_panel_ssd1681()
    // because gpio_isr_handler_add() is called in esp_lcd_new_panel_ssd1681()
    gpio_install_isr_service(0);
    ret = esp_lcd_new_panel_ssd1681(io_handle, &panel_config, &panel_handle);
    ESP_ERROR_CHECK(ret);
    // --- Reset the display
    ESP_LOGI(TAG, "Resetting e-Paper display...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // --- Initialize LCD panel
    ESP_LOGI(TAG, "Initializing e-Paper display...");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // Turn on the screen
    ESP_LOGI(TAG, "Turning e-Paper display on...");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    // --- Configurate the screen
    // NOTE: the configurations below are all FALSE by default
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    esp_lcd_panel_invert_color(panel_handle, false);
    // NOTE: Calling esp_lcd_panel_disp_on_off(panel_handle, true) will reset the LUT to the panel built-in one,
    // custom LUT will not take effect any more after calling esp_lcd_panel_disp_on_off(panel_handle, true)
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

void init_lvgl() {
    // --- Initialize LVGL
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // disp = lv_display_create(200, 200);
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 200 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 200 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // alloc bitmap buffer to draw
    // 1. Allocate your buffers (keep your existing DMA allocation)
    converted_buffer_black = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8, MALLOC_CAP_DMA);

    // 2. Create the display object (this replaces drv_init and register)
    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    // 3. Set the draw buffers (replaces lv_disp_draw_buf_init)
    // buf1 and buf2 are your internal LVGL render buffers
    lv_display_set_buffers(disp, buf1, buf2, EXAMPLE_LCD_H_RES * 200, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 4. Assign callbacks and user data
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);
    lv_display_set_user_data(disp, panel_handle);

    // 5. Set Full Refresh mode (MANDATORY for most e-papers)
    lv_display_set_antialiasing(disp, false); // Recommended for monochrome
    // In v9, "full_refresh" is handled by the render mode or direct flag:
    // lv_display_set_offset(disp, 0, 0); // Ensure origin is correct

    // 6. Install LVGL tick timer (Logic remains same, ensure headers are included)
    ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));



    // converted_buffer_black = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8, MALLOC_CAP_DMA);
    // // initialize LVGL draw buffers
    // lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 200);
    // // initialize LVGL display driver
    // lv_fs_drv_init(&disp_drv);
    // disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    // disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    // disp_drv.flush_cb = example_lvgl_flush_cb;
    // disp_drv.wait_cb = example_lvgl_wait_cb;
    // disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    // disp_drv.draw_buf = &disp_buf;
    // disp_drv.user_data = panel_handle;
    // // NOTE: The ssd1681 e-paper is monochrome and 1 byte represents 8 pixels
    // // so full_refresh is MANDATORY because we cannot set position to bitmap at pixel level
    // disp_drv.full_refresh = true;
    // ESP_LOGI(TAG, "Register display driver to LVGL");
    // lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    // // init lvgl tick
    // ESP_LOGI(TAG, "Install LVGL tick timer");
    // // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    // const esp_timer_create_args_t lvgl_tick_timer_args = {
    //     .callback = &example_increase_lvgl_tick,
    //     .name = "lvgl_tick"
    // };
    // esp_timer_handle_t lvgl_tick_timer = NULL;
    // ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    // ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    // meter = lv_meter_create(lv_scr_act());
    // lv_obj_set_size(meter, 200, 200);
    // lv_obj_center(meter);

    // /*Create a scale for the minutes*/
    // /*61 ticks in a 360 degrees range (the last and the first line overlaps)*/
    // lv_meter_scale_t *scale_min = lv_meter_add_scale(meter);
    // lv_meter_set_scale_ticks(meter, scale_min, 61, 1, 10, lv_palette_main(LV_PALETTE_BLUE));
    // lv_meter_set_scale_range(meter, scale_min, 0, 60, 360, 270);

    // /*Create another scale for the hours. It's only visual and contains only major ticks*/
    // lv_meter_scale_t *scale_hour = lv_meter_add_scale(meter);
    // lv_meter_set_scale_ticks(meter, scale_hour, 12, 0, 0, lv_palette_main(LV_PALETTE_BLUE));               /*12 ticks*/
    // lv_meter_set_scale_major_ticks(meter, scale_hour, 1, 2, 20, lv_palette_main(LV_PALETTE_BLUE), 10);    /*Every tick is major*/
    // lv_meter_set_scale_range(meter, scale_hour, 1, 12, 330, 300);       /*[1..12] values in an almost full circle*/

    // LV_IMG_DECLARE(img_hand)

    // /*Add the hands from images*/
    // lv_meter_indicator_t *indic_min = lv_meter_add_needle_line(meter, scale_min, 4, lv_palette_main(LV_PALETTE_RED), -10);
    // lv_meter_indicator_t *indic_hour = lv_meter_add_needle_img(meter, scale_min, &img_hand, 5, 5);

    // /*Create an animation to set the value*/
    // lv_anim_t a;
    // lv_anim_init(&a);
    // lv_anim_set_exec_cb(&a, set_value);
    // lv_anim_set_values(&a, 0, 60);
    // lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    // lv_anim_set_time(&a, 60000 * 30);     /* 60 * 30 sec for 1 turn, 30 sec for 1 move of the minute hand*/
    // lv_anim_set_var(&a, indic_min);
    // lv_anim_start(&a);

    // lv_anim_set_var(&a, indic_hour);
    // lv_anim_set_time(&a, 60000 * 60 * 10);    /*36000 sec for 1 turn, 600 sec for 1 move of the hour hand*/
    // lv_anim_set_values(&a, 0, 60);
    // lv_anim_start(&a);
}


void task_display(void *param) {
    init_ssd1681();
    init_lvgl();

    // uint8_t *clear_buffer = malloc(5000);
    // memset(clear_buffer, 0x00, 5000);
    // // Push the "white" buffer to the panel
    // // panel_handle is your esp_lcd_panel_handle_t
    // esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 200, 200, clear_buffer);
    // esp_lcd_panel_disp_on_off(panel_handle, true);
    // free(clear_buffer);

    // esp_lcd_panel_invert_color(panel_handle, false);

    // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
    // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    // ESP_LOGI(TAG, "Drawing bitmap...");
    // ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 10, 128, 64, yapragim_128x64_bitmap));
    // ESP_ERROR_CHECK(epaper_panel_refresh_screen(panel_handle));

    ESP_LOGI(TAG, "Display LVGL Meter Widget");
    example_lvgl_demo_ui(disp_buf);

    while (1) {
        // bike_data_t d = data_get_snapshot();
        // display_draw_speed(d.speed);
        // display_draw_coords(d.lat, d.lon);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
