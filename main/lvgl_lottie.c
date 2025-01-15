#include "esp_lvgl_port.h"
// #include "lottie_1.h"
#include "lvgl.h"

void lv_example_lottie_1(void) {
  extern const uint8_t lv_example_lottie_approve[];
  extern const size_t lv_example_lottie_approve_size;

  lv_obj_t *lottie = lv_lottie_create(lv_screen_active());
  lv_lottie_set_src_data(lottie, lv_example_lottie_approve,
                         lv_example_lottie_approve_size);

  static uint8_t buf[64 * 64 * 4];
  lv_lottie_set_buffer(lottie, 64, 64, buf);
  lv_obj_center(lottie);
}

static void anim_x_cb(void *var, int32_t v) { lv_obj_set_x(var, v); }

static void anim_size_cb(void *var, int32_t v) { lv_obj_set_size(var, v, v); }

/**
 * Create a playback animation
 */
void lv_example_anim_2(void) {

  lv_obj_t *obj = lv_obj_create(lv_screen_active());
  lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);

  lv_obj_align(obj, LV_ALIGN_LEFT_MID, 10, 0);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, obj);
  lv_anim_set_values(&a, 10, 50);
  lv_anim_set_duration(&a, 1000);
  lv_anim_set_repeat_delay(&a, 500);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

  lv_anim_set_exec_cb(&a, anim_size_cb);
  lv_anim_start(&a);
  lv_anim_set_exec_cb(&a, anim_x_cb);
  lv_anim_set_values(&a, 10, 240);
  lv_anim_start(&a);
}

void lvgl_app(lv_display_t *display) {
  lvgl_port_lock(0);
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000),
                            LV_PART_MAIN);
  lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_sysmon_create(display);
  lv_example_anim_2();
  // lv_example_lottie_1();

  lvgl_port_unlock();
}
