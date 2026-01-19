#pragma once

#include "RingBuffer.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <simdjson.h>
#include <thread>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

struct Trade {
    std::string_view symbol;    // s
    double price;               // p (could be int64_t if needed for precision)
    double quantity;            // q
    int64_t trade_time;         // t
    int64_t event_time;         // e
    bool is_buyer_maker;        // m
};

using tcp = net::ip::tcp;

class MarketDataConnector : public std::enable_shared_from_this<MarketDataConnector>{

    using WssStream = websocket::stream<beast::ssl_stream<tcp::socket>>;
    std::shared_ptr<RingBuffer<Trade>> queue_;

public:
    MarketDataConnector(net::io_context& ioc, ssl::context& ctx, std::shared_ptr<RingBuffer<Trade>> q)
        :   ws_(ioc, ctx), resolver_(ioc), queue_(q) {}
    ~MarketDataConnector() = default;

    void run(std::string host, std::string port);

private:

    tcp::resolver resolver_;
    WssStream ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    simdjson::ondemand::parser parser_;
    simdjson::padded_string json_data_;
    
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

    void on_connect(beast::error_code ec, tcp::endpoint ep);

    void on_ssl_handshake(beast::error_code ec);

    void on_handshake(beast::error_code ec);

    void on_read(beast::error_code ec, std::size_t bytes_transferred);
};