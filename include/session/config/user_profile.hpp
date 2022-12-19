#pragma once

#include <memory>
#include <session/config.hpp>

#include "base.hpp"
#include "namespaces.hpp"
#include "profile_pic.hpp"

namespace session::config {

/// keys used in this config, either currently or in the past (so that we don't reuse):
///
/// n - user profile name
/// p - user profile url
/// q - user profile decryption key (binary)

class UserProfile final : public ConfigBase {

  public:
    // No default constructor
    UserProfile() = delete;

    /// Constructs a user profile from existing data (stored from `dump()`) and the user's secret
    /// key for generating the data encryption key.  To construct a blank profile (i.e. with no
    /// pre-existing dumped data to load) pass `std::nullopt` as the second argument.
    ///
    /// \param ed25519_secretkey - contains the libsodium secret key used to encrypt/decrypt the
    /// data when pushing/pulling from the swarm.  This can either be the full 64-byte value (which
    /// is technically the 32-byte seed followed by the 32-byte pubkey), or just the 32-byte seed of
    /// the secret key.
    ///
    /// \param dumped - either `std::nullopt` to construct a new, empty object; or binary state data
    /// that was previously dumped from an instance of this class by calling `dump()`.
    UserProfile(ustring_view ed25519_secretkey, std::optional<ustring_view> dumped);

    Namespace storage_namespace() const override { return Namespace::UserProfile; }

    const char* encryption_domain() const override { return "UserProfile"; }

    /// Returns the user profile name, or std::nullopt if there is no profile name set.
    std::optional<std::string_view> get_name() const;

    /// Sets the user profile name; if given an empty string then the name is removed.
    void set_name(std::string_view new_name);

    /// Gets the user's current profile pic URL and decryption key.  Returns nullptr for *both*
    /// values if *either* value is unset or empty in the config data.
    std::optional<profile_pic> get_profile_pic() const;

    /// Sets the user's current profile pic to a new URL and decryption key.  Clears both if either
    /// one is empty.
    void set_profile_pic(std::string_view url, ustring_view key);
    void set_profile_pic(profile_pic pic);
};

}  // namespace session::config
