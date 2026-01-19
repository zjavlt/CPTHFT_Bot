#include "MarketDataConnector.hpp"
#include <iostream>

int main() {
    net::io_context ioc;

    ssl::context ctx(ssl::context::tlsv12_client);

    ctx.set_verify_mode(ssl::verify_none);

    auto queue = std::make_shared<RingBuffer<Trade>>(4096);

    auto bot = std::make_shared<MarketDataConnector>(ioc, ctx, queue);
    bot->run("data-stream.binance.vision", "9443");

    std::thread consumer_threaD([queue]() {
        Trade t;
        while (true) {
            if (queue->dequeue(t)) {
                std::cout << "[Consumer] " << t.symbol
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