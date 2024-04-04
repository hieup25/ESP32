/*RESPONSE EXAMPLE API
  {"coord":{"lon":105.8412,"lat":21.0245},"weather":[{"id":802,"main":"Clouds","description":"scattered clouds","icon":"03n"}],
  "base":"stations","main":{"temp":18,"feels_like":17.89,"temp_min":18,"temp_max":18,"pressure":1011,"humidity":78,"sea_level":1011,"grnd_level":1010},
  "visibility":10000,"wind":{"speed":4.67,"deg":136,"gust":9.29},"clouds":{"all":47},"dt":1709472231,
  "sys":{"type":1,"id":9308,"country":"VN","sunrise":1709421336,"sunset":1709463712},"timezone":25200,"id":1581130,"name":"Hanoi","cod":200}
*/
#include <TimeLib.h>
#include <string.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include "icon.h"

#define SCREEN_WIDTH 128      // OLED display width, in pixels
#define SCREEN_HEIGHT 64      // OLED display height, in pixels
#define OLED_RESET     -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C   // See datasheet for Address

//String openWeatherMapApiKey = "REPLACE_WITH_YOUR_OPEN_WEATHER_MAP_API_KEY";
String openWeatherMapApiKey = "f15f17c31e98077943fb7bd900d5facc";
String city = "HaNoi";
const char* ssid = "TP-Link_0D5E";
const char* password = "93853767";

//char test1[] PROGMEM = R"=====({"coord":{"lon":105.8412,"lat":21.0245},"weather":[{"id":802,"main":"Clouds","description":"scattered clouds","icon":"50n"}],"base":"stations","main":{"temp":18,"feels_like":17.89,"temp_min":18,"temp_max":18,"pressure":1011,"humidity":78,"sea_level":1011,"grnd_level":1010},"visibility":10000,"wind":{"speed":4.67,"deg":136,"gust":9.29},"clouds":{"all":47},"dt":1709472231,"sys":{"type":1,"id":9308,"country":"VN","sunrise":1709421336,"sunset":1709463712},"timezone":25200,"id":1581130,"name":"Hanoi","cod":200})=====";
typedef struct pos {
  int16_t x;
  int16_t y;
};
typedef struct pos_element_tab_2 {
  pos icon[2];
  pos days[2];
  pos temp[2];
};
pos_element_tab_2 pos_el_tab2 = {
  {{15, 10}, {80, 10}},
  {{25, 0}, {90, 0}},
  {{25, 43}, {90, 43}}
};

typedef struct content_tab1 {
  bool          is_data;
  char          main[10]; // doc["weather"][0]["main"]
  char          icon[5];  // doc["weather"][0]["icon"]
  unsigned char temp;     // doc["main"]["temp"]
  unsigned char humi;     // doc["main"]["humidity"]
  unsigned long dt;       // doc["dt"] <=> time stamp
};
typedef struct content_tab2 {
  bool          is_data;
  char          arr_icon[2][4];
  unsigned char arr_temp[2];
  unsigned long arr_dt[2];
};
content_tab1 tab1;
content_tab2 tab2;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long previousMillis_updateWeather = 20000;
unsigned long previousMillis_swTab = 5000;
const long interval_updateWeather = 20000;
const long interval_swTab = 5000;
byte tab_current = 2; // 1 and 2
void setup() {
  Serial.begin(115200);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Camera at address: http://");
  Serial.println(WiFi.localIP());
  // Default
  tab1.is_data = false;
  tab2.is_data = false;
}
void timestamp_to_string(unsigned long ts, char* out) {
  time_t timestamp = ts + (7 * 3600); // múi giờ GMT+7
  struct tm *timeinfo;
  timeinfo = localtime(&timestamp);
  char buffer[30];
  strftime(buffer, 20, "%H:%M %d-%m %a", timeinfo);
  strcpy(out, buffer);
}

