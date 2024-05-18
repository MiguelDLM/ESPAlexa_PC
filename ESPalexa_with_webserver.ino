#include <Espalexa.h>
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <WebServer.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClient.h>

#endif

#define R1 2
// WiFi Credentials
String ssid = "";
String password = "";

boolean connectWifi();
void handleRoot();
void handleWifi();

//callback functions
void firstDeviceChanged(uint8_t brightness);

boolean wifiConnected = false;

Espalexa espalexa;
#ifdef ARDUINO_ARCH_ESP32
WebServer server(80);
#else
ESP8266WebServer server(80);
#endif

void setup()
{
  Serial.begin(115200);
  pinMode(R1, OUTPUT);
  
  EEPROM.begin(96);

  ssid = ""; // Clear ssid string
  password = ""; // Clear password string
  for (int i = 0; i < 48; ++i)
  {
    ssid += char(EEPROM.read(i));
    password += char(EEPROM.read(48 + i));
  }

  Serial.print("SSID read from EEPROM: ");
  Serial.println(ssid);
  Serial.print("Password read from EEPROM: ");
  Serial.println(password);

  // Initialise wifi connection
  wifiConnected = connectWifi();

  if (wifiConnected)
  {
    server.on("/", HTTP_GET, [](){
      server.send(200, "text/plain", "This is an example index page your server may send.");
    });
    server.on("/test", HTTP_GET, [](){
      server.send(200, "text/plain", "This is a second subpage you may have.");
    });
    server.on("/config", HTTP_GET, handleRoot);
    server.onNotFound([](){
      if (!espalexa.handleAlexaApiCall(server.uri(),server.arg(0)))
      {
        server.send(404, "text/plain", "Not found");
      }
    });

    // Define your devices here.
    espalexa.addDevice("Computadora", firstDeviceChanged);
    espalexa.begin(&server); // Pass server object to Espalexa
    digitalWrite(R1, HIGH);
    Serial.println("WiFi connected, Alexa devices initialized.");
  }
  else
  {
    // Setup AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP8266 AP");
    Serial.println("AP mode enabled, waiting for configuration...");

    server.on("/", handleRoot);
    server.on("/wifi", handleWifi);
    server.begin();
  }
}

void loop()
{
  if (wifiConnected)
  {
    espalexa.loop();
    delay(1);
  }
  else
  {
    server.handleClient();
  }
}

// Callback function for device state change
void firstDeviceChanged(uint8_t brightness)
{
  // Control the device based on brightness value
  if (brightness)
  {
    if (brightness == 255)
    {
      digitalWrite(R1, LOW);
      delay(2000);
      digitalWrite(R1, HIGH);
      Serial.println("Device1 ON");
    }
  }
  else
  {
    digitalWrite(R1, LOW);
    delay(2000);
    digitalWrite(R1, HIGH);
    Serial.println("Device1 OFF");
  }
}

// Existing connectWifi, handleRoot, and handleWifi functions remain unchanged
boolean connectWifi()
{
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("");
  Serial.print("Intentando conectar a la red ");
  Serial.print(ssid);
  Serial.print(" con la contraseña ");
  Serial.println(password);

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Conectado a ");
    Serial.println(ssid);
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("La conexión falló.");
    Serial.print("WiFi.status() = ");
    Serial.println(WiFi.status());
  }
  return state;
}

void handleRoot() {
  server.send(200, "text/html", "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Configuración WiFi</title>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<meta charset='UTF-8'>" // Especifica la codificación de caracteres
                "<style>"
                "body{font-family: Arial; padding: 20px;}"
                "input[type=text], input[type=password]{width: 100%; padding: 12px 20px; margin: 8px 0; display: inline-block; border: 1px solid #ccc; box-sizing: border-box;}"
                "input[type=submit]{background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0; border: none; cursor: pointer; width: 100%;}"
                ".container{max-width: 400px; margin: auto;}"
                "</style>"
                "</head>"
                "<body>"
                "<div class='container'>"
                "<h2>Configuración WiFi</h2>"
                "<form action=\"/wifi\" method=\"POST\">"
                "<label for=\"ssid\">SSID:</label><br>"
                "<input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"\"><br>"
                "<label for=\"password\">Password:</label><br>"
                "<input type=\"password\" id=\"password\" name=\"password\" value=\"\">"
                "<input type=\"checkbox\" onclick=\"togglePasswordVisibility()\">Mostrar/Ocultar Contraseña<br><br>"
                "<input type=\"submit\" value=\"Conectar\">"
                "</form>"
                "</div>"
                "<script>"
                "function togglePasswordVisibility() {"
                "  var x = document.getElementById('password');"
                "  if (x.type === 'password') {"
                "    x.type = 'text';"
                "  } else {"
                "    x.type = 'password';"
                "  }"
                "}"
                "</script>"
                "</body>"
                "</html>");
}

void handleWifi() {
  if(server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");

    for (int i = 0; i < 48; ++i) {
      EEPROM.write(i, i < ssid.length() ? ssid[i] : 0);
      EEPROM.write(48 + i, i < password.length() ? password[i] : 0);
    }
    EEPROM.commit();

    // Specify UTF-8 in the Content-Type header
    server.sendHeader("Content-Type", "text/html; charset=utf-8");
    server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h2>Conexión en proceso...</h2><p>Intentando conectar al nuevo SSID. Por favor, espera y revisa el monitor serial para la nueva dirección IP.</p></body></html>");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain; charset=utf-8", "400: Invalid Request - Arg not found");
  }
}