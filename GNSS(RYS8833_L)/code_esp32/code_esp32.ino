#include <string.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "TP-Link_0D5E"
#define WIFI_PASSWORD "93853767"
//#define WIFI_SSID "Galaxy S23 Ultra 0B65"
//#define WIFI_PASSWORD "12345678"

// Thay đổi API Key cho đúng project của bạn
/* 2. Define the API Key */
#define API_KEY "AIzaSyA60XyWnId7purPGY6fNAdA5z1ijoJ9yrk"

/* 3. Define the RTDB URL, thay đổi URL cho đúng project của bạn */
#define DATABASE_URL "https://esp32-5be65-default-rtdb.firebaseio.com/"


#define UNDEFINE "undefine"
#define GET_LOCATION_INTERVAL 20000

typedef struct
{
  char time[10];        // UTC. Format: hhmmss.ss
  char date[10];        // UTC. Format: ddmmyy
  char lat[15];         // Latitude
  char lng[15];         // Longitude
  char speed[11];        // knot => to km/h

} location_info_t;
//char lat_old[15] = {0};         // Latitude
//char lng_old[15] = {0};         // Longitude
// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;
bool signupOK = false;

void setup()
{
  // Khởi tạo serial để đọc response từ module GNSS [test]
  Serial.begin(115200);
  // Khởi tạo serial2 để gửi lệnh đến module GNSS
  Serial2.begin(115200);
  while (!Serial || !Serial2) {
    delay(200);
  };
  // Setup wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.reconnectNetwork(true);
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("signUp ok");
    signupOK = true;
  }
  else
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  Firebase.begin(&config, &auth);
  config.timeout.serverResponse = 10 * 1000;
  // gửi lệnh để module chạy GNSS
  Serial2.println("@GSTP");
  delay(100);
  //  Serial2.println("@GNS 47");
  Serial2.println("@GNS 47");
  delay(100);
  //  Serial2.println("@GCD"); // COLD Start
  Serial2.println("@GSR"); // HOT start
  delay(100);
}
bool getLatLng (const String raw_data, location_info_t &output) {
  char line_str[raw_data.length() + 1];
  strcpy(line_str, raw_data.c_str());
  // check data valid?
  char is_data_valid;
  sscanf(line_str + 1, "%*[^,],%c", &is_data_valid);
  if (is_data_valid != 'A') // invalid
  {
    Serial.printf("%s [%c]", "Data invalid", is_data_valid);
    return false;
  }
  // OK, dữ liệu đúng định dạng, bắt đầu lấy dữ liệu
  char latDir[2] = {0}; // latDirection
  char lngDir[2] = {0}; // lngDirection
  sscanf(line_str + 1, "%10[^,],%*c,%14[^,],%c,%14[^,],%c,%10[^,],%*[^,],%[^,],%*[^\n]", output.time, output.lat, latDir, output.lng, lngDir, output.speed, output.date);
  // Nếu vị trí trùng lặp => không gửi lại, hoặc sai số nhỏ
  //  int newLength_lat = strlen(output.lat) - 2; // Lấy độ dài mới sau khi loại bỏ 2 số cuối
  //  char lat_cmp[newLength_lat + 1]; // Tạo chuỗi mới với độ dài mới
  //  snprintf(lat_cmp, sizeof(lat_cmp), "%.*s", newLength_lat, output.lat);
  //
  //  int newLength_lng = strlen(output.lng) - 2; // Lấy độ dài mới sau khi loại bỏ 2 số cuối
  //  char lng_cmp[newLength_lng + 1]; // Tạo chuỗi mới với độ dài mới
  //  snprintf(lng_cmp, sizeof(lng_cmp), "%.*s", newLength_lng, output.lng);
  //
  //  //  Serial.printf("lat_cmp: %s, lng_cmp: %s\n", lat_cmp, lng_cmp);
  //  if ((strcmp(lat_cmp, lat_old) == 0) && (strcmp(lng_cmp, lng_old) == 0)) {
  //    Serial.printf("%s cmp[%s, %s] | old[%s, %s]\n", "coordinates duplicate", lat_cmp, lng_cmp, lat_old, lng_old);
  //    return false;
  //  } else {
  //    strcpy(lat_old, lat_cmp);
  //    strcpy(lng_old, lng_cmp);
  //  }
  // Đổi đơn vị tốc độ từ knot sang km/h
  sprintf(output.speed, "%.1f", atof(output.speed) * 1.852);
  // Nên check thêm điều kiện nếu trường dữ liệu nào trống thì đặt 1 giá trị mặc định
  if (!output.time) strcpy(output.time, UNDEFINE);
  (!output.lat) ? strcpy(output.lat, UNDEFINE) : strcat(output.lat, ("," + String(latDir)).c_str());
  (!output.lng) ? strcpy(output.lng, UNDEFINE) : strcat(output.lng, ("," + String(lngDir)).c_str());
  if (!output.date) strcpy(output.date, UNDEFINE);
  return true;
}
void loop()
{
  //      if (Serial2.available()) {
  //          Serial.print((char)Serial2.read());
  //      }
  if (Serial2.find("$GNRMC"))
  {
    // Đọc các ký tự tiếp theo đến ký tự new line
    String line = Serial2.readStringUntil('\n');
    Serial.println("$GNRMC" + line);
    location_info_t output = {0};
    if ((millis() - sendDataPrevMillis > GET_LOCATION_INTERVAL || sendDataPrevMillis == 0) && // timeout GET_LOCATION_INTERVAL (s)
        getLatLng(line, output) && // data valid
        Firebase.ready() && // firebase ok
        signupOK)
    {
      sendDataPrevMillis = millis();
      Serial.printf("time: %s\nlat: %s\nlng: %s\nspeed: %s\ndate: %s\n",
                    output.time, output.lat, output.lng, output.speed, output.date);
      FirebaseJson json;
      json.set("time", output.time);
      json.set("date", output.date);
      json.set("speed", output.speed);
      json.set("lat", output.lat);
      json.set("lng", output.lng);
      if (count == 0) { // remove
        Serial.printf("Remove node 'location'... %s\n", Firebase.RTDB.deleteNode(&fbdo, "/location") ? "ok" : fbdo.errorReason().c_str());
      }
      Serial.printf("Set node 'location'... %s\n", Firebase.RTDB.set(&fbdo, "/location/" + String(count), &json) ? "ok" : fbdo.errorReason().c_str());
      count++;
    }
  }
}
