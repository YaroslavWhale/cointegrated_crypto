#pragma once
#include "data/i_live_data_feed.hpp"
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <vector>

class BinanceWebSocketFeed : public ILiveDataFeed {
public:
    explicit BinanceWebSocketFeed(const std::string& interval);
    ~BinanceWebSocketFeed() override;

    void set_callback(PairPriceCallback cb) override;
    void subscribe(const std::string& sym1, const std::string& sym2) override;
    void start() override;
    void stop() override;

private:
    using WSClient = websocketpp::client<websocketpp::config::asio_tls_client>;
    using Connection = websocketpp::connection_hdl;

    WSClient ws_client_;
    Connection connection_;
    std::thread ws_thread_;
    std::atomic<bool> running_{false};
    std::mutex mutex_;

    PairPriceCallback callback_;
    std::vector<std::string> subscribed_streams_;
    std::string sym1_orig_, sym2_orig_;
    std::string interval_;

    std::map<std::string, double> last_prices_;
    std::map<std::string, uint64_t> last_timestamps_;
};
