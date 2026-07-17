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
             "\"version\":\"%s\",\"rssi\":%d,\"freeHeap\":%lu,\"tempC\":%.1f,"
             "\"btcAddress\":\"%s\"}",
             is_pplns_port(s.port) ? "shared" : "solo",
             s.pool.c_str(), s.port, s.hashRateKHs, s.shares,
             s.bestDiff.c_str(), s.templates, s.uptimeSeconds,
             s.version.c_str(), s.rssi, s.freeHeap, s.tempC,
             s.btcAddress.c_str());
    return std::string(buf);
}

void history_init(HashrateHistory& h) {
    h.count = 0;
    h.head = 0;
}

void history_add(HashrateHistory& h, double kHs) {
    h.samples[h.head] = (unsigned short)(kHs + 0.5);
    h.head = (h.head + 1) % HISTORY_CAPACITY;
    if (h.count < HISTORY_CAPACITY) h.count++;
}

std::string history_json(const HashrateHistory& h) {
    std::string out = "[";
    int start = (h.head - h.count + HISTORY_CAPACITY) % HISTORY_CAPACITY;
    for (int i = 0; i < h.count; i++) {
        if (i) out += ",";
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", h.samples[(start + i) % HISTORY_CAPACITY]);
        out += buf;
    }
    out += "]";
    return out;
}

const char* dashboard_hostname(int stratumPort) {
    if (stratumPort == PPLNS_PORT || stratumPort == PPLNS_TLS_PORT)
        return "nerdminer-shared";
    return "nerdminer-solo";
}
