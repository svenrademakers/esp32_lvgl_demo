#include "draw/lv_draw_vector.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_st7796.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "hal/ledc_types.h"
#include "hal/spi_types.h"
#include "lvgl.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DIAMETER 360
#define WIDTH DIAMETER
#define HEIGHT 320
#define TFT_SPI_FREQ_HZ (50 * 1000 * 1000)

#define DISP_BUF_SIZE 1024 * 8
#define DISP_TAG "DISP_INIT"

#define QSPI_CLK 9
#define QSPI_CS 10
#define QSPI_D0 11
#define QSPI_D1 12
#define QSPI_D2 13
#define QSPI_D3 14
#define DISP_RST 47

#define TRANSACTION_SIZE 2

static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_display_t *disp;

static const st7796_lcd_init_cmd_t init_cmd_list[] = {
    {0xF0, (uint8_t[]){0x10}, 1, 0}, {0xF3, (uint8_t[]){0x10}, 1, 0},
    {0xE0, (uint8_t[]){0x0A}, 1, 0}, {0xE1, (uint8_t[]){0x00}, 1, 0},
    {0xE2, (uint8_t[]){0x0B}, 1, 0}, {0xE3, (uint8_t[]){0x00}, 1, 0},
    {0xE4, (uint8_t[]){0xE0}, 1, 0}, {0xE5, (uint8_t[]){0x06}, 1, 0},
    {0xE6, (uint8_t[]){0x21}, 1, 0}, {0xE7, (uint8_t[]){0x00}, 1, 0},
    {0xE8, (uint8_t[]){0x05}, 1, 0}, {0xE9, (uint8_t[]){0x82}, 1, 0},
    {0xEA, (uint8_t[]){0xDF}, 1, 0}, {0xEB, (uint8_t[]){0x89}, 1, 0},
    {0xEC, (uint8_t[]){0x20}, 1, 0}, {0xED, (uint8_t[]){0x14}, 1, 0},
    {0xEE, (uint8_t[]){0xFF}, 1, 0}, {0xEF, (uint8_t[]){0x00}, 1, 0},
    {0xF8, (uint8_t[]){0xFF}, 1, 0}, {0xF9, (uint8_t[]){0x00}, 1, 0},
    {0xFA, (uint8_t[]){0x00}, 1, 0}, {0xFB, (uint8_t[]){0x30}, 1, 0},
    {0xFC, (uint8_t[]){0x00}, 1, 0}, {0xFD, (uint8_t[]){0x00}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0}, {0xFF, (uint8_t[]){0x00}, 1, 0},
    {0x60, (uint8_t[]){0x42}, 1, 0}, {0x61, (uint8_t[]){0xE0}, 1, 0},
    {0x62, (uint8_t[]){0x40}, 1, 0}, {0x63, (uint8_t[]){0x40}, 1, 0},
    {0x64, (uint8_t[]){0x02}, 1, 0}, {0x65, (uint8_t[]){0x00}, 1, 0},
    {0x66, (uint8_t[]){0x40}, 1, 0}, {0x67, (uint8_t[]){0x03}, 1, 0},
    {0x68, (uint8_t[]){0x00}, 1, 0}, {0x69, (uint8_t[]){0x00}, 1, 0},
    {0x6A, (uint8_t[]){0x00}, 1, 0}, {0x6B, (uint8_t[]){0x00}, 1, 0},
    {0x70, (uint8_t[]){0x42}, 1, 0}, {0x71, (uint8_t[]){0xE0}, 1, 0},
    {0x72, (uint8_t[]){0x40}, 1, 0}, {0x73, (uint8_t[]){0x40}, 1, 0},
    {0x74, (uint8_t[]){0x02}, 1, 0}, {0x75, (uint8_t[]){0x00}, 1, 0},
    {0x76, (uint8_t[]){0x40}, 1, 0}, {0x77, (uint8_t[]){0x03}, 1, 0},
    {0x78, (uint8_t[]){0x00}, 1, 0}, {0x79, (uint8_t[]){0x00}, 1, 0},
    {0x7A, (uint8_t[]){0x00}, 1, 0}, {0x7B, (uint8_t[]){0x00}, 1, 0},
    {0x80, (uint8_t[]){0x38}, 1, 0}, {0x81, (uint8_t[]){0x00}, 1, 0},
    {0x82, (uint8_t[]){0x04}, 1, 0}, {0x83, (uint8_t[]){0x02}, 1, 0},
    {0x84, (uint8_t[]){0xDC}, 1, 0}, {0x85, (uint8_t[]){0x00}, 1, 0},
    {0x86, (uint8_t[]){0x00}, 1, 0}, {0x87, (uint8_t[]){0x00}, 1, 0},
    {0x88, (uint8_t[]){0x38}, 1, 0}, {0x89, (uint8_t[]){0x00}, 1, 0},
    {0x8A, (uint8_t[]){0x06}, 1, 0}, {0x8B, (uint8_t[]){0x02}, 1, 0},
    {0x8C, (uint8_t[]){0xDE}, 1, 0}, {0x8D, (uint8_t[]){0x00}, 1, 0},
    {0x8E, (uint8_t[]){0x00}, 1, 0}, {0x8F, (uint8_t[]){0x00}, 1, 0},
    {0x90, (uint8_t[]){0x38}, 1, 0}, {0x91, (uint8_t[]){0x00}, 1, 0},
    {0x92, (uint8_t[]){0x08}, 1, 0}, {0x93, (uint8_t[]){0x02}, 1, 0},
    {0x94, (uint8_t[]){0xE0}, 1, 0}, {0x95, (uint8_t[]){0x00}, 1, 0},
    {0x96, (uint8_t[]){0x00}, 1, 0}, {0x97, (uint8_t[]){0x00}, 1, 0},
    {0x98, (uint8_t[]){0x38}, 1, 0}, {0x99, (uint8_t[]){0x00}, 1, 0},
    {0x9A, (uint8_t[]){0x0A}, 1, 0}, {0x9B, (uint8_t[]){0x02}, 1, 0},
    {0x9C, (uint8_t[]){0xE2}, 1, 0}, {0x9D, (uint8_t[]){0x00}, 1, 0},
    {0x9E, (uint8_t[]){0x00}, 1, 0}, {0x9F, (uint8_t[]){0x00}, 1, 0},
    {0xA0, (uint8_t[]){0x38}, 1, 0}, {0xA1, (uint8_t[]){0x00}, 1, 0},
    {0xA2, (uint8_t[]){0x03}, 1, 0}, {0xA3, (uint8_t[]){0x02}, 1, 0},
    {0xA4, (uint8_t[]){0xDB}, 1, 0}, {0xA5, (uint8_t[]){0x00}, 1, 0},
    {0xA6, (uint8_t[]){0x00}, 1, 0}, {0xA7, (uint8_t[]){0x00}, 1, 0},
    {0xA8, (uint8_t[]){0x38}, 1, 0}, {0xA9, (uint8_t[]){0x00}, 1, 0},
    {0xAA, (uint8_t[]){0x05}, 1, 0}, {0xAB, (uint8_t[]){0x02}, 1, 0},
    {0xAC, (uint8_t[]){0xDD}, 1, 0}, {0xAD, (uint8_t[]){0x00}, 1, 0},
    {0xAE, (uint8_t[]){0x00}, 1, 0}, {0xAF, (uint8_t[]){0x00}, 1, 0},
    {0xB0, (uint8_t[]){0x38}, 1, 0}, {0xB1, (uint8_t[]){0x00}, 1, 0},
    {0xB2, (uint8_t[]){0x07}, 1, 0}, {0xB3, (uint8_t[]){0x02}, 1, 0},
    {0xB4, (uint8_t[]){0xDF}, 1, 0}, {0xB5, (uint8_t[]){0x00}, 1, 0},
    {0xB6, (uint8_t[]){0x00}, 1, 0}, {0xB7, (uint8_t[]){0x00}, 1, 0},
    {0xB8, (uint8_t[]){0x38}, 1, 0}, {0xB9, (uint8_t[]){0x00}, 1, 0},
    {0xBA, (uint8_t[]){0x09}, 1, 0}, {0xBB, (uint8_t[]){0x02}, 1, 0},
    {0xBC, (uint8_t[]){0xE1}, 1, 0}, {0xBD, (uint8_t[]){0x00}, 1, 0},
    {0xBE, (uint8_t[]){0x00}, 1, 0}, {0xBF, (uint8_t[]){0x00}, 1, 0},
    {0xC0, (uint8_t[]){0x22}, 1, 0}, {0xC1, (uint8_t[]){0xAA}, 1, 0},
    {0xC2, (uint8_t[]){0x65}, 1, 0}, {0xC3, (uint8_t[]){0x74}, 1, 0},
    {0xC4, (uint8_t[]){0x47}, 1, 0}, {0xC5, (uint8_t[]){0x56}, 1, 0},
    {0xC6, (uint8_t[]){0x00}, 1, 0}, {0xC7, (uint8_t[]){0x88}, 1, 0},
    {0xC8, (uint8_t[]){0x99}, 1, 0}, {0xC9, (uint8_t[]){0x33}, 1, 0},
    {0xD0, (uint8_t[]){0x11}, 1, 0}, {0xD1, (uint8_t[]){0xAA}, 1, 0},
    {0xD2, (uint8_t[]){0x65}, 1, 0}, {0xD3, (uint8_t[]){0x74}, 1, 0},
    {0xD4, (uint8_t[]){0x47}, 1, 0}, {0xD5, (uint8_t[]){0x56}, 1, 0},
    {0xD6, (uint8_t[]){0x00}, 1, 0}, {0xD7, (uint8_t[]){0x88}, 1, 0},
    {0xD8, (uint8_t[]){0x99}, 1, 0}, {0xD9, (uint8_t[]){0x33}, 1, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0}, {0xF1, (uint8_t[]){0x10}, 1, 0},
    {0xF3, (uint8_t[]){0x01}, 1, 0},
};

