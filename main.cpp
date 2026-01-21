#include "MarketDataConnector.hpp"
#include "CoinbaseConnector.hpp"
#include "BinanceConnector.hpp"
#include "ArbitrageStrategy.hpp"
#include <iostream>

int main() {
    net::io_context ioc;

    ssl::context ctx(ssl::context::tlsv12_client);

    ctx.set_verify_mode(ssl::verify_none);

    auto queue = std::make_shared<RingBuffer<Trade>>(4096);

    auto binance = std::make_shared<BinanceConnector>(ioc, ctx, queue);
    binance->run("data-stream.binance.com", "9443", "/ws/btcusdt@trade");

    auto coinbase = std::make_shared<CoinbaseConnector>(ioc, ctx, queue);
    coinbase->run("ws-feed.exchange.coinbase.com", "443", "/");

    std::thread consumer_thread([queue]() {
        ArbtirageStrategy strategy;
        while (true) {
            Trade t;
            if (queue->dequeue(t)) {
                
                strategy.on_trade(t);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });

    ioc.run();
    return 0;
}