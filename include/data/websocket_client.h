#pragma once
#include <websocketpp/config/asio_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <functional>
#include <vector>
#include <string>
#include <thread>

using json = nlohmann::json;

class WebSocketPriceFeed {
public:
    using PriceCallback = std::function<void(const std::string& symbol, double price)>;

    WebSocketPriceFeed();
    explicit WebSocketPriceFeed(PriceCallback callback);
    ~WebSocketPriceFeed();

    void set_callback(PriceCallback callback);
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
    PriceCallback callback_;
    std::vector<std::string> subscribed_streams_;
    bool running_ = false;
};