void init_backlight(void) {
  ledc_timer_config_t ledc_timer = {0};
  ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num = 0;
  ledc_timer.duty_resolution = LEDC_TIMER_5_BIT;
  ledc_timer.freq_hz = 24000;
  ledc_timer.clk_cfg = LEDC_APB_CLK;

  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  ESP_LOGI(DISP_TAG, "LEDC timer configured");

  ledc_channel_config_t ledc_channel = {0};
  ledc_channel.gpio_num = 15;
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.duty = 32;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  ESP_LOGI(DISP_TAG, "LEDC channel configured");
}

uint32_t elapsed_time(void) { return esp_timer_get_time() / 1000; }

void init_spi_bus() {
  ESP_LOGI(DISP_TAG, "spi device setup");

  spi_bus_config_t bus_cfg = {0};
  bus_cfg.sclk_io_num = QSPI_CLK;
  bus_cfg.mosi_io_num = QSPI_D0;
  bus_cfg.miso_io_num = QSPI_D1;
  bus_cfg.data2_io_num = QSPI_D2;
  bus_cfg.data3_io_num = QSPI_D3;
  bus_cfg.data4_io_num = -1;
  bus_cfg.data5_io_num = -1;
  bus_cfg.data6_io_num = -1;
  bus_cfg.data7_io_num = -1;
  bus_cfg.max_transfer_sz = DISP_BUF_SIZE;
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
}

