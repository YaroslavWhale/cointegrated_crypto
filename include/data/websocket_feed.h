#pragma once
#include "data/i_live_data_feed.h"
#include <websocketpp/config/asio_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <map>

class BinanceWebSocketFeed : public ILiveDataFeed {
public:
    BinanceWebSocketFeed();
    ~BinanceWebSocketFeed() override;

    void set_callback(PairPriceCallback cb) override;
    void subscribe(const std::string& symbol1, const std::string& symbol2) override;
    void start() override;
    void stop() override;

private:
    using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
    using Hdl = websocketpp::connection_hdl;

    void on_open(Hdl);
    void on_message(Hdl, Client::message_ptr msg);
    void on_close(Hdl);
    void on_fail(Hdl);
    websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(Hdl);

    Client ws_client_;
    Hdl connection_;
    PairPriceCallback callback_;
    std::vector<std::string> subscribed_streams_;
    bool running_ = false;
    std::thread ws_thread_;

    std::mutex mutex_;
    std::map<std::string, double> last_prices_;
    std::map<std::string, uint64_t> last_timestamps_;
    std::string sym1_orig_, sym2_orig_;
};
