#include "MarketDataConnector.hpp"
#include "CoinbaseConnector.hpp"
#include "BinanceConnector.hpp"
#include <iostream>

int main() {
    net::io_context ioc;

    ssl::context ctx(ssl::context::tlsv12_client);

    ctx.set_verify_mode(ssl::verify_none);

    auto queue = std::make_shared<RingBuffer<Trade>>(4096);

    auto binance = std::make_shared<BinanceConnector>(ioc, ctx, queue);
    binance->run("data-stream.binance.vision", "9443", "/ws/btcusdt@trade");

    auto coinbase = std::make_shared<CoinbaseConnector>(ioc, ctx, queue);
    coinbase->run("advanced-trade-ws.coinbase.com", "443", "/");

    std::thread consumer_thread([queue]() {
        Trade t;
        while (true) {
            if (queue->dequeue(t)) {

                int64_t latency = t.event_time - t.trade_time;
                std::cout << std::fixed << std::setprecision(8)
                          << "[Consumer] " << t.symbol
                          << " | P: " << t.price
                          << " | Q: " << t.quantity
                          << " | Latency: " << (t.event_time - t.trade_time) << "ms"
                          << std::endl;
            } else {
                std::this_thread::yield();
            }
        }
    });

    ioc.run();
    return 0;
}