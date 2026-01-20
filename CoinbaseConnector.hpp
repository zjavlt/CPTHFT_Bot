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

    int64_t parse_iso8601(std::string_view s) {
        if (s.length() < 19) return 0;
        struct tm tm = {};
        
        tm.tm_year = (s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0') - 1900;
        tm.tm_mon  = (s[5]-'0')*10 + (s[6]-'0') - 1;
        tm.tm_mday = (s[8]-'0')*10 + (s[9]-'0');
        tm.tm_hour = (s[11]-'0')*10 + (s[12]-'0');
        tm.tm_min  = (s[14]-'0')*10 + (s[15]-'0');
        tm.tm_sec  = (s[17]-'0')*10 + (s[18]-'0');

        int64_t seconds = timegm(&tm);
        int64_t millis = 0;

        if (s.length() > 20 && s[19] == '.') {
            if (s[20] >= '0' && s[20] <= '9') millis += (s[20] - '0') * 100;
            if (s.length() > 21 && s[21] >= '0' && s[21] <= '9') millis += (s[21] - '0') * 10;
            if (s.length() > 22 && s[22] >= '0' && s[22] <= '9') millis += (s[22] - '0');
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
            "product_ids": ["BTC-USD"],
            "channels": ["ticker"]
        })";

        ws_.async_write(net::buffer(sub_msg),
            [](beast::error_code ec, std::size_t) {}); 
    }

    void process_message(std::string_view data) override {
        // std::cout << "[RAW CB] " << data << std::endl;
        json_data_ = simdjson::padded_string(data);

        try {
            auto doc = parser_.iterate(json_data_);
            simdjson::ondemand::object obj = doc.get_object();
            // std::cout << "Parser check" << std::endl;

            auto type_res = obj.find_field("type");

            if (type_res.error() == simdjson::SUCCESS) {
                std::string_view type = type_res.get_string();
                
                if (type == "ticker") {
                    // std::cout << "Ticker check" << std::endl;
                    Trade t;
                    t.exchange = ExchangeId::COINBASE;

                    //symbol
                    std::string_view pid_sv = obj["product_id"];
                    size_t len = std::min(pid_sv.length(), sizeof(t.symbol) - 1);
                    std::memcpy(t.symbol, pid_sv.data(), len);
                    t.symbol[len] = '\0';
                    // std::cout << "symbol check" << std::endl;

                    //price
                    std::string_view p_str = obj["price"];
                    t.price = std::stod(std::string(p_str));
                    // std::cout << "price check" << std::endl;
                    //time
                    auto time_res = obj.find_field("time");
                    if (time_res.error() == simdjson::SUCCESS) {
                        std::string_view time_sv = time_res.get_string();
                        t.trade_time = parse_iso8601(time_sv);
                        t.event_time = t.trade_time;
                        // std::cout << "time check" << std::endl;
                    } 
                    
                    //quant
                    auto size_res = obj.find_field("last_size");
                    if (size_res.error() == simdjson::SUCCESS) {
                        // std::cout << "lastsize success check" << std::endl;
                        std::string_view s_str = size_res.get_string();
                        t.quantity = std::stod(std::string(s_str));
                    } else {
                        // std::cout << "last size fail check" << std::endl;
                        t.quantity = 0.0;
                    }
                    queue_->enqueue(t);
                }
            }
        } catch (simdjson::simdjson_error& e){
            std::cerr << "[JSON ERROR] " << e.what() << "| Payload: " << data << std::endl;
        } catch (std::exception& e) {
            std::cerr << "[STD ERROR] " << e.what() << std::endl;
            //coinbase sends welcome message -> that fails parser logic
        }
    }
};