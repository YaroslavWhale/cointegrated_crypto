#include "strategy/pair_trading_strategy.hpp"
#include <iostream>
#include <cmath>

PairTradingStrategy::PairTradingStrategy(
    std::unique_ptr<IStateEstimator> est,
    std::unique_ptr<SpreadAnalyzer> anal,
    std::unique_ptr<IPortfolio> port)
    : estimator_(std::move(est)), analyzer_(std::move(anal)),
    portfolio_(std::move(port)) {}

void PairTradingStrategy::on_price_update(double price1, double price2, uint64_t) {
    double log1 = std::log(price1), log2 = std::log(price2);
    estimator_->update(log1, log2);


    double spread = estimator_->get_spread();
    std::string signal = analyzer_->add_spread(spread);
    double z = analyzer_->z_score();

    std::cout << "Prices: " << price1 << " " << price2
              << " | spread=" << spread << " z=" << z
              << " | " << signal << std::endl;

    if (portfolio_->has_position() && std::abs(z) < 0.5) {
        std::cout << "[CLOSE] z near zero, closing\n";
        portfolio_->close_at_market(price1, price2);
        portfolio_->print_status(price1, price2);
        return;
    }

    if (signal == "SELL_SYM1" || signal == "BUY_SYM1") {
        portfolio_->process_signal(signal, price1, price2);
        portfolio_->print_status(price1, price2);
    }
}
