#pragma once
#include "MarketDataConnector.hpp"

enum class Tier {
    REGULAR,
    VIP
};

class FeeCalculator {
private:
    // regular price
    const double CB_TAKER_FEE = 0.0060;
    const double BIN_TAKER_FEE = 0.0010;

    //VIP fees
    const double VIP_CB_TAKER = 0.0005;
    const double VIP_BIN_TAKER = 0.0001725;

public:
    double calc_net_profit(double buy_price, double sell_price, bool buy_on_bin, Tier tier) {
        double fee_buy = 0.0;
        double fee_sell = 0.0;

        if (tier == Tier::REGULAR) {
            fee_buy = buy_on_bin ? BIN_TAKER_FEE : CB_TAKER_FEE;
            fee_sell = buy_on_bin ? CB_TAKER_FEE : BIN_TAKER_FEE;
        } else {
            fee_buy = buy_on_bin ? VIP_BIN_TAKER : VIP_CB_TAKER;
            fee_sell = buy_on_bin ? VIP_CB_TAKER : VIP_BIN_TAKER;
        }

        double cost = buy_price * (1.0 + fee_buy);
        double revenue = sell_price * (1.0 - fee_sell);

        return revenue - cost;
    }
};