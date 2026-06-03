#include "data/websocket_client.h"
#include <iostream>
#include <algorithm>
#include <cctype>

WebSocketPriceFeed::WebSocketPriceFeed(PriceCallback callback)
    : callback_(std::move(callback)) {
    client_.init_asio();
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.clear_error_channels(websocketpp::log::elevel::all);

    client_.set_message_handler(
        [this](auto hdl, auto msg) { on_message(hdl, msg); });
    client_.set_open_handler(
        [this](auto hdl) { on_open(hdl); });
    client_.set_close_handler(
        [this](auto hdl) { on_close(hdl); });
    client_.set_fail_handler(
        [this](auto hdl) { on_fail(hdl); });
    client_.set_tls_init_handler(
        [this](auto hdl) { return on_tls_init(hdl); });
}

WebSocketPriceFeed::~WebSocketPriceFeed() {
    stop();
}

void WebSocketPriceFeed::subscribe(const std::string& stream_name) {
    subscribed_streams_.push_back(stream_name);
}

void WebSocketPriceFeed::run() {
    if (subscribed_streams_.empty()) {
        std::cerr << "No streams to subscribe\n";
        return;
    }

    std::string uri = "wss://stream.binance.com:9443/stream?streams=";
    for (size_t i = 0; i < subscribed_streams_.size(); ++i) {
        if (i > 0) uri += "/";
        std::string stream = subscribed_streams_[i];
        std::transform(stream.begin(), stream.end(), stream.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        uri += stream;
    }

    std::cout << "Connecting to " << uri << std::endl;

    websocketpp::lib::error_code ec;
    auto con = client_.get_connection(uri, ec);
    if (ec) {
        std::cerr << "Connection error: " << ec.message() << "\n";
        return;
    }
    client_.connect(con);
    running_ = true;
    client_.run();
}

void WebSocketPriceFeed::stop() {
    if (running_) {
        client_.stop();
        running_ = false;
    }
}

websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>
WebSocketPriceFeed::on_tls_init(ConnectionHdl) {
    namespace asio = websocketpp::lib::asio;
    auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(
        asio::ssl::context::sslv23);
    ctx->set_default_verify_paths();
    ctx->set_options(asio::ssl::context::default_workarounds |
                     asio::ssl::context::no_sslv2 |
                     asio::ssl::context::single_dh_use);
    return ctx;
}

void WebSocketPriceFeed::on_open(ConnectionHdl hdl) {
    std::cout << "WebSocket connected and subscribed" << std::endl;
    hdl_ = hdl;
}

void WebSocketPriceFeed::on_message(ConnectionHdl, Client::message_ptr msg) {
    try {
        auto payload = json::parse(msg->get_payload());

        if (payload.contains("data")) {
            auto& data = payload["data"];

            if (data.contains("e") && data["e"] == "kline") {
                auto& kline = data["k"];
                bool is_closed = kline["x"].get<bool>();
                if (is_closed) {
                    std::string symbol = kline["s"];
                    double price = std::stod(kline["c"].get<std::string>());
                    if (callback_) {
                        callback_(symbol, price);
                    }
                }
            }
            else if (data.contains("e") && data["e"] == "trade") {
                std::string symbol = data["s"];
                double price = std::stod(data["p"].get<std::string>());
                if (callback_) {
                    callback_(symbol, price);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}

void WebSocketPriceFeed::on_close(ConnectionHdl) {
    std::cout << "WebSocket closed\n";
}

void WebSocketPriceFeed::on_fail(ConnectionHdl) {
    std::cerr << "WebSocket connection failed\n";
}
