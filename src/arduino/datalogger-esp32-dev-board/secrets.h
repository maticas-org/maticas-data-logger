#ifndef SECRETS_H
#define SECRETS_H

// ------------------------ WiFi Configuration ------------------------
#define MY_SSID "ArchLinux"
#define MY_PASSWORD "nuncaheusadoarch"
#define PING_URLS [ "www.google.com", "www.facebook.com", "www.twitter.com" ]

// ------------------------ Data Storage Configuration ------------------------
#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5
#define MAX_EVENTS_PER_FILE 3


// ------------------------ API Configuration ------------------------
extern const uint API_PORT = 8000;
extern const String API_URL = "192.168.1.105";
extern const String API_CREDS = "{\"username\":\"admin\", \"password\":\"admin\"}";
extern const String API_TOKEN = "Token 872408e3e07b09c35cd89b10eba29aae1e35bcfd";

#define HTTP_TIMEOUT 5000
#define API_ENDPOINT "/api/measurement/careverga"
#define API_LOGIN_ENDPOINT "/api/user/login/"

extern const String TZ = "-05:00";
extern const String DLI_UIID = "\"b32669d0-92e0-4f36-a254-6f202f24301d\"";
extern const String VPD_UIID = "\"7fc78b41-67e6-4fae-988e-cafa962287e4\"";
extern const String LUX_UIID = "\"19e3bb86-8814-4a7c-86d3-522af32df2e0\"";
extern const String HUMIDITY_UIID = "\"5d706a1a-86b0-4bca-b2c8-6a9e52107f27\"";
extern const String DEWPOINT_UIID = "\"7e95ee3a-7d75-4d9e-a442-64e0c99a9eda\"";
extern const String TEMPERATURE_UIID = "\"3a56695b-005c-48d3-8005-a76f5d0dc9f6\"";

extern const String CROP_UIID = "\"592ec839-6b48-499d-b3b6-dde99fd4630e\"";

#endif // SECRETS_H