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

// SPI Bus
#define EPD_PANEL_SPI_CLK           1000000
#define EPD_PANEL_SPI_CMD_BITS      8
#define EPD_PANEL_SPI_PARAM_BITS    8
#define EPD_PANEL_SPI_MODE          0
#define EXAMPLE_LCD_H_RES 200
#define EXAMPLE_LCD_V_RES 200
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2


static esp_lcd_panel_handle_t s_panel_handle = NULL;

static void lvgl_tick_cb(void *arg) {
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    ESP_LOGI(TAG, "flush area: x1=%d y1=%d x2=%d y2=%d", offsetx1, offsety1, offsetx2, offsety2);

    epaper_panel_set_bitmap_color(s_panel_handle, SSD1681_EPAPER_BITMAP_BLACK);
    esp_lcd_panel_draw_bitmap(s_panel_handle, offsetx1, offsety1,
                                              offsetx2 + 1, offsety2 + 1,
                                              px_map + 8);
    epaper_panel_refresh_screen(s_panel_handle);
    lv_display_flush_ready(disp);
}

void init_ssd1681() {
    esp_err_t ret;
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
    ret = esp_lcd_new_panel_ssd1681(io_handle, &panel_config, &s_panel_handle);
    ESP_ERROR_CHECK(ret);
    // --- Reset the display
    ESP_LOGI(TAG, "Resetting e-Paper display...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // --- Initialize LCD panel
    ESP_LOGI(TAG, "Initializing e-Paper display...");
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // Turn on the screen
    ESP_LOGI(TAG, "Turning e-Paper display on...");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
    // --- Configurate the screen
    // NOTE: the configurations below are all FALSE by default
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel_handle, false));
    esp_lcd_panel_invert_color(s_panel_handle, false);
    // NOTE: Calling esp_lcd_panel_disp_on_off(panel_handle, true) will reset the LUT to the panel built-in one,
    // custom LUT will not take effect any more after calling esp_lcd_panel_disp_on_off(panel_handle, true)
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
}

static void init_lvgl(void) {
    ESP_LOGI(TAG, "Initializing LVGL v9...");
    ESP_LOGI(TAG, "Free DMA memory: %d bytes", heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "Free internal memory: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "Largest free DMA block: %d bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "Largest free internal block: %d bytes", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    size_t buf_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LV_COLOR_DEPTH;
    ESP_LOGI(TAG, "Attempting to allocate: %d bytes", buf_size);
    ESP_LOGI(TAG, "sizeof(lv_color_t): %d bytes", sizeof(lv_color_t));
    // lv_color_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    lv_init();
    ESP_LOGI(TAG, "LVGL color depth: %d", LV_COLOR_DEPTH);
    ESP_LOGI(TAG, "sizeof(lv_color_t): %d", sizeof(lv_color_t));

    // allocate render buffers
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 1, MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 1, MALLOC_CAP_DMA);
    assert(buf1 && buf2);

    // allocate bitmap conversion buffers
    // s_converted_buffer_black = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8, MALLOC_CAP_DMA);
    // assert(s_converted_buffer_black);

    // create display
    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_display_set_buffers(disp, buf1, buf2,
                           EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 1,
                           LV_DISPLAY_RENDER_MODE_FULL);  // e-ink needs full refresh
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_user_data(disp, s_panel_handle);
    lv_display_set_antialiasing(disp, false);

    // tick timer
    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "LVGL initialized");
    ESP_LOGI(TAG, "display width: %d height: %d", 
         lv_display_get_horizontal_resolution(lv_display_get_default()),
         lv_display_get_vertical_resolution(lv_display_get_default()));
}

static void build_ui(void) {

    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    // ascent label
    lv_obj_t *ascent = lv_label_create(screen);
    lv_label_set_text(ascent, "1240 M");
    lv_obj_set_style_text_font(ascent, &lv_font_montserrat_20, 0);
    lv_obj_align(ascent, LV_ALIGN_TOP_LEFT, 4, 4);

    // descent label
    lv_obj_t *descent = lv_label_create(screen);
    lv_label_set_text(descent, "150 M");
    lv_obj_set_style_text_font(descent, &lv_font_montserrat_20, 0);
    lv_obj_align(descent, LV_ALIGN_TOP_RIGHT, -4, 4);

    // speed label
    lv_obj_t *speed_val = lv_label_create(screen);
    lv_label_set_text(speed_val, "23.5");
    lv_obj_set_style_text_font(speed_val, &lv_font_montserrat_48, 0);
    lv_obj_align(speed_val, LV_ALIGN_CENTER, -10, 0);

    lv_obj_t *speed_unit = lv_label_create(screen);
    lv_label_set_text(speed_unit, "KM/H");
    lv_obj_set_style_text_font(speed_unit, &lv_font_montserrat_20, 0);
    lv_obj_align(speed_unit, LV_ALIGN_RIGHT_MID, -4, 9);

    // distance label
    lv_obj_t *distance = lv_label_create(screen);
    lv_label_set_text(distance, "120.56 KM");
    lv_obj_set_style_text_font(distance, &lv_font_montserrat_20, 0);
    lv_obj_align(distance, LV_ALIGN_CENTER, 0, 40);


    // logo
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "SPINIX");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);


    // lv_obj_set_pos(speed_label, 0, 0);    // x, y from top-left
    // lv_obj_set_pos(coords_label, 63, 92);
    // lv_obj_set_pos(label, 0, 184);

    lv_obj_update_layout(screen);
}

void build_ssd1681_ui() {
    uint8_t *clear_buffer = malloc(5000);
    memset(clear_buffer, 0x00, 5000);
    // Push the "white" buffer to the panel
    esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, 200, 200, clear_buffer);
    esp_lcd_panel_disp_on_off(s_panel_handle, true);
    free(clear_buffer);

    esp_lcd_panel_invert_color(s_panel_handle, false);

    esp_lcd_panel_mirror(s_panel_handle, false, false);
    esp_lcd_panel_invert_color(s_panel_handle, false);
    esp_lcd_panel_swap_xy(s_panel_handle, false);
    esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, 128, 64, BITMAP_128_64);
    epaper_panel_refresh_screen(s_panel_handle);
}

void task_display(void *param) {
    init_ssd1681();
    init_lvgl();
    build_ui();

    // build_ssd1681_ui();

    while (1) {
        // bike_data_t d = data_get_snapshot();
        // display_draw_speed(d.speed);
        // display_draw_coords(d.lat, d.lon);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
