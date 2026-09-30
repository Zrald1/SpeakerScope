#pragma once

#include <map>
#include <mutex>
#include <vector>
#include "core/Types.h"

namespace ss {

// Thread-safe store of Turn events keyed by turn_order.
// Partial turns overwrite earlier partials of the same turn_order.
class TranscriptBuffer {
public:
    void onTurn(const Turn& t);
    void clear();

    std::vector<Turn> finalizedTurns() const;
    std::vector<Turn> partialTurns() const;  // latest non-final turn(s)
    std::string finalizedText() const;

private:
    mutable std::mutex m_;
    std::map<uint32_t, Turn> turns_;
};

} // namespace ss
