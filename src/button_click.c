#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *time_layer;
static TextLayer *debug_text;
static ActionBarLayer *action_bar;

static GBitmap *action_icon_none;
static GBitmap *action_icon_play;
static GBitmap *action_icon_pause;
static GBitmap *action_icon_settings;
static GBitmap *action_icon_tele;
static GBitmap *action_icon_stop;
void AD_end_timer();

enum {
  kStateOff,
  kStateAuto,
  kStateWaiting,
  kStateTeleOp
} ADTimerState;
int AD_time_value = 0;
const int AD_warning_time = 30;
bool AD_do_auto = true;

//========== Tick Handler ==========
void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  AD_time_value--;
  if (AD_time_value == 0) {
    vibes_long_pulse();
    light_enable_interaction();
    if (ADTimerState == kStateAuto)
      ADTimerState = kStateWaiting;
    if (ADTimerState == kStateTeleOp)
      ADTimerState = kStateOff;
    
  } else if (AD_time_value == AD_warning_time && ADTimerState == kStateTeleOp) {
    vibes_double_pulse();
  } else if ((AD_time_value <= 5 || AD_time_value == 10) && ADTimerState == kStateTeleOp) {
    vibes_short_pulse();
  }
  
  char *time_str = "2:00";

//strftime(time_str, 5, "%M:%S", (const struct *tm){.tm_min = AD_time_value/60, .tm_sec = AD_time_value%60});
  snprintf(time_str, 5, "%i:%i", AD_time_value/60, AD_time_value%60);
  text_layer_set_text(time_layer, time_str);
  
  AD_end_timer();
}

//========== Primary Actions ==========
void AD_start_timer() {
  if(ADTimerState == kStateWaiting || AD_do_auto == false) {
    AD_time_value = 120;
    ADTimerState = kStateTeleOp;
    text_layer_set_text(text_layer, "TeleOp");
    text_layer_set_text(time_layer, "2:00");
    tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
  } else if (ADTimerState == kStateOff) {
    AD_time_value = 30;
    ADTimerState = kStateAuto;
    text_layer_set_text(text_layer, "Autonomous");
    text_layer_set_text(time_layer, "0:30");
    tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
  }
}

void AD_end_timer() {
  tick_timer_service_unsubscribe();
  if (ADTimerState == kStateOff) {
    text_layer_set_text(text_layer, "Stopped");
  } else if (ADTimerState == kStateWaiting) {
    text_layer_set_text(text_layer, "Waiting");
  } else {
    ADTimerState = kStateOff;
    text_layer_set_text(text_layer, "Stopped");
    text_layer_set_text(time_layer, "-:--");
  }
}

//========== Button Callbcks ==========
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  AD_do_auto = !AD_do_auto;
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, AD_do_auto ? action_icon_settings : action_icon_tele);

}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Down");
  if (ADTimerState == kStateOff || ADTimerState == kStateWaiting)
    AD_start_timer();
  else
    AD_end_timer();
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, (ADTimerState == kStateOff || ADTimerState == kStateWaiting) ? action_icon_play : action_icon_stop);
}

//========== Click Provider ==========
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

//========== Window ==========
static void window_load(Window *window) {
  action_icon_play = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLAY);
  action_icon_none = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HOME);
  action_icon_settings = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SETTINGS);
  action_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PAUSE);
  action_icon_tele = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TELE);
  action_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STOP);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  text_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w-20, 30 } });
  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  
  time_layer = text_layer_create((GRect) {.origin = {20, 72}, .size = {bounds.size.w-20, 49}});
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  text_layer_set_text(time_layer, "-:--");
  text_layer_set_text_alignment(time_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
  debug_text = text_layer_create((GRect) { .origin = {0, 30}, .size = { bounds.size.w-20, 20} });
  text_layer_set_text_alignment(debug_text, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(debug_text));
  
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, window);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  
	action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_settings);
	action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_play);
	//action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, action_icon_none);
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

//========== init ==========
static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
	.load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}