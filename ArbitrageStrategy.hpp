#pragma once
#include "MarketDataConnector.hpp"
#include "FeeCalculator.hpp"
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

    FeeCalculator fee_calc_;

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
        if (binance_price_ > 1.0 && coinbase_price_ > 1.0) {
            double raw_spread = binance_price_ - coinbase_price_;
            bool buy_bin = (raw_spread < 0); //if bin < cb, buy bin

            double reg_prof = 0.0; double vip_prof = 0.0;

            if (raw_spread > 0) {
                reg_prof = fee_calc_.calc_net_profit(coinbase_price_, binance_price_, false, Tier::REGULAR);
                vip_prof = fee_calc_.calc_net_profit(coinbase_price_, binance_price_, false, Tier::VIP);
            } else {
                reg_prof = fee_calc_.calc_net_profit(binance_price_, coinbase_price_, true, Tier::REGULAR);
                vip_prof = fee_calc_.calc_net_profit(binance_price_, coinbase_price_, true, Tier::VIP);
            }

            int64_t bin_jitter = (t.exchange == ExchangeId::BINANCE) ? (raw_latency - bin_min_lat_) : 0;
            int64_t cb_jitter = (t.exchange == ExchangeId::COINBASE) ? (raw_latency - cb_min_lat_) : 0;

            print_dashboard(raw_spread, reg_prof, vip_prof, t.exchange, raw_latency, bin_jitter, cb_jitter);
        }
    }

private:
    void print_dashboard(double spread, double reg_p, double vip_p, ExchangeId last_updater, int64_t raw_lat, int64_t bin_jit, int64_t cb_jit) {
        std::cout << CLEAR_SCREEN;
        std::cout << "\n============ARBITRAGE BOT TESTING...============\n";



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
        std::cout << "------------------------------------------------\n";
        
        //spread
        // Line 3: Spread
        std::cout << "RAW SPREAD: $" << std::setw(8) << std::abs(spread) << "\n";
            
        // Visual Alert
        std::cout << "REGULAR NET PROFIT: (Fee: 0.70%)\n";
        if (reg_p > 0.0) {
            std::cout << "\033[1;32m+$" << std::setw(8) << reg_p << " (PROFIT)\033[0m\n";
        } else {
            std::cout << "\033[1;31m-$" << std::setw(8) << std::abs(reg_p) << " (LOSS)\033[0m\n";
        }
        std::cout << "\n";

        std::cout << "VIP     NET PROFIT: (Fee: 0.067%)\n";
        if (vip_p > 0.0) {
            std::cout << "\033[1;32m+$" << std::setw(8) << vip_p << " (PROFIT)\033[0m\n";
        } else {
            std::cout << "\033[1;31m-$" << std::setw(8) << std::abs(vip_p) << " (LOSS)\033[0m\n";
        }
        std::cout << "================================================\n";
        std::cout << std::flush;
    }  
};