lv_display_t *display_start(void) {
  ESP_LOGD(DISP_TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = -1,
      .cs_gpio_num = QSPI_CS,
      .pclk_hz = TFT_SPI_FREQ_HZ,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .spi_mode = 0,
      .trans_queue_depth = 10,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                           &io_config, &io_handle));

  ESP_LOGD(DISP_TAG, "Install LCD driver of ST7796");
  st7796_vendor_config_t vendor_config = {0};
  vendor_config.init_cmds = init_cmd_list;
  vendor_config.init_cmds_size =
      sizeof(init_cmd_list) / sizeof(st7796_lcd_init_cmd_t);

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = DISP_RST,
      .rgb_endian = LCD_RGB_ENDIAN_BGR,
      .bits_per_pixel = 16,
      .vendor_config = &vendor_config,
  };

  ESP_ERROR_CHECK(
      esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  /* Add LCD screen */
  ESP_LOGD(DISP_TAG, "Add LCD screen");
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = HEIGHT * 100,
      .double_buffer = false,
      .hres = HEIGHT,
      .vres = WIDTH,
      .monochrome = false,
      .color_format = LV_COLOR_FORMAT_RGB565,
      /* Rotation values must be same as used in esp_lcd for initial settings of
         the screen */
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = false,
              .mirror_y = false,
          },
      .flags = {
          .buff_dma = true, .buff_spiram = false,
          //.sw_rotate =
          //.swap_bytes
      }};

  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
  disp = lvgl_port_add_disp(&disp_cfg);

  return disp;
}

void lv_example_get_started_1(void) {
  lvgl_port_lock(0);
  /*Change the active screen's background color*/
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57),
                            LV_PART_MAIN);

  /*Create a white label, set its text and align it to the center*/
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello world");
  lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lvgl_port_unlock();
}

void app_main(void) {

  init_spi_bus();
  init_backlight();
  display_start();

  lv_tick_set_cb(elapsed_time);
  lv_example_get_started_1();

  while (1) {
    ESP_LOGW("LVGL", ".");
    uint32_t time_till_next = lv_timer_handler();
    vTaskDelay(time_till_next / portTICK_PERIOD_MS);
  }
}
