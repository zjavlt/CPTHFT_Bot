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
    binance->run("data-stream.binance.com", "9443", "/ws/btcusdt@trade");

    auto coinbase = std::make_shared<CoinbaseConnector>(ioc, ctx, queue);
    coinbase->run("ws-feed.exchange.coinbase.com", "443", "/");

    std::thread consumer_thread([queue]() {
        Trade t;
        while (true) {
            if (queue->dequeue(t)) {
                std::string ex_name = (t.exchange == ExchangeId::BINANCE) ? "BIN" : "C B";

                // int64_t latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                //     std::chrono::system_clock::now().time_since_epoch()
                // ).count() - t.trade_time;
                int64_t serverLat = t.event_time - t.trade_time;
                std::cout << std::fixed << std::setprecision(8)
                          << "[" << ex_name << "] "
                          << t.symbol
                          << " | P: " << t.price
                          << " | Q: " << t.quantity
                        //   << " | Lat: " << latency << "ms"
                          << " | Server Lat: " << serverLat << "ms"
                          << " | Time: " << t.event_time
                          // WSL time delay not resolvable
                          // TODO: integrate internal delay calculation for latency (if heavy compute -> drop)
                          << std::endl;
            } else {
                std::this_thread::yield();
            }
        }
    });

    ioc.run();
    return 0;
}