#include "stt/TranscriptBuffer.h"

namespace ss {

void TranscriptBuffer::onTurn(const Turn& t) {
    std::lock_guard lk(m_);
    turns_[t.turn_order] = t;
}

void TranscriptBuffer::clear() {
    std::lock_guard lk(m_);
    turns_.clear();
}

std::vector<Turn> TranscriptBuffer::finalizedTurns() const {
    std::lock_guard lk(m_);
    std::vector<Turn> out;
    for (const auto& [_, t] : turns_)
        if (t.end_of_turn) out.push_back(t);
    return out;
}

std::vector<Turn> TranscriptBuffer::partialTurns() const {
    std::lock_guard lk(m_);
    std::vector<Turn> out;
    for (const auto& [_, t] : turns_)
        if (!t.end_of_turn) out.push_back(t);
    return out;
}

std::string TranscriptBuffer::finalizedText() const {
    std::lock_guard lk(m_);
    std::string out;
    for (const auto& [_, t] : turns_) {
        if (!t.end_of_turn || t.transcript.empty()) continue;
        if (!out.empty()) out += ' ';
        out += t.transcript;
    }
    return out;
}

} // namespace ss
