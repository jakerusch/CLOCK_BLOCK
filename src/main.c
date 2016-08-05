#include <pebble.h>

#define KEY_TEMP 0

static Window *s_main_window;
static GFont s_font, s_font_small;
static TextLayer *s_hour_layer, *s_minute_layer, *s_temp_layer;
static Layer *s_batt_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
BatteryChargeState battery_state;

static void batt_update_proc(Layer *layer, GContext *ctx) {
  ////////////////////
  // battery circle //
  ////////////////////
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 20, DEG_TO_TRIGANGLE(360-(battery_state.charge_percent*3.6)), DEG_TO_TRIGANGLE(360));    
}

//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  
  //////////////////////////
  // set background color //
  //////////////////////////
  window_set_background_color(window, GColorBlack); // default GColorWhite
  
  ///////////////////////
  // get window bounds //
  ///////////////////////
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  ///////////////////////
  // load custom fonts //
  ///////////////////////
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BRACIOLA_STENCIL_FONT_72));
  s_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BRACIOLA_BOLD_FONT_18));
  
  ///////////////////////
  // load minute layer //
  ///////////////////////
  s_minute_layer = text_layer_create(GRect(0, bounds.size.h/2-28, bounds.size.w, bounds.size.h/2-10)); // adjusted to center on screen
  text_layer_set_background_color(s_minute_layer, GColorClear); // default GColorWhite
  text_layer_set_text_color(s_minute_layer, GColorWhite); // default GColorBlack
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter); // default GTextAlignmentLeft
  text_layer_set_font(s_minute_layer, s_font); // load custom font
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer)); // add layer to window
  
  /////////////////////
  // load hour layer //
  /////////////////////
  s_hour_layer = text_layer_create(GRect(28, 11, bounds.size.w-60, bounds.size.h/2-10)); // adjusted to center on screen
  text_layer_set_background_color(s_hour_layer, GColorClear); // default GColorWhite
  text_layer_set_text_color(s_hour_layer, GColorWhite); // default GColorBlack
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentRight); // default GTextAlignmentLeft
  text_layer_set_font(s_hour_layer, s_font); // load custom font
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer)); // add layer to window
  
  /////////////////////
  // load temp layer //
  /////////////////////
  s_temp_layer = text_layer_create(GRect(bounds.size.w/2, 0, bounds.size.w/2, 20));
  text_layer_set_background_color(s_temp_layer, GColorClear); // default GColorWhite
  text_layer_set_text_color(s_temp_layer, GColorWhite); // default GColorBlack
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentRight); // default GTextAlignmentLeft
  text_layer_set_font(s_temp_layer, s_font_small); // load custom font
  text_layer_set_text(s_temp_layer, "-");
  layer_add_child(window_layer, text_layer_get_layer(s_temp_layer)); // add layer to window  
  
  ////////////////////////
  // load battery layer //
  ////////////////////////
  s_batt_layer = layer_create(GRect(5, 5, 14, 14));
  layer_set_update_proc(s_batt_layer, batt_update_proc);
  layer_add_child(window_get_root_layer(window), s_batt_layer); // add layer to window    
  
  ///////////////////
  // charging icon //
  ///////////////////
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIGHTENING_WHITE);
  s_bitmap_layer = bitmap_layer_create(GRect(20, 5, 14, 14));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap); 
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));  
}

//////////////////////////////
// update time every minute //
//////////////////////////////
static void update_time() {
  
  ////////////////////////
  // get a tm strucutre //
  ////////////////////////
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  ///////////////////////////////////////////
  // write the current hours into a buffer //
  ///////////////////////////////////////////
  static char s_hour_buffer[8];
  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%k" : "%l", tick_time);
  // %k = Hour 0..23
  // %l = Hour 0..12
  
  ///////////////////////////////
  // write the current minutes //
  ///////////////////////////////
  static char s_minute_buffer[8];
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", tick_time);
  // %M = Minute (00..59)
  
  ////////////////
  // write hour //
  ////////////////
  text_layer_set_text(s_hour_layer, s_hour_buffer); // allows leading 0 on double digit numbers, handled by %l strftime
  
  ////////////////////////////////
  // write minute to text layer //
  ////////////////////////////////
  text_layer_set_text(s_minute_layer, s_minute_buffer);
}

/////////////////////////////////////////////
// allows app to subscribe to time updates //
/////////////////////////////////////////////
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  /////////////////////////////////////////
  // Get weather update every 30 minutes //
  /////////////////////////////////////////
  if(tick_time->tm_min % 30 == 0) {
    
    //////////////////////
    // Begin dictionary //
    //////////////////////
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    //////////////////////////
    // Add a key-value pair //
    //////////////////////////
    dict_write_uint8(iter, 0, 0);

    ///////////////////////
    // Send the message! //
    ///////////////////////
    app_message_outbox_send();
  }  
}

static void battery_state_handler(BatteryChargeState charge) {
  battery_state = charge;
  layer_mark_dirty(s_batt_layer);
    
  if (charge.is_charging) {
    layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer), false);
  } else {
    layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer), true);
  }
}

////////////////////////
// main window unload //
////////////////////////
static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_font);
  fonts_unload_custom_font(s_font_small);
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_temp_layer);
  layer_destroy(s_batt_layer);
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

///////////////////////
// for weather calls //
///////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  ////////////////////////////////
  // Store incoming information //
  ////////////////////////////////
  static char temp_buf[8];
  static char temp_layer_buf[32];

  //////////////////////////
  // Read tuples for data //
  //////////////////////////
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMP);

  //////////////////////////////////////
  // If all data is available, use it //
  //////////////////////////////////////
  if(temp_tuple) {
    snprintf(temp_buf, sizeof(temp_buf), "%d°", (int)temp_tuple->value->int32);

    /////////////////
    // temperature //
    /////////////////
    snprintf(temp_layer_buf, sizeof(temp_layer_buf), "%s", temp_buf);
    text_layer_set_text(s_temp_layer, temp_layer_buf);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

////////////////////
// initialize app //
////////////////////
void init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });  

  window_stack_push(s_main_window, true);
  
  ////////////////////////////////
  // register for clock updates //
  ////////////////////////////////
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  /////////////////////////////////////////
  // register with Battery State Service //
  /////////////////////////////////////////
  battery_state_service_subscribe(battery_state_handler);
  battery_state_handler(battery_state_service_peek());
  
  ////////////////////////////////////////////////////
  // Make sure the time is displayed from the start //
  ////////////////////////////////////////////////////
  update_time();  
  
  ////////////////////////////////
  // Register weather callbacks //
  ////////////////////////////////
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  
  ///////////////////////////////////////////
  // Open AppMessage for weather callbacks //
  ///////////////////////////////////////////
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);    
}

///////////////////////
// de-initialize app //
///////////////////////
void deinit(void) {
  window_destroy(s_main_window);
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
}

/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
}