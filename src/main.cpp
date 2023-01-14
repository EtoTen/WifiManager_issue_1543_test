#include <Arduino.h>
#include "main.h"

#include <ESPmDNS.h>
#include <Time.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

static const char *TAG = "main";

// Instantiate the WifiManager object
WiFiManager wifiManager;
boolean firstConnection = false;
time_t lastRunWifiCheck;

// from WifiManager
/** IP to String? */
String toStringIp(IPAddress ip)
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

bool hasWifiBeenConfigured()
{
  uint8_t empty_bssid[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  wifi_config_t config = {};

  if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK)
    return false;
  ESP_LOGI(TAG, "SSID: %s, BSSID: " MACSTR, config.sta.ssid, MAC2STR(config.sta.bssid));
  return (strlen((const char *)config.sta.ssid) >= 1) || (memcmp(empty_bssid, config.sta.bssid, sizeof(empty_bssid)) != 0);
}

// Get WiFi SSID
String WIFI_GetSSID()
{
  String mSSID = "";
  if (!hasWifiBeenConfigured())
  {
    Serial.println("Wifi Has not been configured. Returning blank SSID.");
  }
  else
  {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    mSSID = String(reinterpret_cast<const char *>(conf.sta.ssid));
  }
  return mSSID;
}

// Get WiFi Password
String WIFI_GetPassword()
{
  String mPassword = "";
  if (!hasWifiBeenConfigured())
  {
    Serial.println("Wifi Has not been configured. Returning blank password.");
  }
  else
  {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    mPassword = String(reinterpret_cast<const char *>(conf.sta.password));
  }
  return mPassword;
}

// Attempt to connect to WiFi
boolean WIFI_Connect()
{
  WiFi.disconnect();
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_GetSSID().c_str());
  Serial.print("Password: ");
  Serial.println(WIFI_GetPassword().c_str());
  // bool res = wifiManager.autoConnect((const char *)WIFI_GetSSID().c_str(), (const char *)WIFI_GetPassword().c_str()); // password protected ap
  //  if (WiFi.begin(WIFI_GetSSID().c_str(), WIFI_GetPassword().c_str()) != WL_CONNECTED)
  WiFi.begin(WIFI_GetSSID().c_str(), WIFI_GetPassword().c_str());
  for (int x = 0; x < MAX_WIFI_RETRIES; x++)
  {
    if (WiFi.waitForConnectResult(10000) != WL_CONNECTED)
    {
      Serial.println("Failed to connect to WiFi");
    }
    else
    {
      Serial.println("Connected to WiFi");
#if WIFI_WEB_PORTAL_ENABLED
      // start web portal
      if (!MDNS.begin(HOSTNAME))
      {
        Serial.println("Error setting up MDNS responder!");
      }
      wifiManager.setConfigPortalBlocking(false);
      wifiManager.startWebPortal();
#endif
      Serial.print("RRSI: ");
      Serial.println(WiFi.RSSI());
      Serial.print("Local IP: ");
      Serial.println(toStringIp(WiFi.localIP()));
      return true;
    }
  }
  return false;
}

void setup()
{

  // Configure serial interface
  Serial.begin(115200);
  delay(800); // alows serial to start, or else initial messages are lost

  Serial.println("Starting...");

  lastRunWifiCheck = now();

#if WIFI_ENABLED
  wifiManager.setDebugOutput(true);

  //  Reset saved settings for testing purposes
  //  Should be commented out for normal operation
  wifiManager.resetSettings();

  wifiManager.setHostname(HOSTNAME);
  wifiManager.setCountry("US");

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Set WiFi RF power output to highest level

  // If WiFi setup is not configured, start access point
  // Otherwise, connect to WiFi

  if (!hasWifiBeenConfigured())
  {
    Serial.println("Starting Wifi AP");

    wifiManager.setConfigPortalBlocking(false);
    wifiManager.startConfigPortal(AP_NAME);
    firstConnection = true;
    wifiManager.setDebugOutput(true);
  }
  else
  {
    Serial.println("Starting Wifi Client");
    WIFI_Connect();
  }
#endif

  Serial.println("Ready!");
}

// ***************************************************************
// Program Loop
// ***************************************************************

void loop()
{
#if WIFI_ENABLED
  // Check WiFi connectivity and restart it if it fails.
  if ((((now() - lastRunWifiCheck) / 60) >= WIFI_CHECK_INTERVAL) && (hasWifiBeenConfigured()))
  {
    lastRunWifiCheck = now();
    Serial.println("Checking WiFi connection...");
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi connection was lost. Reconnecting...");
      WIFI_Connect();
    }
    else
    {
      Serial.println("Wifi is already connected.");
    }
  }
  if (wifiManager.getWebPortalActive() || wifiManager.getConfigPortalActive())
    wifiManager.process();
#endif
}
