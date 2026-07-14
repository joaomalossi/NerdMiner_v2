#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

#include "version.h"
#include "otaUpdater.h"

// Board padrao caso o ambiente de build nao defina OTA_ASSET_NAME
#ifndef OTA_ASSET_NAME
#define OTA_ASSET_NAME "ESP32-devKitv1_firmware.bin"
#endif

static const uint32_t kOtaCheckIntervalMs = 7UL * 24UL * 60UL * 60UL * 1000UL; // 7 dias
static const uint32_t kOtaFirstCheckDelayMs = 30UL * 1000UL; // deixa o miner subir antes

// Aponta para o FORK do usuario (nao o upstream BitMaker-hub/NerdMiner_v2):
// o fork tem uma Action que recompila automaticamente cada release upstream
// com este patch de OTA aplicado, senao o self-update baixaria o binario
// oficial (sem esse recurso) e perderia a capacidade de auto-update.
//
// Pagina web (nao a API) redireciona direto para a tag da release mais
// recente, ex: .../releases/latest -> .../releases/tag/nerdminer-release-V1.8.3
// Evita baixar o JSON completo da API (~65KB, com todos os assets de cada board).
static const char kReleasesLatestPage[] = "https://github.com/joaomalossi/NerdMiner_v2/releases/latest";
static const char kDownloadBaseUrl[] = "https://github.com/joaomalossi/NerdMiner_v2/releases/download/";

static bool fetchLatestTag(String &tagOut) {
  WiFiClientSecure client;
  client.setInsecure(); // sem CA pinning: tradeoff aceito para este device hobby

  HTTPClient http;
  http.setTimeout(15000);
  http.setConnectTimeout(15000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(client, kReleasesLatestPage)) {
    Serial.println("[OTA] Falha ao conectar no GitHub");
    return false;
  }

  // Por padrao o HTTPClient descarta headers nao coletados explicitamente
  static const char *kHeadersToCollect[] = {"Location"};
  http.collectHeaders(kHeadersToCollect, 1);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_FOUND && httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
    Serial.printf("[OTA] GET releases/latest -> HTTP %d (esperava redirect)\n", httpCode);
    http.end();
    return false;
  }

  String location = http.header("Location");
  http.end();

  int lastSlash = location.lastIndexOf('/');
  if (lastSlash < 0 || lastSlash == (int)location.length() - 1) {
    Serial.printf("[OTA] Nao foi possivel extrair a tag do redirect: '%s'\n", location.c_str());
    return false;
  }

  tagOut = location.substring(lastSlash + 1);
  return true;
}

// Baixa o .bin da release informada e grava por cima da propria particao em execucao.
// Sem particao OTA dupla nesta board: uma queda de energia/wifi no meio da gravacao
// pode deixar o dispositivo sem bootar, exigindo reflash via USB.
static bool downloadAndFlash(const String &url) {
  Serial.printf("[OTA] Heap livre antes do download: %u bytes\n", ESP.getFreeHeap());

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(30000);
  http.setConnectTimeout(30000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    Serial.println("[OTA] Falha ao abrir conexao com o asset");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] GET asset -> HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  if (len <= 0) {
    Serial.println("[OTA] Tamanho do asset desconhecido, abortando");
    http.end();
    return false;
  }

  if (!Update.begin((size_t)len)) {
    Serial.printf("[OTA] Sem espaco para gravar %d bytes: %s\n", len, Update.errorString());
    http.end();
    return false;
  }

  Serial.printf("[OTA] Gravando %d bytes...\n", len);
  WiFiClient &stream = http.getStream();
  size_t written = Update.writeStream(stream);
  http.end();

  if (written != (size_t)len) {
    Serial.printf("[OTA] Gravado %u de %d bytes, abortando\n", (unsigned)written, len);
    Update.abort();
    return false;
  }

  if (!Update.end(true)) {
    Serial.printf("[OTA] Falha ao finalizar update: %s\n", Update.errorString());
    return false;
  }

  Serial.println("[OTA] Firmware gravado com sucesso, reiniciando...");
  return true;
}

static void otaCheckTask(void *pvParameters) {
  // Espera o Wi-Fi conectar antes da primeira checagem
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  vTaskDelay(kOtaFirstCheckDelayMs / portTICK_PERIOD_MS);

  const String expectedTag = String("nerdminer-release-") + CURRENT_VERSION;

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      String latestTag;
      if (fetchLatestTag(latestTag)) {
        Serial.printf("[OTA] Versao instalada: %s | Release mais recente: %s\n",
                      expectedTag.c_str(), latestTag.c_str());

        if (latestTag != expectedTag) {
          String url = String(kDownloadBaseUrl) + latestTag + "/" + OTA_ASSET_NAME;
          Serial.printf("[OTA] Nova versao disponivel! Baixando: %s\n", url.c_str());

          if (downloadAndFlash(url)) {
            delay(500);
            ESP.restart();
          } else {
            Serial.println("[OTA] Falha na atualizacao, tenta de novo no proximo ciclo");
          }
        } else {
          Serial.println("[OTA] Ja esta na versao mais recente");
        }
      }
    } else {
      Serial.println("[OTA] Wi-Fi desconectado, pulando checagem semanal");
    }

    vTaskDelay(kOtaCheckIntervalMs / portTICK_PERIOD_MS);
  }
}

void setup_ota_updater() {
  xTaskCreatePinnedToCore(otaCheckTask, "OtaUpdater", 10000, NULL, 1, NULL, 1);
}
