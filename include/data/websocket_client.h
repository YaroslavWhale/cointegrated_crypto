#pragma once
#include <websocketpp/config/asio_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <functional>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <cstdint>

using json = nlohmann::json;

class WebSocketPriceFeed {
public:
    using PairPriceCallback = std::function<void(const std::string& sym1, double price1,
                                                 const std::string& sym2, double price2,
                                                 uint64_t close_time)>;

    WebSocketPriceFeed();
    explicit WebSocketPriceFeed(PairPriceCallback callback);
    ~WebSocketPriceFeed();

    void set_callback(PairPriceCallback callback);
    void subscribe(const std::string& stream_name);
    void run();
    void stop();

private:
    using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
    using ConnectionHdl = websocketpp::connection_hdl;

    void on_message(ConnectionHdl, Client::message_ptr msg);
    void on_open(ConnectionHdl);
    void on_close(ConnectionHdl);
    void on_fail(ConnectionHdl);
    websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>
        on_tls_init(ConnectionHdl);

    Client client_;
    ConnectionHdl hdl_;
    PairPriceCallback callback_;
    std::vector<std::string> subscribed_streams_;
    bool running_ = false;

    std::mutex mutex_;
    std::map<std::string, double> latest_price_;
    std::map<std::string, std::string> latest_original_symbol_;
};