void draw_icon_tab1() {
  if (strncmp(tab1.icon, "01d", 3) == 0) {
    // show clear sky day
    display.drawBitmap(75, 5, clear_sky_d_bmp, W_CLEAR_SKY_D, H_CLEAR_SKY_D, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "01n", 3) == 0) {
    // show clear sky night
    display.drawBitmap(75, 5, clear_sky_n_bmp, W_CLEAR_SKY_N, H_CLEAR_SKY_N, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "02d", 3) == 0) {
    // show few clouds day
    display.drawBitmap(75, 5, few_cloud_d_bmp, W_FEW_COULD_D, H_FEW_COULD_D, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "02n", 3) == 0) {
    // show few clouds night
    display.drawBitmap(75, 5, few_cloud_n_bmp, W_FEW_COULD_N, H_FEW_COULD_N, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "03", 2) == 0) {
    // show few clouds day and night
    display.drawBitmap(68, 5, scattered_cloud_bmp, W_SCATTERED_COULD, H_SCATTERED_COULD, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "04", 2) == 0) {
    // show broken clouds day and night
    display.drawBitmap(75, 5, broken_cloud_bmp, W_BROKEN_COULD, H_BROKEN_COULD, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "09", 2) == 0) {
    // show shower rain day and night
    display.drawBitmap(75, 5, shower_rain_bmp, W_SHOWER_RAIN, H_SHOWER_RAIN, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "10d", 3) == 0) {
    // show shower rain day
    display.drawBitmap(75, 5, rain_d_bmp, W_RAIN_D, H_RAIN_D, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "10n", 3) == 0) {
    // show shower rain night
    display.drawBitmap(75, 5, rain_n_bmp, W_RAIN_N, H_RAIN_N, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "11", 2) == 0) {
    // show shower thunder day and night
    display.drawBitmap(75, 5, thunder_bmp, W_THUNDER, H_THUNDER, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "13", 2) == 0) {
    // show shower snow day and night
    display.drawBitmap(75, 5, snow_bmp, W_SNOW, H_SNOW, SSD1306_WHITE);
  } else if (strncmp(tab1.icon, "50", 2) == 0) {
    // show shower mist day and night
    display.drawBitmap(75, 5, mist_bmp, W_MIST, H_MIST, SSD1306_WHITE);
  }
}
void draw_tab2(int16_t index) {
  // show day
  char out_time[20] = {0};
  int16_t x_day = pos_el_tab2.days[index].x;
  int16_t y_day = pos_el_tab2.days[index].y;
  timestamp_to_string(tab2.arr_dt[index], out_time);
  char _day[4] = {0};
  sscanf(out_time, "%*s %*s %s", _day);
  display.setTextSize(1);
  display.setCursor(x_day, y_day);
  display.println(_day);
  Serial.println(_day);
  // show temp
  int16_t x_temp = pos_el_tab2.temp[index].x;
  int16_t y_temp = pos_el_tab2.temp[index].y;
  display.setTextSize(1);
  display.setCursor(x_temp, y_temp);
  display.println(tab2.arr_temp[index]);
  display.drawCircle(x_temp + 15, y_temp, 2, 1);
  display.setCursor(x_temp + 15 + 3 + 3, y_temp);
  display.println("C");
  // show icon
  const char* _icon = tab2.arr_icon[index];
  int16_t x_icon = pos_el_tab2.icon[index].x;
  int16_t y_icon = pos_el_tab2.icon[index].y;
  //  Serial.println(String(_icon) + String(" ") + String(x_icon) + String(" ") + String(y_icon));
  if (strncmp(_icon, "01d", 3) == 0) {
    // show clear sky day
    display.drawBitmap(x_icon, y_icon, clear_sky_d_bmp, W_CLEAR_SKY_D, H_CLEAR_SKY_D, SSD1306_WHITE);
  } else if (strncmp(_icon, "01n", 3) == 0) {
    // show clear sky night
    display.drawBitmap(x_icon, y_icon, clear_sky_n_bmp, W_CLEAR_SKY_N, H_CLEAR_SKY_N, SSD1306_WHITE);
  } else if (strncmp(_icon, "02d", 3) == 0) {
    // show few clouds day
    display.drawBitmap(x_icon, y_icon, few_cloud_d_bmp, W_FEW_COULD_D, H_FEW_COULD_D, SSD1306_WHITE);
  } else if (strncmp(_icon, "02n", 3) == 0) {
    // show few clouds night
    display.drawBitmap(x_icon, y_icon, few_cloud_n_bmp, W_FEW_COULD_N, H_FEW_COULD_N, SSD1306_WHITE);
  } else if (strncmp(_icon, "03", 2) == 0) {
    // show few clouds day and night
    display.drawBitmap(x_icon, y_icon, scattered_cloud_bmp, W_SCATTERED_COULD, H_SCATTERED_COULD, SSD1306_WHITE);
  } else if (strncmp(_icon, "04", 2) == 0) {
    // show broken clouds day and night
    display.drawBitmap(x_icon, y_icon, broken_cloud_bmp, W_BROKEN_COULD, H_BROKEN_COULD, SSD1306_WHITE);
  } else if (strncmp(_icon, "09", 2) == 0) {
    // show shower rain day and night
    display.drawBitmap(x_icon, y_icon, shower_rain_bmp, W_SHOWER_RAIN, H_SHOWER_RAIN, SSD1306_WHITE);
  } else if (strncmp(_icon, "10d", 3) == 0) {
    // show shower rain day
    display.drawBitmap(x_icon, y_icon, rain_d_bmp, W_RAIN_D, H_RAIN_D, SSD1306_WHITE);
  } else if (strncmp(_icon, "10n", 3) == 0) {
    // show shower rain night
    display.drawBitmap(x_icon, y_icon, rain_n_bmp, W_RAIN_N, H_RAIN_N, SSD1306_WHITE);
  } else if (strncmp(_icon, "11", 2) == 0) {
    // show shower thunder day and night
    display.drawBitmap(x_icon, y_icon, thunder_bmp, W_THUNDER, H_THUNDER, SSD1306_WHITE);
  } else if (strncmp(_icon, "13", 2) == 0) {
    // show shower snow day and night
    display.drawBitmap(x_icon, y_icon, snow_bmp, W_SNOW, H_SNOW, SSD1306_WHITE);
  } else if (strncmp(_icon, "50", 2) == 0) {
    // show shower mist day and night
    display.drawBitmap(x_icon, y_icon, mist_bmp, W_MIST, H_MIST, SSD1306_WHITE);
  }
}
void showTab1() {
  if (tab_current == 1 || !tab1.is_data) return;
  display.clearDisplay();
  /*Title*/
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setCursor(0, 0);             // Start at top-left corner
  display.println(F("@HieuP25"));
  /*Show Time - Tab*/
  char out_time[20] = {0};
  timestamp_to_string(tab1.dt, out_time);
  char _time[6] = {0};
  char _date[6] = {0};
  //  Serial.println(out_time);
  sscanf(out_time, "%s %s", _time, _date);
  // show time
  display.setTextSize(1);
  display.drawFastHLine(0, 52, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.println(_time);
  display.setCursor(SCREEN_WIDTH - strlen(_date) * 6, 56);
  display.println(_date);
  // show tab
  display.fillCircle(SCREEN_WIDTH / 2 - 5, 60, 2, SSD1306_WHITE);
  display.drawCircle(SCREEN_WIDTH / 2 + 2, 60, 2, SSD1306_WHITE);
  /*Show Temp*/
  display.setTextSize(3);
  display.setCursor(5, 15);
  display.println(tab1.temp);
  display.drawCircle(45, 16, 3, 1);
  /*Show main => weather[0]["main"]*/
  display.setTextSize(1);
  display.setCursor(5, 41);
  display.println(tab1.main);
  // drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color)
  /*Show Icon - Humi - City*/
  // show Icon
  draw_icon_tab1();
  // show City
  display.setTextSize(1);
  char city_humi[15] = {0};
  snprintf(city_humi, 15, "%d%% - %s", tab1.humi, "Ha Noi"); // maybe change City
  display.setCursor(128 - strlen(city_humi) * 6, 41);
  display.println(city_humi);
  // DONE - SHOW
  display.display();

}
void showTab2() {
  if (tab_current == 2 || !tab2.is_data) return;
  display.clearDisplay();
  //  /*Title*/
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setTextSize(1);             // Normal 1:1 pixel scale
  //  display.setCursor(0, 0);             // Start at top-left corner
  //  display.println(F("@HieuP25"));
  /*Show Time - Tab*/
  char out_time[20] = {0};
  timestamp_to_string(tab1.dt, out_time);
  char _time[6] = {0};
  char _date[6] = {0};
  //  Serial.println(out_time);
  sscanf(out_time, "%s %s", _time, _date);
  // show time
  display.setTextSize(1);
  display.drawFastHLine(0, 52, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.println(_time);
  display.setCursor(SCREEN_WIDTH - strlen(_date) * 6, 56);
  display.println(_date);
  // show tab
  display.fillCircle(SCREEN_WIDTH / 2 + 2, 60, 2, SSD1306_WHITE);
  display.drawCircle(SCREEN_WIDTH / 2 - 5, 60, 2, SSD1306_WHITE);
  // Show forecast weather
  for (byte i = 0; i < 2; i++) {
    draw_tab2(i);
  }
  // DONE - SHOW
  display.display();
}
void updateWeather(DynamicJsonDocument& doc, byte num_tab) { // value num_tab 1 or 2
  if (num_tab == 1) {
    // Update tab1
    strncpy(tab1.main, doc["weather"][0]["main"].as<String>().c_str(), sizeof(tab1.main));
    strncpy(tab1.icon, doc["weather"][0]["icon"].as<String>().c_str(), sizeof(tab1.icon));
    tab1.temp = (int) doc["main"]["temp"].as<double>();
    tab1.humi = (int) doc["main"]["humidity"].as<double>();
    tab1.dt   = doc["dt"].as<unsigned long>();
    tab1.is_data = true;
  } else if (num_tab == 2) {
    // Update tab2
    // API sẽ lấy giá trị dự báo của 2 ngày,
    // Mỗi ngày chỉ lấy 1 thời điểm(thời gian) đầu tiên được dự báo (trừ 00:00:00)
    char first_day[11] = {0};
    sscanf(doc["list"][0]["dt_txt"].as<String>().c_str(), "%s %*s", first_day);
    strcpy(tab2.arr_icon[0], doc["list"][0]["weather"][0]["icon"].as<String>().c_str());
    tab2.arr_temp[0] = (int)doc["list"][0]["main"]["temp"].as<double>();
    tab2.arr_dt[0] = doc["list"][0]["dt"].as<unsigned long>();
    //    Serial.print("t1: ");
    //    Serial.println(tab2.arr_dt[0]);
    //    Serial.print("First day: ");
    //    Serial.println(first_day);
    int list_size = doc["list"].size();
    for (int i = 1; i < list_size; i++) {
      char _t_day[11] = {0};
      char _t_time[9] = {0};
      sscanf(doc["list"][i]["dt_txt"].as<String>().c_str(), "%s %s", _t_day, _t_time);
      Serial.println(_t_day);
      Serial.println(_t_time);
      if ((strcmp(first_day, _t_day) != 0) && (strcmp(_t_time, "00:00:00") != 0)) {
        //        Serial.print("Second day: ");
        //        Serial.println(_t_day);
        strcpy(tab2.arr_icon[1], doc["list"][i]["weather"][0]["icon"].as<String>().c_str());
        tab2.arr_temp[1] = (int)doc["list"][i]["main"]["temp"].as<double>();
        tab2.arr_dt[1] = doc["list"][i]["dt"].as<unsigned long>();
        //        Serial.print("t2: ");
        //        Serial.println(tab2.arr_dt[1]);
        break;
      }
    }
    tab2.is_data = true;
  }
  doc.clear();
}
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
void loop() {
  unsigned long currentMillis = millis();
  // Kiểm tra nếu đã đủ thời gian timeout
  if (currentMillis - previousMillis_updateWeather >= interval_updateWeather) {
    previousMillis_updateWeather = currentMillis;
    {
      Serial.println(F("Get Weather Current!"));
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city  + "&appid=" + openWeatherMapApiKey + "&units=metric";
      String weather_current = httpGETRequest(serverPath.c_str());
      Serial.println(F("GET weather current:"));
      //      Serial.println(weather_current);
      // TODO get API
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, weather_current);
      if (error) {
        Serial.println(String(F("deserializeJson() (weather_current) failed: ")) + String(error.c_str()));
      } else {
        updateWeather(doc, 1);
      }
    }
    {
      Serial.println(F("Get Weather Forecast!"));
      String serverPath = "http://api.openweathermap.org/data/2.5/forecast?q=" + city  + "&appid=" + openWeatherMapApiKey + "&units=metric" + "&cnt=16";
      String weather_forecast = httpGETRequest(serverPath.c_str());
      Serial.println(F("GET weather forecast:"));
      //      Serial.println(weather_forecast);
      // TODO get API
      DynamicJsonDocument doc(10000);
      DeserializationError error = deserializeJson(doc, weather_forecast);
      if (error) {
        Serial.println(String(F("deserializeJson() (weather_forecast) failed: ")) + String(error.c_str()));
      } else {
        updateWeather(doc, 2);
      }
    }
  }
  else if (currentMillis - previousMillis_swTab >= interval_swTab) {
    previousMillis_swTab = currentMillis;
    if (tab_current == 1) {
      Serial.println(F("Auto Show tab 2"));
      showTab2();
      tab_current = 2;
    } else {
      Serial.println(F("Auto Show tab 1"));
      showTab1();
      tab_current = 1;
    }
  }
}
