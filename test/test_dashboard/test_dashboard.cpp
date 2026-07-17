#include <unity.h>
#include "dashboard_core.h"

void setUp(void) {}
void tearDown(void) {}

// Portas PPLNS (13333 e a variante TLS 14333) identificam a placa "shared";
// qualquer outra porta (3333, 21496, 4333...) é "solo".
void test_hostname_shared_for_pplns_ports(void) {
    TEST_ASSERT_EQUAL_STRING("nerdminer-shared", dashboard_hostname(13333));
    TEST_ASSERT_EQUAL_STRING("nerdminer-shared", dashboard_hostname(14333));
}

void test_hostname_solo_for_other_ports(void) {
    TEST_ASSERT_EQUAL_STRING("nerdminer-solo", dashboard_hostname(21496));
    TEST_ASSERT_EQUAL_STRING("nerdminer-solo", dashboard_hostname(3333));
    TEST_ASSERT_EQUAL_STRING("nerdminer-solo", dashboard_hostname(4333));
}

// O JSON do /api/stats carrega os valores ao vivo e o modo derivado da porta —
// é o contrato que a página consome.
void test_stats_json_carries_live_values_and_mode(void) {
    DashboardStats s{};
    s.port = 21496;
    s.pool = "public-pool.io";
    s.hashRateKHs = 354.2;
    s.shares = 3;
    std::string json = dashboard_stats_json(s);
    TEST_ASSERT_TRUE(json.find("\"mode\":\"solo\"") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"pool\":\"public-pool.io\"") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"port\":21496") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"hashRateKHs\":354.2") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"shares\":3") != std::string::npos);
}

void test_stats_json_mode_shared_on_pplns_port(void) {
    DashboardStats s{};
    s.port = 13333;
    std::string json = dashboard_stats_json(s);
    TEST_ASSERT_TRUE(json.find("\"mode\":\"shared\"") != std::string::npos);
}

void test_stats_json_carries_diagnostic_fields(void) {
    DashboardStats s{};
    s.port = 21496;
    s.bestDiff = "6.73";
    s.templates = 623;
    s.uptimeSeconds = 3725;
    s.version = "V1.8.3-dash1";
    s.rssi = -61;
    s.freeHeap = 245000;
    s.tempC = 47.8;
    std::string json = dashboard_stats_json(s);
    TEST_ASSERT_TRUE(json.find("\"bestDiff\":\"6.73\"") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"templates\":623") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"uptimeSeconds\":3725") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"version\":\"V1.8.3-dash1\"") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"rssi\":-61") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"freeHeap\":245000") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"tempC\":47.8") != std::string::npos);
}

// A página precisa do endereço de payout pra consultar a conta na Public Pool.
void test_stats_json_carries_btc_address(void) {
    DashboardStats s{};
    s.port = 21496;
    s.btcAddress = "bc1qdp3m0m2hwztvlz2gcjq2zqmqegp33r694832cz";
    std::string json = dashboard_stats_json(s);
    TEST_ASSERT_TRUE(json.find("\"btcAddress\":\"bc1qdp3m0m2hwztvlz2gcjq2zqmqegp33r694832cz\"")
                     != std::string::npos);
}

// Histórico de hashrate: ring buffer de 24h (1440 amostras de 1min) exposto
// em /api/history como array JSON do mais antigo pro mais novo.
void test_history_empty_serializes_as_empty_array(void) {
    HashrateHistory h;
    history_init(h);
    TEST_ASSERT_EQUAL_STRING("[]", history_json(h).c_str());
}

void test_history_lists_samples_oldest_first(void) {
    HashrateHistory h;
    history_init(h);
    history_add(h, 350.4);
    history_add(h, 355.0);
    history_add(h, 340.6);
    TEST_ASSERT_EQUAL_STRING("[350,355,341]", history_json(h).c_str());
}

void test_history_wraps_dropping_oldest(void) {
    HashrateHistory h;
    history_init(h);
    for (int i = 0; i < HISTORY_CAPACITY + 2; i++) history_add(h, 100.0 + i);
    std::string json = history_json(h);
    TEST_ASSERT_TRUE(json.rfind("[102,", 0) == 0);   // 2 mais antigas caíram
    TEST_ASSERT_TRUE(json.find(",1541]") != std::string::npos); // mais nova no fim
    int commas = 0;
    for (char c : json) if (c == ',') commas++;
    TEST_ASSERT_EQUAL_INT(HISTORY_CAPACITY - 1, commas); // tamanho fixo em 1440
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_hostname_shared_for_pplns_ports);
    RUN_TEST(test_hostname_solo_for_other_ports);
    RUN_TEST(test_stats_json_carries_live_values_and_mode);
    RUN_TEST(test_stats_json_mode_shared_on_pplns_port);
    RUN_TEST(test_stats_json_carries_diagnostic_fields);
    RUN_TEST(test_stats_json_carries_btc_address);
    RUN_TEST(test_history_empty_serializes_as_empty_array);
    RUN_TEST(test_history_lists_samples_oldest_first);
    RUN_TEST(test_history_wraps_dropping_oldest);
    return UNITY_END();
}
