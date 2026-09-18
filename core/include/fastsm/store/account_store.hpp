#pragma once

#include <memory>
#include <string>
#include <vector>

#include "fastsm/net/http_client.hpp"
#include "fastsm/platform/social_account.hpp"
#include "fastsm/store/app_config.hpp"

namespace fastsm {

// In-memory roster of live accounts plus the selection. Reconstructs accounts
// from persisted config (Bluesky re-establishes a session over the network) and
// can serialize back to an AppConfig for saving.
class AccountStore {
public:
    explicit AccountStore(net::IHttpClient* http);

    void load(const store::AppConfig& config); // rebuild from disk
    void add(std::unique_ptr<SocialAccount> account, store::StoredCredential credential);
    void remove(const std::string& account_key);
    void select(const std::string& account_key);

    bool empty() const { return entries_.empty(); }
    SocialAccount* selected() const;
    std::vector<SocialAccount*> accounts() const;
    const std::string& selected_key() const { return selected_key_; }

    store::AppConfig to_config() const;

private:
    struct Entry {
        std::unique_ptr<SocialAccount> account;
        store::StoredCredential credential;
    };

    net::IHttpClient* http_;
    std::vector<Entry> entries_;
    // Accounts removed from the active roster (logout, or a reload) are moved here
    // rather than freed, because a background worker task (a refresh, a post) may
    // still hold the raw SocialAccount* through its TimelineController. They live
    // until the store is destroyed — which, by CoreSession's member order, happens
    // only after the worker queues have drained. Keeps the roster's account
    // pointers valid for the whole session (mirrors CoreSession's retired_).
    std::vector<std::unique_ptr<SocialAccount>> retained_;
    std::string selected_key_;
};

} // namespace fastsm
