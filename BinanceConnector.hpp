#pragma once
#include "MarketDataConnector.hpp"
#include <simdjson.h>

class BinanceConnector : public MarketDataConnector {
private:
    simdjson::ondemand::parser parser_;
    simdjson::padded_string json_data_;

public:
    BinanceConnector(net::io_context& ioc, ssl::context& ctx, std::shared_ptr<RingBuffer<Trade>> queue) 
        : MarketDataConnector(ioc, ctx, queue) {}

protected:
    void process_message(std::string_view data) override {
        json_data_ = simdjson::padded_string(data);
        try {
            auto doc = parser_.iterate(json_data_);
            std::string_view event_type = doc["e"].get_string();

            if (event_type == "trade") {
                Trade t;
                t.exchange = ExchangeId::BINANCE;
                std::string_view s_sv = doc["s"].get_string();

                size_t len = std::min(s_sv.length(), sizeof(t.symbol) - 1);
                std::memcpy(t.symbol, s_sv.data(), len);
                t.symbol[len] = '\0'; //null terminate
                
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
        }catch (simdjson::simdjson_error&) {}
    }
};