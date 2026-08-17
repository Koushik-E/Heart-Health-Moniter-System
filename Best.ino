#include <Wire.h>
#include <MAX30105.h>
#include "heartRate.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// OLED Config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MAX30105 Sensor
MAX30105 particleSensor;

// Wi-Fi Config
const char *ssid = "Galaxy A14 5G EBF8";      // Replace with your Wi-Fi SSID
const char *password = "Mahi777#69";  // Replace with your Wi-Fi Password
WebServer server(80); // ESP32 WebServer

// Variables for Heart Rate
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// Variables for SpO2 and Temperature
float spO2 = 0.0; // SpO2 placeholder
float temperature = 0.0; // Temperature placeholder

bool fingerDetected = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.display();

  // Initialize MAX30105
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (true);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  long irValue = particleSensor.getIR();
  fingerDetected = irValue > 50000;

  if (!fingerDetected) {
    // No finger detected
    beatsPerMinute = 0;
    beatAvg = 0;
    spO2 = 0;
    temperature = 0;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("No finger detected!");
    display.display();
  } else {
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute > 20 && beatsPerMinute < 255) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

    // SpO2 Calculation (Placeholder)
    spO2 = calculateSpO2();

    // Temperature Calculation (Placeholder)
    temperature = calculateTemperature();
  }

  // Display on OLED
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  if (fingerDetected) {
    display.print("BPM: ");
    display.println(beatsPerMinute);
    display.print("Avg BPM: ");
    display.println(beatAvg);
    display.print("SpO2: ");
    display.println(spO2);
    display.print("Temp: ");
    display.println(temperature);
  } else {
    display.println("No finger detected!");
  }
  display.display();

  // Handle HTTP requests
  server.handleClient();
}

// Placeholder functions for SpO2 and temperature
float calculateSpO2() {
  return random(94, 100); // Replace with actual calculation
}

float calculateTemperature() {
  return random(35, 38); // Replace with actual calculation
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Health Monitor Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: black;
      color: white;
      text-align: center;
      margin: 0;
      padding: 0;
    }
    .card {
      background: rgba(255, 255, 255, 0.1);
      padding: 20px;
      margin: 20px auto;
      border-radius: 12px;
      max-width: 400px;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.8);
    }
    h1 {
      font-size: 2.5rem;
      margin-bottom: 0.5rem;
    }
    p {
      margin: 5px 0;
    }
    .heart {
      display: inline-block;
      font-size: 50px;
      color: red;
      animation: beat 1s infinite;
      transform-origin: center;
    }
    @keyframes beat {
      0%, 100% {
        transform: scale(1);
      }
      50% {
        transform: scale(1.2);
      }
    }
    .alert {
      color: red;
      font-weight: bold;
      font-size: 1.2rem;
    }
    .triangle {
      color: red;
      font-size: 2rem;
    }
  </style>
  <script>
    function fetchData() {
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById('bpm').innerText = data.bpm;
        document.getElementById('avgBpm').innerText = data.avgBpm;
        document.getElementById('spO2').innerText = data.spO2;
        document.getElementById('temp').innerText = data.temp;
        document.getElementById('fingerStatus').innerText = data.fingerDetected ? "" : "No finger detected!";
        if (data.fingerDetected && (data.avgBpm < 30 || data.avgBpm > 150)) {
          document.getElementById('alert').innerHTML = '<div class="triangle">&#9888;</div> Abnormal BPM!';
        } else {
          document.getElementById('alert').innerText = '';
        }
        document.getElementById('heart').style.visibility = data.fingerDetected ? "visible" : "hidden";
      });
    }
    setInterval(fetchData, 1000);
  </script>
</head>
<body>
  <h1>Health Monitor Dashboard</h1>
  <div class="card">
    <div id="heart" class="heart">&#10084;</div>
    <p id="alert" class="alert"></p>
    <p id="fingerStatus"></p>
    <p>BPM: <span id="bpm">Loading...</span></p>
    <p>Avg BPM: <span id="avgBpm">Loading...</span></p>
    <p>SpO2: <span id="spO2">Loading...</span>%</p>
    <p>Temperature: <span id="temp">Loading...</span>°C</p>
  </div>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"bpm\":" + String(beatsPerMinute) + ",";
  json += "\"avgBpm\":" + String(beatAvg) + ",";
  json += "\"spO2\":" + String(spO2) + ",";
  json += "\"temp\":" + String(temperature) + ",";
  json += "\"fingerDetected\":" + String(fingerDetected ? "true" : "false") + ",";
  json += "\"alert\":" + String((beatAvg < 30 || beatAvg > 150) ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}
