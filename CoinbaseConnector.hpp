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
                        std::string_view pid_sv = ticker["product_id"];
                        size_t len = std::min(pid_sv.length(), sizeof(t.symbol) - 1);
                        std::memcpy(t.symbol, pid_sv.data(), len);
                        t.symbol[len] = '\0';

                        std::string_view p_str = ticker["price"];
                        t.price = std::stod(std::string(p_str));
                        t.quantity = 0.0;

                        auto time_val = ticker.find_field("time");
                        if (time_val.error() == simdjson::SUCCESS) {
                            // Extract string_view and pass to helper
                            std::string_view time_sv = time_val.get_string();
                            t.trade_time = parse_iso8601(time_sv);
                        } else {
                            // Fallback if "time" field is missing
                            auto now = std::chrono::system_clock::now().time_since_epoch();
                            t.trade_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
                        }

                        t.event_time = t.trade_time;

                        queue_->enqueue(t);
                    }
                    
                }
            }
        } catch (simdjson::simdjson_error& e) {
            //coinbase sends welcome message -> that fails parser logic
        }
    }
};