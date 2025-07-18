//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//            Phật phù hộ, không bao giờ BUG
//                Nam mô a di đà phật
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DESIGN BY MENSON - Huynh Manh Sang 
// PHONE: 0869053063
// Facebook: https://www.facebook.com/profile.php?id=100076267646838&mibextid=ZbWKwL (Huynh Sang)

// ====================================================== KHAI BÁO THƯ VIỆN ======================================================================================
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <HardwareSerial.h> 
#include <SPI.h>
#include "lvgl.h"
#include "ui.h"
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 480;
enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
static lv_color_t buf [SCREENBUFFER_SIZE_PIXELS];
TFT_eSPI tft = TFT_eSPI( screenWidth, screenHeight ); 

// ========================================================= KHAI BÁO CHÂN ======================================================================================
#define ESP_RX 21
#define ESP_TX 22
#define XPT2046_IRQ 36 
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass tsSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// ========================================================= KHAI BÁO BIẾN ======================================================================================
const char* ssid = ".....";
const char* password = ".....";
const char* Gemini_Token = "TỰ ĐIỀN KEY"; // lên gemini lấy API key 
const char* Gemini_Max_Tokens = "100";
String res = "";
String resp_text = "";
uint16_t touchScreenMinimumX = 180, touchScreenMaximumX = 3770, touchScreenMinimumY = 240,touchScreenMaximumY = 3870; 
unsigned long timer_change_sceen = millis();
unsigned long vla_timer_change_sceen = 3000;
bool change_sceen = false;

// ========================================================= HÀM GỬI DỮ LIỆU DISP LVGL TỚI TFT ===================================================================
void my_disp_flush (lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    if (LV_COLOR_16_SWAP) {
        size_t len = lv_area_get_size( area );
        lv_draw_sw_rgb565_swap( pixelmap, len );
    }

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( (uint16_t*) pixelmap, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

// =========================================================== HÀM ĐỌC TOUCH CẢM ỨNG ======================================================================================
void my_touch_read (lv_indev_t *indev_drv, lv_indev_data_t * data)
{
    if(ts.touched())
    {
        TS_Point p = ts.getPoint();
        if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
        data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, screenWidth, 1);
        data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,screenHeight); 
        data->state = LV_INDEV_STATE_PR;

    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}
static uint32_t my_tick_get_cb (void) { return millis(); }

// ========================================================= HÀM CHUYỂN MÀN HÌNH ======================================================================================
void next_logo(){
    if((millis()-timer_change_sceen>=vla_timer_change_sceen) && (change_sceen == false))
{
timer_change_sceen = millis();
change_sceen = true;
_ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_NONE, 0, 2000, &ui_Screen2_screen_init);
} 
}

// ========================================================= SETUP ======================================================================================
void check_dec_obj(lv_event_t * e);
void setup (){
    Serial.begin( 115200 );
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
    delay(500);
    lv_init();
    tsSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); 
    ts.begin(tsSpi);    
    ts.setRotation(2);   
    tft.begin();        
    tft.setRotation(0); 
    static lv_disp_t* disp;
    disp = lv_display_create( screenWidth, screenHeight );
    lv_display_set_buffers( disp, buf, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL );
    lv_display_set_flush_cb( disp, my_disp_flush );
    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touch_read);
    lv_tick_set_cb( my_tick_get_cb );
    ui_init();
    lv_obj_t* button_labels[1] = {ui_Button3
    };
    lv_obj_add_event_cb(button_labels[0], check_dec_obj, LV_EVENT_ALL, NULL);
// Chỗ này nếu muốn khai báo thêm các nút nhấn thì thêm vào nhãn button_labels + dùng hàm for để khai báo sẽ tiện hơn
}

void send_get(String qinput){
  resp_text = "";
  res = qinput;
  int len = res.length();
  res = "\"" + res + "\"";
  HTTPClient https;
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + (String)Gemini_Token)) {  // HTTPS

    https.addHeader("Content-Type", "application/json");
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":" + res + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)Gemini_Max_Tokens + "}}");

    int httpCode = https.POST(payload);

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = https.getString();
      DynamicJsonDocument doc(1024);

      deserializeJson(doc, payload);

      String Answer = doc["candidates"][0]["content"]["parts"][0]["text"];

      Answer.trim();
      resp_text = Answer;
    } else {
    }
    https.end();
  } else {
  }
  res = "";
}

void check_dec_obj(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);\
  lv_obj_t* button_check_labels[1] = {ui_Button3
  };
// Chỗ này nếu muốn thêm các nút nhấn thì thêm vào nhãn button_labels + dùng hàm for để khai báo sẽ tiện hơn
    if (obj == button_check_labels[0] && code == LV_EVENT_CLICKED) {
    send_get(String(lv_textarea_get_text(ui_TextArea8)));
    lv_textarea_set_text(ui_TextArea7, lv_textarea_get_text(ui_TextArea8));
    lv_textarea_set_text(ui_TextArea8, "");
    delay(200);
    lv_textarea_set_text(ui_TextArea1, resp_text.c_str());
     }
  }

// ========================================================= LOOP ======================================================================================
void loop ()
{
    lv_timer_handler();  
    next_logo();
  }