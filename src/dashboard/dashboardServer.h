#ifndef DASHBOARD_SERVER_H
#define DASHBOARD_SERVER_H

// Servidor web embutido do dashboard (task FreeRTOS de prioridade baixa).
// Expõe GET /api/stats com os dados ao vivo da mineração, com CORS liberado,
// e registra o hostname mDNS derivado da porta de stratum (dashboard_core).
void setup_dashboard_server();

#endif // DASHBOARD_SERVER_H
