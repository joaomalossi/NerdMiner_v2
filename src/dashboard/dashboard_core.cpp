#include "dashboard_core.h"

#include <cstdio>

static bool is_pplns_port(int port) {
    return port == PPLNS_PORT || port == PPLNS_TLS_PORT;
}

std::string dashboard_stats_json(const DashboardStats& s) {
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"mode\":\"%s\",\"pool\":\"%s\",\"port\":%d,"
             "\"hashRateKHs\":%.1f,\"shares\":%lu,"
             "\"bestDiff\":\"%s\",\"templates\":%lu,\"uptimeSeconds\":%lu,"
             "\"version\":\"%s\",\"rssi\":%d,\"freeHeap\":%lu,\"tempC\":%.1f}",
             is_pplns_port(s.port) ? "shared" : "solo",
             s.pool.c_str(), s.port, s.hashRateKHs, s.shares,
             s.bestDiff.c_str(), s.templates, s.uptimeSeconds,
             s.version.c_str(), s.rssi, s.freeHeap, s.tempC);
    return std::string(buf);
}

const char* dashboard_hostname(int stratumPort) {
    if (stratumPort == PPLNS_PORT || stratumPort == PPLNS_TLS_PORT)
        return "nerdminer-shared";
    return "nerdminer-solo";
}
