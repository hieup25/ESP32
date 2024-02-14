#include <string.h>

typedef struct
{
  char time[10];        // UTC. Format: hhmmss.ss
  char date[10];        // UTC. Format: ddmmyy
  char lat[11];         // Latitude
  char lng[11];         // Longitude
  char speed[11];        // knot => to km/h

} location_info_t;

void setup()
{
  // Khởi tạo serial để đọc response từ module GNSS [test]
  Serial.begin(115200);
  // Khởi tạo serial2 để gửi lệnh đến module GNSS
  Serial2.begin(115200);
  while (!Serial || !Serial2) {
    delay(200);
  };
  // gửi lệnh để module chạy
  Serial2.println("@GSTP");
  delay(100);
  Serial2.println("@GNS 47");
  delay(100);
  Serial2.println("@GSR");
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
  sscanf(line_str + 1, "%10[^,],%*c,%10[^,],%*c,%10[^,],%*c,%10[^,],%*[^,],%[^,],%*[^\n]", output.time, output.lat, output.lng, output.speed, output.date);
  // Đổi đơn vị tốc độ từ knot sang km/h
  sprintf(output.speed, "%.1f", atof(output.speed) * 1.852);
  return true;
}
void loop()
{
  //    if (Serial2.available()) {
  //        Serial.print((char)Serial2.read());
  //    }
  //    delay(3000);
  if (Serial2.find("$GNRMC"))
  {
    // Đọc các ký tự tiếp theo đến ký tự new line
    String line = Serial2.readStringUntil('\n');
    Serial.println("$GNRMC" + line);
    location_info_t output = {0};
    if (getLatLng(line, output)) { // has data => send to Firebase
      Serial.printf("time: %s\nlat: %s\nlng: %s\nspeed: %s\ndate: %s\n",
                    output.time, output.lat, output.lng, output.speed, output.date);
    }
  }
}