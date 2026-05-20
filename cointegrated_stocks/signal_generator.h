#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <string>
#include "trading_pair.h"

// Генерация торгового сигнала на основе Z-score
inline std::string generate_signal(const TradingPair& pair,
                                   const std::string& sym1,
                                   const std::string& sym2) {
    double z = pair.get_z_score();
    int hist_size = pair.get_spread_history_size();

    if (hist_size < 30) {
        return "CALIBRATING... Need " + std::to_string(30 - hist_size) +
               " more observations (current Z=" + std::to_string(z) + ")";
    }

    if (z > 2.0) {
        return "🔴 SELL " + sym1 + ", BUY " + sym2 + " (Z=" + std::to_string(z) + ")";
    }
    if (z < -2.0) {
        return "🟢 BUY " + sym1 + ", SELL " + sym2 + " (Z=" + std::to_string(z) + ")";
    }
    if (z > 1.0) {
        return "📈 " + sym1 + " overpriced (Z=" + std::to_string(z) + ")";
    }
    if (z < -1.0) {
        return "📉 " + sym2 + " overpriced (Z=" + std::to_string(z) + ")";
    }
    return "✅ Balanced (Z=" + std::to_string(z) + ")";
}

#endif // SIGNAL_GENERATOR_H
