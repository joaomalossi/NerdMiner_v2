#ifndef DASHBOARD_CORE_H
#define DASHBOARD_CORE_H

// Lógica pura do dashboard (sem dependência de Arduino) — testável em pio test -e native.

// Portas de stratum shared/PPLNS da Public Pool (V1 e V1+TLS)
#define PPLNS_PORT 13333
#define PPLNS_TLS_PORT 14333

const char* dashboard_hostname(int stratumPort);

#include <string>

typedef struct {
    int port;
    std::string pool;
    double hashRateKHs;
    unsigned long shares;
    std::string bestDiff;
    unsigned long templates;
    unsigned long uptimeSeconds;
    std::string version;
    int rssi;
    unsigned long freeHeap;
    double tempC;
    std::string btcAddress;
} DashboardStats;

std::string dashboard_stats_json(const DashboardStats& s);

#endif // DASHBOARD_CORE_H
