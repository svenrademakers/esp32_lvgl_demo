#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "hal/ledc_types.h"
#include "lvgl.h"
#include <stdio.h>

#define DISP_TAG "DISP_INIT"

void init_backlight(void) {
  esp_err_t ret;
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,   // Low-speed mode
      .timer_num = 0,                      // Timer 0
      .duty_resolution = LEDC_TIMER_5_BIT, // 5-bit resolution
      .freq_hz = 24000,                    // Frequency 24 kHz
      .clk_cfg = LEDC_APB_CLK              // Use APB clock source
  };

  ret = ledc_timer_config(&ledc_timer);
  if (ret != ESP_OK) {
    ESP_LOGE(DISP_TAG, "Failed to configure LEDC timer: %s",
             esp_err_to_name(ret));
    return;
  }
  ESP_LOGI(DISP_TAG, "LEDC timer configured");

  // Set up LEDC channel configuration
  ledc_channel_config_t ledc_channel = {
      .gpio_num = 15,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .duty = 32,
  };

  ret = ledc_channel_config(&ledc_channel);
  if (ret != ESP_OK) {
    ESP_LOGE(DISP_TAG, "Failed to configure LEDC channel: %s",
             esp_err_to_name(ret));
    return;
  }
  ESP_LOGI(DISP_TAG, "LEDC channel configured");
}

void app_main(void) {
  ESP_LOGI("SVEN", "TEST");
  // lv_obj_t *lottie = lv_lottie_create(lv_screen_active());
  lv_init();
  init_backlight();
  printf("all done\n");
}
