#include "data/rest_client.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

std::vector<double> RestClient::fetch_klines(const std::string& symbol,
                                             const std::string& interval,
                                             int limit) {
    std::vector<double> closes;
    try {
        std::string host = "api.binance.com";
        std::string target = "/api/v3/klines?symbol=" + symbol +
                             "&interval=" + interval +
                             "&limit=" + std::to_string(limit);

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, "443");
        beast::ssl_stream<beast::tcp_stream> stream(ioc);
        // Используем стандартный SSL контекст
        stream.next_layer().connect(results);
        stream.handshake(beast::ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        auto body = beast::buffers_to_string(res.body().data());
        auto j = nlohmann::json::parse(body);

        for (const auto& candle : j) {
            // candle[4] – close price (строка)
            double close = std::stod(candle[4].get<std::string>());
            closes.push_back(close);
        }

    } catch (std::exception const& e) {
        std::cerr << "REST fetch error: " << e.what() << std::endl;
    }
    return closes;
}
