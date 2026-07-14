#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

// Inicia a tarefa em background que checa semanalmente por uma nova release
// no GitHub e, se houver, baixa e grava o firmware sozinho (self-OTA).
void setup_ota_updater();

#endif // OTA_UPDATER_H
