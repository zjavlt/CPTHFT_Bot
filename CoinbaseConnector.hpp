#pragma once
#include "MarketDataConnector.hpp"
#include <simdjson.h>

class CoinbaseConnector : public MarketDataConnector {
private:
    simdjson::ondemand::parser parser_;
    simdjson::padded_string json_data_;

public:
    CoinbaseConnector(net::io_context& ioc, ssl::context& ctx, std::shared_ptr<RingBuffer<Trade>> queue)
        : MarketDataConnector(ioc, ctx, queue) {}

protected:
    void on_session_started() override {
        std::string sub_msg = R"({
            "type": "subscribe",
            "product_ids": ["BTC_USD"],
            "channel": "ticker"
        })";

        ws_.async_write(net::buffer(sub_msg),
            [](beast::error_code ec, std::size_t) {
                if (ec) std::cerr << "Coinbase Subscribe Failed: " << ec.message() << std::endl;
                else std::cout << "Coinbase Subscription Sent" << std::endl;
            });
    }

    void process_message(std::string_view data) override {
        json_data_ = simdjson::padded_string(data);

        try {
            auto doc = parser_.iterate(json_data_);
            // coinbase format: { "channel": "ticker", "events": [ { "tickers": [ ... ] } ] }
            // Note: This is a simplified parse logic for demonstration.
            std::string_view channel = doc["channel"];
            if (channel == "ticker") {
                auto events = doc["events"];
                for (auto event : events) {
                    auto tickers = event["ticers"];
                    for (auto ticker : tickers) {
                        Trade t;
                        t.symbol = "BTC-USD";

                        std::string_view p_str = ticker["price"];
                        std::string_view v_str = ticker["volume_24h"];

                        t.price = std::stod(std::string(p_str));
                        t.quantity = 0.0;

                        //timestamps in Coinbase = ISO strings "2023-01-01T...Z"
                        // parsing ISO -> int64 is complex 
                        // todo: implement ISO8601 parser
                        t.event_time = 0;
                        t.trade_time = 0;

                        queue_->enqueue(t);
                    }
                    
                }
            }
        } catch (simdjson::simdjson_error& e) {
            //coinbase sends welcome message -> that fails parser logic
        }
    }
};