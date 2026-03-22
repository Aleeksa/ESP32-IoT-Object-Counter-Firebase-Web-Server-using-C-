#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <time.h>

// ================================================================
//  KONFIGURACIJA
// ================================================================
#define WIFI_SSID   ""
#define WIFI_PASS   ""

#define FB  ""

#define TRIG_PIN    5
#define ECHO_PIN    18
#define THRESHOLD   200.0f    // cm - prag detekcije
#define HYSTERESIS  250.0f    // cm - koliko daleko mora da ode pre ponovne detekcije
#define COOLDOWN    140000UL  // ms - min. vreme izmedju detekcija

// ================================================================
//  GLOBALNE PROMENLJIVE
// ================================================================
int           g_count         = 0;
float         g_dist          = -1;
bool          g_inside        = false;
unsigned long g_lastDetect    = 0;
unsigned long g_lastSensor    = 0;
unsigned long g_lastDistWrite = 0;
unsigned long g_lastFlagCheck = 0;
String        g_lastDetTime   = "";
String        g_lastReset     = "";
bool          g_timeSynced    = false;

WebServer server(80);

// ================================================================
//  VREME
// ================================================================
String now() {
    if (!g_timeSynced) return "";
    struct tm t;
    if (!getLocalTime(&t)) return "";
    char b[32];
    strftime(b, sizeof(b), "%d.%m.%Y %H:%M:%S", &t);
    return String(b);
}

// ================================================================
//  ULTRAZVUK
// ================================================================
float readDist() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long d = pulseIn(ECHO_PIN, HIGH, 30000);
    return d == 0 ? -1 : (d * 0.0343f) / 2.0f;
}

// ================================================================
//  FIREBASE
// ================================================================
void fbPut(String path, String val) {
    HTTPClient h;
    h.begin(String(FB) + path);
    h.addHeader("Content-Type", "application/json");
    h.PUT(val);
    h.end();
}

void fbPost(String path, String val) {
    HTTPClient h;
    h.begin(String(FB) + path);
    h.addHeader("Content-Type", "application/json");
    h.POST(val);
    h.end();
}

void fbDelete(String path) {
    HTTPClient h;
    h.begin(String(FB) + path);
    h.sendRequest("DELETE");
    h.end();
}

String fbGet(String path) {
    HTTPClient h;
    h.begin(String(FB) + path);
    String r = "";
    if (h.GET() == 200) r = h.getString();
    h.end();
    return r;
}

// ================================================================
//  RESET FLAG
// ================================================================
bool checkResetFlag() {
    String rf = fbGet("/reset_flag.json");
    if (rf == "true") {
        g_count = 0;
        g_lastReset = now();
        g_lastDetect = 0;
        fbPut("/object_count.json", "0");
        fbPut("/last_reset.json", "\"" + g_lastReset + "\"");
        fbDelete("/reset_flag.json");
        Serial.println(">>> RESET: brojac = 0");
        return true;
    }
    return false;
}

// ================================================================
//  SYNC
// ================================================================
void fbSync() {
    fbPut("/distance.json",     String(g_dist, 1));
    fbPut("/object_count.json", String(g_count));
    fbPut("/sensor_near.json",  g_inside ? "true" : "false");
    if (g_lastDetTime != "")
        fbPut("/last_detection.json", "\"" + g_lastDetTime + "\"");
}

// ================================================================
//  WEB SERVER
// ================================================================
void handleRoot() {
    String h = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
               "<meta http-equiv='refresh' content='2'>"
               "<title>ESP32</title></head><body>"
               "<h2>ESP32 Senzor</h2>"
               "<p>Rastojanje: <b>" + String(g_dist, 1) + " cm</b></p>"
               "<p>Broj: <b>" + String(g_count) + "</b></p>"
               "<p>Status: <b>" + String(g_inside ? "DETEKTOVAN" : "Slobodno") + "</b></p>"
               "<p>Vreme: " + now() + "</p>"
               "</body></html>";
    server.send(200, "text/html; charset=utf-8", h);
}

void handleResetCount() {
    g_count = 0;
    g_lastReset = now();
    g_lastDetect = 0;
    fbPut("/object_count.json", "0");
    fbPut("/last_reset.json", "\"" + g_lastReset + "\"");
    fbDelete("/reset_flag.json");
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
}

void handleResetLog() {
    fbDelete("/detection_log.json");
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);

    IPAddress ip(192,168,100,160), gw(192,168,100,1), nm(255,255,255,0), dns(8,8,8,8);
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gw, nm, dns);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println(" OK: " + WiFi.localIP().toString());

    configTime(3600, 3600, "pool.ntp.org");  // UTC+1 + DST (Srbija)
    struct tm t;
    for (int i = 0; i < 20; i++) {
        if (getLocalTime(&t)) { g_timeSynced = true; break; }
        delay(500);
    }
    Serial.println(g_timeSynced ? "NTP OK: " + now() : "NTP FAIL");

    bool wasReset = checkResetFlag();
    if (!wasReset) {
        String r = fbGet("/object_count.json");
        if (r != "" && r != "null") g_count = r.toInt();
    }

    String lr = fbGet("/last_reset.json");
    lr.replace("\"", "");
    if (lr != "null" && lr != "") g_lastReset = lr;

    String ld = fbGet("/last_detection.json");
    ld.replace("\"", "");
    if (ld != "null" && ld != "") g_lastDetTime = ld;

    Serial.println("Pocetni broj: " + String(g_count));

    server.on("/", handleRoot);
    server.on("/reset-count", handleResetCount);
    server.on("/reset-log",   handleResetLog);
    server.begin();

    fbSync();
    Serial.println("Sve OK. Startuje...");
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
    server.handleClient();

    unsigned long ms = millis();

    // Proveri reset_flag svakih 3s
    if (ms - g_lastFlagCheck >= 3000) {
        g_lastFlagCheck = ms;
        checkResetFlag();
    }

    // Meri senzor svakih 300ms
    if (ms - g_lastSensor < 300) return;
    g_lastSensor = ms;

    float d = readDist();
    g_dist = d;
    Serial.println("Dist: " + String(d, 1) + " cm | Count: " + String(g_count));

    // Ako senzor vrati -1 (nema odjeka) — ignorisi merenje potpuno
    // Ne menjaj g_inside, ne broji, ne resetuj
    if (d < 0) return;

    // Detekcija — ulazak u zonu
    if (d < THRESHOLD && !g_inside) {
        g_inside = true;
        if (ms - g_lastDetect > COOLDOWN || g_lastDetect == 0) {
            g_count++;
            g_lastDetect  = ms;
            g_lastDetTime = now();
            Serial.println(">>> DETEKTOVAN #" + String(g_count) + " u " + g_lastDetTime);
            fbPut("/object_count.json", String(g_count));
            fbPost("/detection_log.json",
                "{\"count\":" + String(g_count) + ",\"time\":\"" + g_lastDetTime + "\"}");
            fbPut("/last_detection.json", "\"" + g_lastDetTime + "\"");
        }
    }

    // Resetuj tek kad se objekat dovoljno udalji (samo na validnom merenju)
    if (g_inside && d >= HYSTERESIS) {
        g_inside = false;
        Serial.println("    Otisao (dist=" + String(d, 1) + ")");
    }

    // Pisi rastojanje + sensor_near na Firebase svakih 3s
    if (ms - g_lastDistWrite >= 3000) {
        g_lastDistWrite = ms;
        fbPut("/distance.json",    String(g_dist, 1));
        fbPut("/sensor_near.json", g_inside ? "true" : "false");
    }
}

