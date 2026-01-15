#include "MarketDataConnector.hpp"
#include <iostream>

int main() {
    try {
        net::io_context ioc;

        ssl::context ctx(ssl::context::tlsv12_client);

        ctx.set_verify_mode(ssl::verify_none);

        auto bot = std::make_shared<MarketDataConnector>(ioc, ctx);

        bot->run("data-stream.binance.vision", "9443");

        ioc.run();
    } catch (std::exception const& e) {
        std::cerr<< "Error: " << e.what() << std::endl;
    }

    return 0;
}