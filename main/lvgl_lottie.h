#include "esp_lvgl_port.h"
#include "lvgl.h"

void lvgl_app(lv_display_t *display) {
  lvgl_port_lock(0);
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000),
                            LV_PART_MAIN);
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello world");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  ///*Create a white label, set its text and align it to the center*/
  lv_sysmon_create(display);

  lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_sysmon_show_performance(display);
  lvgl_port_unlock();
}
