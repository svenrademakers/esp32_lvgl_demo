#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_st77916.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "hal/ledc_types.h"
#include "hal/spi_types.h"
#include "init.h"
#include "lvgl_lottie.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DIAMETER 360
#define WIDTH DIAMETER
#define HEIGHT 360

#define DISP_BUF_SIZE 4092 * 2
#define DISP_TAG "DISP_INIT"

#define QSPI_CLK 9
#define QSPI_CS 10
#define QSPI_D0 11
#define QSPI_D1 12
#define QSPI_D2 13
#define QSPI_D3 14
#define DISP_RST 47

static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_display_t *disp;

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
  bus_cfg.max_transfer_sz = DISP_BUF_SIZE * sizeof(uint16_t);
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
}

void display_start(void) {
  ESP_LOGD(DISP_TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config =
      ST77916_PANEL_IO_QSPI_CONFIG(QSPI_CS, NULL, NULL);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                           &io_config, &io_handle));

  ESP_LOGD(DISP_TAG, "Install LCD driver of ST77916");
  st77916_vendor_config_t vendor_config = {
      .init_cmds = init_cmd_list,
      .init_cmds_size = sizeof(init_cmd_list) / sizeof(st77916_lcd_init_cmd_t),
      .flags =
          {
              .use_qspi_interface = 1,
          },
  };

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = DISP_RST,
      .rgb_ele_order = COLOR_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
      .vendor_config = &vendor_config,
  };

  ESP_ERROR_CHECK(
      esp_lcd_new_panel_st77916(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  /* Add LCD screen */
  ESP_LOGD(DISP_TAG, "Add LCD screen");
  const lvgl_port_display_cfg_t disp_cfg = {.io_handle = io_handle,
                                            .panel_handle = panel_handle,
                                            .buffer_size = DISP_BUF_SIZE,
                                            .double_buffer = false,
                                            .hres = HEIGHT,
                                            .vres = WIDTH,
                                            .monochrome = false,
                                            .color_format =
                                                LV_COLOR_FORMAT_RGB565,
                                            /* Rotation values must be same as
                                            used in esp_lcd for initial settings
                                            of the screen */
                                            .rotation =
                                                {
                                                    .swap_xy = false,
                                                    .mirror_x = false,
                                                    .mirror_y = false,
                                                },
                                            .flags = {
                                                .buff_dma = true,
                                                .buff_spiram = false,
                                                //.sw_rotate =
                                                .swap_bytes = true,
                                            }};

  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
  disp = lvgl_port_add_disp(&disp_cfg);
}

void app_main(void) {

  init_spi_bus();
  init_backlight();
  display_start();

  lv_tick_set_cb(elapsed_time);
  lvgl_app(disp);

  while (1) {
    ESP_LOGW("LVGL", ".");
    uint32_t time_till_next = lv_timer_handler();
    vTaskDelay(time_till_next / portTICK_PERIOD_MS);
  }
}
