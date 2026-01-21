#pragma once
#include "MarketDataConnector.hpp"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#define CLEAR_SCREEN "\033[2J\033[1;1H"

class AbrtirageStrategy {
private:
    double binance_price_ = 0.0;
    double coinbase_price_ = 0.0;

    int64_t bin_min_lat_ = 99999999;
    int64_t cb_min_lat_ = 99999999;

    int64_t last_ts_bin_ = 0;
    int64_t last_ts_cb_ = 0;

    bool first_print_ = true;

public:
    void on_trade(const Trade& t) {
        int64_t local_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        int64_t raw_latency = local_now - t.trade_time;

        //filter late packets
        if (t.exchange == ExchangeId::BINANCE) {
            if (t.trade_time < last_ts_bin_)return;//ignore stale data
            last_ts_bin_ = t.trade_time;
            binance_price_ = t.price;
            if (raw_latency < bin_min_lat_) bin_min_lat_ = raw_latency;
        } 
        else {
            if (t.trade_time < last_ts_cb_) return;
            last_ts_cb_ = t.trade_time;
            coinbase_price_ = t.price;
            if (raw_latency < cb_min_lat_) cb_min_lat_ = raw_latency;
        }

        //dashboard
        if (binance_price_ < 1.0 || coinbase_price_ < 1.0) return;

        double spread = binance_price_ - coinbase_price_;

        int64_t bin_jitter = (t.exchange == ExchangeId::BINANCE) ? (raw_latency - bin_min_lat_) : 0;
        int64_t cb_jitter = (t.exchange == ExchangeId::COINBASE) ? (raw_latency - cb_min_lat_) : 0;

        print_dashboard(spread, t.exchange, raw_latency, bin_jitter, cb_jitter);
    }

private:
    void print_dashboard(double spread, ExchangeId last_updater, int64_t raw_lat, int64_t bin_jit, int64_t cb_jit) {
        std::cout << CLEAR_SCREEN;

        std::cout << "====================ARBITRAGE BOT TESTING...====================\n";


        //line 1: binance
        std::cout << "[BINANCE ] " 
                  << std::fixed << std::setprecision(2) << std::setw(10) << binance_price_
                  << " (USDT) | Jitter: " << std::setw(3) << bin_jit << "ms"
                  << (last_updater == ExchangeId::BINANCE ? " <--" : "") 
                  << "\n";

        //line 2: Coinbase
        std::cout << "[COINBASE] " 
                  << std::setw(10) << coinbase_price_ 
                  << " (USD)  | Jitter: " << std::setw(3) << cb_jit << "ms"
                  << (last_updater == ExchangeId::COINBASE ? "  <--" : "")
                  << "\n";
        std::cout << "-----------------------------------------------------\n";
        //spread
        // Line 3: Spread
        std::cout << "[SPREAD  ] " 
                  << "$" << std::setw(8) << spread 
                  << " (" << std::setprecision(3) << (spread / coinbase_price_ * 100.0) << "%)"
                  << "\n";
            
        // Visual Alert
        if (std::abs(spread) > 100.0) {
             std::cout << "\n\033[1;32m>>> ARBITRAGE OPPORTUNITY DETECTED! <<<\033[0m\n";
        }

        std::cout << std::flush;
    }  
};