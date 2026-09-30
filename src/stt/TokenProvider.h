#pragma once

#include <string>

namespace ss {

// Fetches a short-lived streaming token so the API key never sits in a WS URL
// query string. M1 uses the Authorization header instead; this is for the
// hardening pass (browser-driven demos, sharable builds).
class TokenProvider {
public:
    explicit TokenProvider(std::string api_key) : api_key_(std::move(api_key)) {}

    // POST https://streaming.assemblyai.com/v3/token -> {"token": "..."}
    // TODO(M6): implement via cpp-httplib; falls back to api_key_ for now.
    std::string temporaryToken(int ttl_seconds = 600) const {
        (void)ttl_seconds;
        return api_key_;
    }

private:
    std::string api_key_;
};

} // namespace ss
