#pragma once
#include "MarketDataConnector.hpp"
#include "FeeCalculator.hpp"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#define CLEAR_SCREEN "\033[2J\033[1;1H"

class ArbtirageStrategy {
private:
    double binance_price_ = 0.0;
    double coinbase_price_ = 0.0;

    int64_t bin_min_lat_ = 99999999;
    int64_t cb_min_lat_ = 99999999;

    int64_t last_ts_bin_ = 0;
    int64_t last_ts_cb_ = 0;

    int skipped_frame_count_ = 0;

    FeeCalculator fee_calc_;

    std::chrono::steady_clock::time_point last_print_time_;
    const int PRINT_INTERVAL_MS = 100; // update UI at 10 FPS (human eye limit? thats kinda cap)

    long long internal_lat_ns_ = 0; //nanoseconds
    long long max_internal_lat_ns = 0; 

public:
    ArbtirageStrategy() {
        last_print_time_ = std::chrono::steady_clock::now();
    }

    void on_trade(const Trade& t) {
        auto start_proc = std::chrono::high_resolution_clock::now();

        int64_t local_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        int64_t raw_latency = local_now - t.trade_time;

        int64_t current_floor = (t.exchange == ExchangeId::BINANCE ? bin_min_lat_ : cb_min_lat_);

        if (raw_latency > (t.exchange == ExchangeId::BINANCE ? bin_min_lat_ : cb_min_lat_) + 2000) {
            if (t.exchange == ExchangeId::BINANCE) bin_min_lat_ = raw_latency;
            else cb_min_lat_ = raw_latency;
        }

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

            auto end_proc = std::chrono::high_resolution_clock::now();
            internal_lat_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(end_proc - start_proc).count();
            if (internal_lat_ns_ > max_internal_lat_ns) max_internal_lat_ns = internal_lat_ns_;

            int64_t bin_jit = (t.exchange == ExchangeId::BINANCE) ? (raw_latency - bin_min_lat_) : 0;
            int64_t cb_jit = (t.exchange == ExchangeId::COINBASE) ? (raw_latency - cb_min_lat_) : 0;
            int64_t current_jitter = std::max(bin_jit, cb_jit);

            if (current_jitter > 200) {
                skipped_frame_count_++;

                if (skipped_frame_count_ > 50) {
                    if (t.exchange == ExchangeId::BINANCE) bin_min_lat_ = raw_latency;
                    else cb_min_lat_ = raw_latency;
                    skipped_frame_count_ = 0;
                } else {
                    return;
                }
            } else {
                skipped_frame_count_ = 0; // We are healthy
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_print_time_).count();

            if (elapsed > PRINT_INTERVAL_MS) {
                print_dashboard(raw_spread, reg_prof, vip_prof, t.exchange, raw_latency, bin_jit, cb_jit);
                last_print_time_ = now;
            }

            
        }
    }

private:
    void print_dashboard(double spread, double reg_p, double vip_p, ExchangeId last_updater, int64_t raw_lat, int64_t bin_jit, int64_t cb_jit) {
        std::cout << CLEAR_SCREEN;
        std::cout << "\n============BITCOIN ARBITRAGE SIM============\n";



        //line 1: binance
        std::cout << "[BINANCE ] " 
                  << std::fixed << std::setprecision(2) << std::setw(10) << binance_price_
                  << " (USDT) | Jitter: " << std::setw(3) << bin_jit << "ms"
                  << (last_updater == ExchangeId::BINANCE ? "  <--" : "") 
                  << "\n";
        //line 2: Coinbase
        std::cout << "[COINBASE] " 
                  << std::setw(10) << coinbase_price_ 
                  << " (USD)  | Jitter: " << std::setw(3) << cb_jit << "ms"
                  << (last_updater == ExchangeId::COINBASE ? "  <--" : "")
                  << "\n";
        std::cout << "------------------------------------------------\n";
        
        //latency
        std::cout << "STRATEGY COMPUTE: " << internal_lat_ns_ << "ns (" << (internal_lat_ns_ / 1000.00) << "us)\n";
        std::cout << "MAX LATENCY:      " << max_internal_lat_ns << "ns\n\n";

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