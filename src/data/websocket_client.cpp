#include "data/websocket_client.h"
#include <iostream>
#include <algorithm>
#include <cctype>

WebSocketPriceFeed::WebSocketPriceFeed() {
    std::cout << "[WS] Initializing..." << std::endl;

    client_.init_asio();
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.clear_error_channels(websocketpp::log::elevel::all);

    client_.set_open_handler([this](auto hdl) {
        std::cout << "[WS] Connected!" << std::endl;
        hdl_ = hdl;
        // Отправляем подписку на каждый поток
        for (const auto& stream : subscribed_streams_) {
            std::string sub_msg = R"({"method":"SUBSCRIBE","params":[")" + stream + R"("],"id":1})";
            client_.send(hdl, sub_msg, websocketpp::frame::opcode::text);
        }
    });

    client_.set_message_handler([this](auto hdl, auto msg) {
        try {
            std::string payload = msg->get_payload();
            if (payload.empty() || payload[0] != '{') return;
            auto j = json::parse(payload);

            // Подтверждение подписки
            if (j.contains("result") && j["result"].is_null()) {
                std::cout << "[WS] Subscription confirmed" << std::endl;
                return;
            }

            // Данные свечи
            if (j.contains("e") && j["e"] == "kline") {
                auto& k = j["k"];
                bool is_closed = k["x"].get<bool>();
                std::string symbol = k["s"];
                double price = std::stod(k["c"].get<std::string>());

                if (is_closed && callback_) {
                    callback_(symbol, price);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[WS] Error: " << e.what() << std::endl;
        }
    });

    client_.set_close_handler([this](auto) {
        std::cout << "[WS] Connection closed" << std::endl;
    });

    client_.set_fail_handler([this](auto) {
        std::cerr << "[WS] Connection failed!" << std::endl;
    });

    client_.set_tls_init_handler([this](auto) {
        auto ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
            websocketpp::lib::asio::ssl::context::sslv23);
        ctx->set_default_verify_paths();
        return ctx;
    });
}

WebSocketPriceFeed::WebSocketPriceFeed(PriceCallback callback)
    : WebSocketPriceFeed() {
    callback_ = std::move(callback);
}

WebSocketPriceFeed::~WebSocketPriceFeed() {
    stop();
}

void WebSocketPriceFeed::set_callback(PriceCallback callback) {
    callback_ = std::move(callback);
}

void WebSocketPriceFeed::subscribe(const std::string& stream_name) {
    std::string lower = stream_name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    subscribed_streams_.push_back(lower);
}

void WebSocketPriceFeed::run() {
    if (subscribed_streams_.empty()) {
        std::cerr << "[WS] No subscriptions\n";
        return;
    }

    std::string uri = "wss://stream.binance.com:9443/ws";
    std::cout << "[WS] Connecting to " << uri << std::endl;

    websocketpp::lib::error_code ec;
    auto con = client_.get_connection(uri, ec);
    if (ec) {
        std::cerr << "[WS] Connection error: " << ec.message() << "\n";
        return;
    }

    client_.connect(con);
    running_ = true;
    client_.run();  // блокирующий вызов
    std::cout << "[WS] Event loop ended" << std::endl;
}

void WebSocketPriceFeed::stop() {
    if (running_) {
        std::cout << "[WS] Stopping..." << std::endl;
        running_ = false;
        client_.stop();
    }
}
