#pragma once
#include "MarketDataConnector.hpp"
#include <simdjson.h>
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <ctime>
#include <algorithm>
#include <cstring>




class CoinbaseConnector : public MarketDataConnector {
private:
    simdjson::ondemand::parser parser_;
    simdjson::padded_string json_data_;

    // Helper: Fast ISO8601 (UTC) to Unix Milliseconds
    int64_t parse_iso8601(std::string_view s) {

        if (s.length() < 19) {
            return 0;
        }

        struct tm tm = {};
        
        tm.tm_year = (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0') - 1900;

        tm.tm_mon = (s[5] - '0') * 10 + (s[6] - '0') - 1;

        tm.tm_mday = (s[8] - '0') * 10 + (s[9] - '0');
        
        tm.tm_hour = (s[11] - '0') * 10 + (s[12] - '0');

        tm.tm_min = (s[14] - '0') * 10 + (s[15] - '0');

        tm.tm_sec = (s[17] - '0') * 10 + (s[18] - '0');

        int64_t seconds = timegm(&tm);

        int millis = 0; 

        // Check if the string has a period '.' at index 19.
        if (s.length() > 20 && s[19] == '.') {
            if (s[20] >= '0' && s[20] <= '9') {
                millis += (s[20] - '0') * 100;
            }

            if (s.length() > 21 && s[21] >= '0' && s[21] <= '9') {
                millis += (s[21]-'0')*10;
            }

            if (s.length() > 22 && s[22] >= '0' && s[22] <= '9') {
                millis += (s[22] - '0');
            }
        }

        return (seconds * 1000) + millis;
    }

public:
    CoinbaseConnector(net::io_context& ioc, ssl::context& ctx, std::shared_ptr<RingBuffer<Trade>> queue)
        : MarketDataConnector(ioc, ctx, queue) {}

protected:
    void on_session_started() override {
        std::string sub_msg = R"({
            "type": "subscribe",
            "product_ids": ["BTC_USD"],
            "channel": "market_trades"
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
            std::string_view channel = doc["channel"];
            std::cout << "at least coinbase connection is working?" << std::endl;
            if (channel == "market_trades") {
                auto events = doc["events"];
                for (auto event : events) {
                    auto trades = event["trades"];
                    for (auto item : trades) {
                        std::cout << "all works" << std::endl;
                        Trade t;
                        t.exchange = ExchangeId::COINBASE;
                        std::string_view pid_sv = item["product_id"];
                        size_t len = std::min(pid_sv.length(), sizeof(t.symbol) - 1);
                        std::memcpy(t.symbol, pid_sv.data(), len);
                        t.symbol[len] = '\0';

                        std::string_view p_str = item["price"];
                        t.price = std::stod(std::string(p_str));
                        
                        std::string_view q_str = item["size"];
                        t.quantity = std::stod(std::string(q_str));

                        auto time_val = item.find_field("time");
                        if (time_val.error() == simdjson::SUCCESS) {
                            // Extract string_view and pass to helper
                            std::string_view time_sv = time_val.get_string();
                            t.trade_time = parse_iso8601(time_sv);
                            t.event_time = t.trade_time;
                            queue_->enqueue(t);
                        } 
                    }
                }
            }
        } catch (simdjson::simdjson_error& e) {
            //coinbase sends welcome message -> that fails parser logic
        }
    }
};