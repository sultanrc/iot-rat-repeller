#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "esp_timer.h"
#include "esp_http_server.h"
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>

WiFiManager wm;

String API_key        = "9807920";
String no_whatsapp    = "6281272702620";

uint8_t SIGNAL_PIN    = 14; 
uint8_t SERVO_PIN     = 12;
uint8_t FLASH_LED     = 4;

uint8_t pir_init          = LOW;
uint8_t on_flash          = 13;
uint8_t off_flash         = 06; 
uint32_t flash_start_time  = 0;
bool flash_active         = false;

#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiUDP ntp_udp;
NTPClient ntp_client(ntp_udp, "pool.ntp.org", 7 * 3600); 
Servo servo;
uint8_t servoPos = 90;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(

<html>
  <head>
    <title>Monitoring CAM</title>
    <link rel="icon" type="image/x-icon" href="https://i.ibb.co.com/7kqNXJS/e10900f8-5afd-47a7-adeb-ed96ac870142.webp">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { 
        font-family: Verdana, Geneva, Tahoma, sans-serif; 
        text-align: center; 
        margin: 0px auto; 
        padding-top: 30px; 
      }
      .container {
        display: flex;
        flex-direction: column;
        margin-top: 40px;
      }
      .button-container{
        display: flex;
        flex-direction: row;
        justify-content: center;
      }

      .button {
        border-width: 1px;
        border-color: grey;
        color: rgb(0, 0, 0);
        padding: 10px 40px;
        font-weight: bold;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 18px;
        cursor: pointer;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0,0,0,0);
      }
      .button.reset {
        padding: 10px 71px;
      }
      .left:hover, .right:hover {
        background-color: #1f912b;
        color: white;
      }
      .flash-container {
        display: flex;
        flex-direction: row;
        justify-content: center;
      }
      .button.flash-on:hover, .button.flash-off:hover {
        background-color: #ffa500;
        color: white;
      }
      .button.send-data:hover {
        background-color:#0000ff;
        color:white;
      }
      .button.reset:hover {
        background-color: #f3593a;
        color: white;
      }
      .about{
        display: flex;
        justify-content: center;
        /* position: fixed;
        bottom: 5px; */
        font-size: 12px;
      }
      img {
        width: auto;
        max-width: 100%;
        height: auto;
      }
    </style>
  </head>
  <body>
    <h1>Monitoring CAM Alat pengusir Tikus</h1>
    <img src="" id="photo">
    <div class="container">
        <div class="button-container">
            <button class="button left" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');"> L </button>
            <button class="button right" onmousedown="toggleCheckbox('right');" ontouchstart="toggleCheckbox('right');"> R </button>
        </div>
        <div class="flash-container">
            <button class="button flash-on" onmousedown="toggleCheckbox('flash_on');" ontouchstart="toggleCheckbox('flash_on');">Flash ON</button>
            <button class="button flash-off" onmousedown="toggleCheckbox('flash_off');" ontouchstart="toggleCheckbox('flash_off');">Flash OFF</button>
        </div>
        <div class="database-container">
            <button class="button send-data" onclick="window.location.href='http://localhost/insert.php'">submit</button>
        </div>
        <div class="reset-container">
            <button class="button reset" onmousedown="toggleCheckbox('reset');" ontouchstart="toggleCheckbox('reset');">reset</button>
        </div>
    </div>
    <div class="about">
        <h4>APT V 1.0 by sultan & rizki</h4>
    </div>
    <script>
      function toggleCheckbox(x) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/action?go=" + x, true);
        xhr.send();
      }
      window.onload = function() {
        document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";
      };
    </script>
  </body>
</html>

)rawliteral";

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  while (1)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->width > 400)
      {
        if (fb->format != PIXFORMAT_JPEG)
        {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted)
          {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else
        {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK)
    {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK)
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    if (res == ESP_OK)
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    buf = (char*)malloc(buf_len);
    if (!buf)
    {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) {}
      else
      {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    }
    else
    {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  }
  else
  {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  sensor_t * s = esp_camera_sensor_get();

  int res = 0;

  if (!strcmp(variable, "left")) {
    if (servoPos <= 170) {
      servoPos += 10;
      servo.write(servoPos);
    }
    Serial.println(servoPos);
    Serial.println("Left");
  } else if (!strcmp(variable, "right")) {
    if (servoPos >= 10) {
      servoPos -= 10;
      servo.write(servoPos);
    }
    Serial.println(servoPos);
    Serial.println("Right");
  } else if (!strcmp(variable, "flash_on")) {
    Serial.println("Flash ON");
    flash_active = true;
    flash_start_time = millis();
    digitalWrite(FLASH_LED, HIGH);
  } else if (!strcmp(variable, "flash_off")) {
    Serial.println("Flash OFF");
    flash_active = false;
    digitalWrite(FLASH_LED, LOW);
  } else if (!strcmp(variable, "reset")) {
    Serial.println("Streaming server stopped");
    flash_start_time = 0;
    ESP.restart();
  } else {
    res = -1;
  }

  if (res) return httpd_resp_send_500(req);

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

void startStream() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t cmd_uri = {
    .uri       = "/action",
    .method    = HTTP_GET,
    .handler   = cmd_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void send_message(String message){

  String url = "https://api.callmebot.com/whatsapp.php?phone=" + no_whatsapp + "&apikey=" + API_key + "&text=" + urlEncode(message);
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.println("Message sent successfully");
  }
  http.end();
}

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  pinMode(SIGNAL_PIN, INPUT);
  pinMode (FLASH_LED, OUTPUT);
  digitalWrite (FLASH_LED, LOW);
  servo.attach(SERVO_PIN);
  servo.write(servoPos);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else{
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK){
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  bool res = wm.autoConnect("APT");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWIFI CONNECTED");
  for (uint8_t a = 0; a < 2; a++) {
    digitalWrite(FLASH_LED, HIGH);
    delay(200);
    digitalWrite(FLASH_LED, LOW);
    delay(200);
  }
}

void loop(){
  ntp_client.update();
  uint8_t current_hour = ntp_client.getHours();
  uint8_t pir_current = digitalRead(SIGNAL_PIN);
  uint32_t current_time = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting reconnection...");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to WiFi!");
    } else {
      Serial.println("Failed to reconnect to WiFi.");
    }
  }

  if (pir_current == HIGH && pir_init == LOW){
    flash_active = true;
    flash_start_time = current_time; 
    String alert_message = "MOTION DETECTED! \nhttp://" + WiFi.localIP().toString();
    Serial.println("GERAKAN TERDETEKSI");
    Serial.println(alert_message);
    send_message(alert_message); 
    startStream();
  }
  pir_init = pir_current;

  if (flash_active) {
    if ((current_time - flash_start_time) < 30000) {
        digitalWrite(FLASH_LED, HIGH);
    } else {
        flash_active = false; 
        digitalWrite(FLASH_LED, LOW);
    }
  } else {
    digitalWrite(FLASH_LED, LOW);
    }
}