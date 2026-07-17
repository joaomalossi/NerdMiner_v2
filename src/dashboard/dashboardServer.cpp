#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "dashboard_core.h"
#include "dashboardServer.h"
#include "../version.h"
#include "../utils.h"
#include "../drivers/storage/storage.h"

extern TSettings Settings;
extern uint32_t templates;
extern uint32_t totalKHashes;
extern uint32_t shares;
extern double best_diff;

static WebServer s_server(80);

// Hashrate calculado de forma independente do loop do display (que usa
// elapsedKHs/mElapsed do ciclo de tela): delta de totalKHashes numa janela
// própria, sem tocar no estado do monitor.
static double s_hashRateKHs = 0.0;

static void sampleHashrate() {
    static uint32_t lastKHashes = 0;
    static int64_t lastUs = 0;
    int64_t nowUs = esp_timer_get_time();
    if (lastUs == 0) {
        lastUs = nowUs;
        lastKHashes = totalKHashes;
        return;
    }
    double dtSec = (double)(nowUs - lastUs) / 1e6;
    if (dtSec >= 10.0) {
        uint32_t kh = totalKHashes;
        s_hashRateKHs = (double)(kh - lastKHashes) / dtSec;
        lastKHashes = kh;
        lastUs = nowUs;
    }
}

static void handleStats() {
    DashboardStats s{};
    s.port = Settings.PoolPort;
    s.pool = Settings.PoolAddress.c_str();
    s.hashRateKHs = s_hashRateKHs;
    s.shares = shares;
    char diffBuf[16] = {0};
    suffix_string(best_diff, diffBuf, sizeof(diffBuf), 0);
    s.bestDiff = diffBuf;
    s.templates = templates;
    s.uptimeSeconds = (unsigned long)(esp_timer_get_time() / 1000000LL);
    s.version = CURRENT_VERSION;
    s.rssi = WiFi.RSSI();
    s.freeHeap = ESP.getFreeHeap();
    s.tempC = temperatureRead();

    std::string json = dashboard_stats_json(s);
    s_server.sendHeader("Access-Control-Allow-Origin", "*");
    s_server.send(200, "application/json", json.c_str());
}

static void dashboardTask(void *pvParameters) {
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    const char *host = dashboard_hostname(Settings.PoolPort);
    if (MDNS.begin(host)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[DASH] mDNS registrado: http://%s.local\n", host);
    } else {
        Serial.println("[DASH] Falha ao registrar mDNS (segue acessivel por IP)");
    }

    s_server.on("/api/stats", HTTP_GET, handleStats);
    s_server.onNotFound([]() {
        s_server.sendHeader("Access-Control-Allow-Origin", "*");
        s_server.send(404, "text/plain", "not found");
    });
    s_server.begin();
    Serial.printf("[DASH] Servidor no ar: http://%s/api/stats\n",
                  WiFi.localIP().toString().c_str());

    for (;;) {
        s_server.handleClient();
        sampleHashrate();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup_dashboard_server() {
    xTaskCreatePinnedToCore(dashboardTask, "Dashboard", 8192, NULL, 1, NULL, 1);
}
