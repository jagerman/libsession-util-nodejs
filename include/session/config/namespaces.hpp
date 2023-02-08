#pragma once

#include <cstdint>

namespace session::config {

enum class Namespace : std::int16_t {
    UserProfile = 2,
    Contacts = 3,
    ConvoInfoVolatile = 4,
    ClosedGroupInfo = 11,
};

}  // namespace session::config
