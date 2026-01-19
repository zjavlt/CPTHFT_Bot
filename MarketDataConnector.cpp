#include "MarketDataConnector.hpp"
#include <iostream>
#include <iomanip> // precision
#include <chrono> //time
#include <string>

MarketDataConnector::MarketDataConnector(net::io_context& ioc, ssl::context& ctx, std::shared_ptr<RingBuffer<Trade>> queue) 
    : ws_(ioc, ctx)
    , resolver_(ioc)
    , queue_(queue)
{
}

void MarketDataConnector::run(std::string host, std::string port) {
    host_ = host;

    resolver_.async_resolve(
        host,
        port,
        beast::bind_front_handler(
            &MarketDataConnector::on_resolve,
            shared_from_this()
        )
    );
}

void MarketDataConnector::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
        std::cerr << "Resolve Error: " << ec.message() << std::endl;
        return;
    }

    net::async_connect(
        beast::get_lowest_layer(ws_),
        results, 
        beast::bind_front_handler(
            &MarketDataConnector::on_connect,
            shared_from_this()
        )
    );
}

void MarketDataConnector::on_connect(beast::error_code ec, tcp::endpoint ep) {
    if (ec) {
        std::cerr << "Connect Error: " << ec.message() << std::endl;
        return;
    }

    if (! SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
        std::cerr << "SSL SNI Error" << std::endl;
        return;
    }

    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &MarketDataConnector::on_ssl_handshake,
            shared_from_this()
        )
    );
}

void MarketDataConnector::on_ssl_handshake(beast::error_code ec) {
    if (ec) {
        std::cerr << "SSL Handshake Error: " << ec.message() << std::endl;
        return;
    }

    // ws_.set_option(websocket::stream_base::decorator(
    //     [](websocket::request_type& req) {
    //         req.set(beast::http::field::user_agent, "CPTHFT_Bot_Client_v1.0");
    //     }
    // ));

    ws_.async_handshake(
        host_,
        "/ws/btcusdt@trade",
        beast::bind_front_handler(
            &MarketDataConnector::on_handshake,
            shared_from_this()
        )
    );
}

void MarketDataConnector::on_handshake(beast::error_code ec) {
    if (ec) {
        std::cerr << "WS Handshake Error: " << ec.message() << std::endl;
        return;
    }

    std::cout << "[Connected] Listening for trades..." << std::endl;

    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &MarketDataConnector::on_read,
            shared_from_this()
        )
    );
}

void MarketDataConnector::on_read(beast::error_code ec, std::size_t byte_transferred) {
    if (ec) {
        std::cerr << "Read Error: " << ec.message() << std::endl;
    }

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    auto data = beast::buffers_to_string(buffer_.data());

    json_data_ = simdjson::padded_string(data);

    try {
        simdjson::ondemand::document doc = parser_.iterate(json_data_);

        std::string_view event_type = doc["e"].get_string();

        if (event_type == "trade") {
            Trade t;
            t.symbol = doc["s"].get_string();
            
            // couold use simdjson double casting (fast)
            std::string_view p_str = doc["p"].get_string();
            t.price = std::stod(std::string(p_str));

            std::string_view q_str = doc["q"].get_string();
            t.quantity = std::stod(std::string(q_str));

            t.event_time = doc["E"].get_int64();
            t.trade_time = doc["T"].get_int64();


            if (!queue_->enqueue(t)) {
                std::cerr << "Queue Full" << std::endl;
            }
        }
    } catch(simdjson::simdjson_error& e) {}

    buffer_.consume(buffer_.size());

    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &MarketDataConnector::on_read,
            shared_from_this()
        )
    );
}