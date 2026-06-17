#include "data/websocket_feed.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

BinanceWebSocketFeed::BinanceWebSocketFeed(const std::string& interval)
    : interval_(interval) {
    ws_client_.init_asio();
    ws_client_.clear_access_channels(websocketpp::log::alevel::all);
    ws_client_.clear_error_channels(websocketpp::log::elevel::all);

    ws_client_.set_open_handler([this](auto hdl) {
        std::cout << "[WS] Connected\n";
        connection_ = hdl;
        for (auto& s : subscribed_streams_) {
            nlohmann::json msg;
            msg["method"] = "SUBSCRIBE";
            msg["params"] = nlohmann::json::array({s});
            msg["id"] = 1;
            ws_client_.send(hdl, msg.dump(), websocketpp::frame::opcode::text);
        }
    });

    ws_client_.set_message_handler([this](auto hdl, auto msg) {
        std::string payload = msg->get_payload();
        if (payload.empty() || payload[0] != '{') return;
        auto j = nlohmann::json::parse(payload);
        if (j.contains("result") && j["result"].is_null()) return;
        if (j.contains("e") && j["e"] == "kline") {
            auto& k = j["k"];
            if (!k["x"].get<bool>()) return;
            std::string sym = k["s"];
            double price = std::stod(k["c"].get<std::string>());
            uint64_t ts = k["T"];
            std::lock_guard<std::mutex> lk(mutex_);
            last_prices_[sym] = price;
            last_timestamps_[sym] = ts;
            if (last_prices_.size() == 2 && callback_ &&
                last_timestamps_[sym1_orig_] == ts &&
                last_timestamps_[sym2_orig_] == ts) {
                callback_(last_prices_[sym1_orig_], last_prices_[sym2_orig_], ts);
                last_prices_.clear();
                last_timestamps_.clear();
            }
        }
    });

    ws_client_.set_close_handler([](auto) { std::cout << "[WS] Closed\n"; });
    ws_client_.set_tls_init_handler([](auto) {
        return websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
            websocketpp::lib::asio::ssl::context::sslv23);
    });
}

BinanceWebSocketFeed::~BinanceWebSocketFeed() { stop(); }

void BinanceWebSocketFeed::set_callback(PairPriceCallback cb) { callback_ = cb; }

void BinanceWebSocketFeed::subscribe(const std::string& sym1, const std::string& sym2) {
    sym1_orig_ = sym1;
    sym2_orig_ = sym2;
    std::string s1 = sym1 + "@kline_" + interval_;
    std::string s2 = sym2 + "@kline_" + interval_;
    std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
    subscribed_streams_ = {s1, s2};
}

void BinanceWebSocketFeed::start() {
    websocketpp::lib::error_code ec;
    auto con = ws_client_.get_connection("wss://stream.binance.com:9443/ws", ec);
    if (ec) { std::cerr << "[WS] Connect error: " << ec.message() << "\n"; return; }
    ws_client_.connect(con);
    running_ = true;
    ws_thread_ = std::thread([this]() {
        ws_client_.run();
        running_ = false;
    });
}

void BinanceWebSocketFeed::stop() {
    if (running_) {
        ws_client_.stop();
        if (ws_thread_.joinable()) ws_thread_.join();
    }
}
