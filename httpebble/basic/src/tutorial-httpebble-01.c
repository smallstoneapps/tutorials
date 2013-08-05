#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"

#if ANDROID
#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x10, 0x34, 0xBF, 0xBE, 0x12, 0x97 }
#else
#define MY_UUID HTTP_UUID
#endif

#define HTTP_COOKIE 6550

PBL_APP_INFO(MY_UUID, "McDonalds Finder", "Matthew Tole", 1, 0,  DEFAULT_MENU_ICON, APP_INFO_STANDARD_APP);

void handle_init(AppContextRef ctx);
void handle_deinit(AppContextRef ctx);
void load_bitmaps();
void unload_bitmaps();
void click_config_provider(ClickConfig **config, Window *window);
void button_clicked(ClickRecognizerRef recognizer, Window *window);
void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context);
void http_failure(int32_t request_id, int http_status, void* context);
void http_location(float latitude, float longitude, float altitude, float accuracy, void* context);
void show_message(int code);

Window window;
BitmapLayer layer_logo;
TextLayer layer_middle_text;
TextLayer layer_bottom_text;
HeapBitmap bmp_logo;
bool working = false;
static char error_msg[] = "HTTP Error 00000";

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .messaging_info = {
      .buffer_sizes = {
        .inbound = 124,
        .outbound = 256,
      }
    }
  };

  app_event_loop(params, &handlers);
}

void button_clicked(ClickRecognizerRef recognizer, Window *window) {
  if (working) {
    return;
  }

  working = true;
  text_layer_set_text(&layer_bottom_text, "Working...");

  http_location_request();
}

void show_message(int code) {
  snprintf(error_msg, sizeof(error_msg), "HTTP Error %d", code);
  text_layer_set_text(&layer_bottom_text, error_msg);
}

void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context) {
  if (request_id != HTTP_COOKIE) {
    return;
  }

  Tuple* tuple = dict_find(received, 1);
  text_layer_set_text(&layer_bottom_text, tuple->value->cstring);
  working = false;
}

void http_failure(int32_t request_id, int http_status, void* context) {
  if (request_id != HTTP_COOKIE) {
    return;
  }
  show_message(http_status > 1000 ? http_status - 1000 : http_status);
}

void http_location(float latitude, float longitude, float altitude, float accuracy, void* context) {
  text_layer_set_text(&layer_bottom_text, "Found You!");

  DictionaryIterator* dict;
  HTTPResult  result = http_out_get("http://api.pblweb.com/mcdonalds/v1/nearest.php", HTTP_COOKIE, &dict);
  if (result != HTTP_OK) {
    show_message(result);
    working = false;
    return;
  }

  dict_write_int32(dict, 1, latitude * 10000);
  dict_write_int32(dict, 2, longitude * 10000);

  result = http_out_send();
  if (result != HTTP_OK) {
    show_message(result);
    working = false;
    return;
  }
}

void handle_init(AppContextRef ctx) {
  resource_init_current_app(&APP_RESOURCES);
  load_bitmaps();

  http_set_app_id(655092678);

  http_register_callbacks((HTTPCallbacks) {
    .success = http_success,
    .failure = http_failure,
    .location = http_location
  }, NULL);

  window_init(&window, "McDonalds Finder");
  window_stack_push(&window, true);
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);

  bitmap_layer_init(&layer_logo, GRect(0, 10, 144, 50));
  bitmap_layer_set_alignment(&layer_logo, GAlignCenter);
  bitmap_layer_set_bitmap(&layer_logo, &bmp_logo.bmp);
  layer_add_child(&window.layer, &layer_logo.layer);

  text_layer_init(&layer_middle_text, GRect(0, 60, 144, 26));
  text_layer_set_text_color(&layer_middle_text, GColorBlack);
  text_layer_set_background_color(&layer_middle_text, GColorClear);
  text_layer_set_font(&layer_middle_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&layer_middle_text, GTextAlignmentCenter);
  text_layer_set_text(&layer_middle_text, "Press button --->");
  layer_add_child(&window.layer, &layer_middle_text.layer);

  text_layer_init(&layer_bottom_text, GRect(0, 120, 144, 48));
  text_layer_set_text_color(&layer_bottom_text, GColorWhite);
  text_layer_set_background_color(&layer_bottom_text, GColorBlack);
  text_layer_set_font(&layer_bottom_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&layer_bottom_text, GTextAlignmentCenter);
  layer_add_child(&window.layer, &layer_bottom_text.layer);
}

void handle_deinit(AppContextRef ctx) {
  unload_bitmaps();
}

void load_bitmaps() {
  heap_bitmap_init(&bmp_logo, RESOURCE_ID_MCDONALDS_LOGO);
}

void unload_bitmaps() {
  heap_bitmap_deinit(&bmp_logo);
}

void click_config_provider(ClickConfig **config, Window *window) {
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) button_clicked;
}