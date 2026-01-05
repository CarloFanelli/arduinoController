#include <WiFiS3.h>
#include <Arduino_LED_Matrix.h>
#include "secrets.h"  // contiene WIFI_SSID e WIFI_PASS

// Oggetto matrice LED integrata
ArduinoLEDMatrix matrix;

// Credenziali WiFi (non hardcodate qui)
const char* ssid = WIFI_SSID;
const char* pass = WIFI_PASS;

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// Frame: matrice spenta
uint8_t frame_off[8][12] = {
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0}
};

// Frame: matrice accesa
uint8_t frame_on[8][12] = {
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1}
};

void applyLedState(bool on) {
  // LED integrato
  digitalWrite(LED_BUILTIN, on ? HIGH : LOW);

  // Matrice LED sincronizzata
  if (on) {
    matrix.renderBitmap(frame_on, 8, 12);
  } else {
    matrix.renderBitmap(frame_off, 8, 12);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(9600);
  while (!Serial);

  // Avvio matrice LED e stato iniziale OFF
  matrix.begin();
  matrix.renderBitmap(frame_off, 8, 12);

  Serial.println("Connessione al WiFi...");

  // Connessione WiFi
  while (status != WL_CONNECTED) {
    Serial.print("Tentativo di connessione a ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(2000);
  }

  Serial.println("WiFi connesso!");

  // --- ATTESA DHCP ---
  IPAddress ip;
  int tentativi = 0;
  do {
    delay(1000);
    ip = WiFi.localIP();
    Serial.print("Tentativo DHCP ");
    Serial.print(tentativi + 1);
    Serial.print(" - IP: ");
    Serial.println(ip);
    tentativi++;
  } while (ip == IPAddress(0, 0, 0, 0) && tentativi < 15);

  Serial.print("IP Address finale: ");
  Serial.println(ip);

  if (ip == IPAddress(0, 0, 0, 0)) {
    Serial.println("ATTENZIONE: il router non ha assegnato un IP (DHCP).");
  }

  // Avvio server
  server.begin();
  Serial.println("Server HTTP avviato sulla porta 80");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("Nuovo client");

  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  bool toOn = false;
  bool toOff = false;

  if (req.indexOf("GET /on") >= 0) {
    applyLedState(true);
    toOn = true;
  } else if (req.indexOf("GET /off") >= 0) {
    applyLedState(false);
    toOff = true;
  }

  // Se la richiesta è /on o /off, reindirizza a /
  if (toOn || toOff) {
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /");
    client.println("Connection: close");
    client.println();
  } else {
    // Pagina principale su /
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><meta charset='utf-8'><title>UNO R4</title><meta name='viewport' content='width=device-width,initial-scale=1'>");
    client.println("<style>");
    client.println("body{margin:0;font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:#0f172a;color:#e5e7eb;display:flex;align-items:center;justify-content:center;height:100vh;}");
    client.println(".card{background:#020617;border:1px solid #1f2937;border-radius:1rem;padding:2rem;box-shadow:0 20px 40px rgba(0,0,0,0.4);text-align:center;max-width:320px;width:100%;}");
    client.println("h1{font-size:1.25rem;margin-bottom:1rem;}");
    client.println(".state{margin-bottom:1.5rem;font-size:0.95rem;color:#9ca3af;}");
    client.println(".pill{display:inline-block;padding:0.15rem 0.7rem;border-radius:999px;font-weight:600;margin-left:0.25rem;}");
    client.println(".pill-on{background:#22c55e33;color:#22c55e;}");
    client.println(".pill-off{background:#ef444433;color:#f97316;}");
    client.println(".btn-row{display:flex;gap:0.75rem;justify-content:center;}");
    client.println(".btn{flex:1;padding:0.6rem 0.75rem;border-radius:0.75rem;border:none;font-size:0.95rem;font-weight:600;cursor:pointer;transition:background 0.15s,transform 0.05s;}");
    client.println(".btn-on{background:#22c55e;color:#022c22;}");
    client.println(".btn-off{background:#0f172a;color:#e5e7eb;border:1px solid #4b5563;}");
    client.println(".btn:active{transform:scale(0.97);}");
    client.println(".footer{margin-top:1rem;font-size:0.75rem;color:#6b7280;}");
    client.println("</style></head><body>");
    client.println("<div class='card'>");
    client.println("<h1>Controllo luce UNO R4</h1>");
    client.print("<div class='state'>Stato LED:<span class='pill ");
    client.print(digitalRead(LED_BUILTIN) ? "pill-on'>ON" : "pill-off'>OFF");
    client.println("</span></div>");
    client.println("<div class='btn-row'>");
    client.println("<form action='/on' method='get'><button class='btn btn-on' type='submit'>Accendi</button></form>");
    client.println("<form action='/off' method='get'><button class='btn btn-off' type='submit'>Spegni</button></form>");
    client.println("</div>");
    client.println("<div class='footer'>Hosted da Arduino UNO R4 WiFi</div>");
    client.println("</div></body></html>");
  }

  delay(1);
  client.stop();
  Serial.println("Client disconnesso");
}