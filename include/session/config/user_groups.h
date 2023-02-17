#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "base.h"
#include "util.h"

typedef struct ugroups_legacy_group_info {
    char session_id[67];  // in hex; 66 hex chars + null terminator.

    char name[101];  // Null-terminated C string (human-readable).  Max length is 511.  Will always
                     // be set (even if an empty string).

    bool have_enc_keys;            // Will be true if we have an encryption keypair, false if not.
    unsigned char enc_pubkey[32];  // If `have_enc_keys`, this is the 32-byte pubkey
    unsigned char enc_seckey[32];  // If `have_enc_keys`, this is the 32-byte secret key

    int64_t disappearing_timer;  // Minutes. 0 == disabled.
    bool hidden;                 // true if hidden from the convo list
    int priority;  // pinned message priority; 0 = unpinned, larger means pinned higher (i.e. higher
                   // priority conversations come first).
} ugroups_legacy_group_info;

typedef struct ugroups_community_info {
    char base_url[268];  // null-terminated (max length 267), normalized (i.e. always lower-case,
                         // only has port if non-default, has trailing / removed)
    char room[65];       // null-terminated (max length 64); this is case-preserving (i.e. can be
                         // "SomeRoom" instead of "someroom".  Note this is different from volatile
                         // info (that one is always forced lower-cased).
    unsigned char pubkey[32];  // 32 bytes (not terminated, can contain nulls)

    int priority;  // pinned message priority; 0 = unpinned, larger means pinned higher (i.e. higher
                   // priority conversations come first).
} ugroups_community_info;

int user_groups_init(
        config_object** conf,
        const unsigned char* ed25519_secretkey,
        const unsigned char* dump,
        size_t dumplen,
        char* error) __attribute__((warn_unused_result));

/// Gets community conversation info into `comm`, if the community info was found.  `base_url` and
/// `room` are null-terminated c strings; pubkey is 32 bytes.  base_url will be
/// normalized/lower-cased; room is case-insensitive for the lookup: note that this may well return
/// a community info with a different room capitalization than the one provided to the call.
///
/// Returns true if the community was found and `comm` populated; false otherwise.
bool user_groups_get_community(
        const config_object* conf,
        ugroups_community_info* comm,
        const char* base_url,
        const char* room) __attribute__((warn_unused_result));

/// Like the above, but if the community was not found, this constructs one that can be inserted.
/// `base_url` will be normalized in the returned object.  `room` is a case-insensitive lookup key
/// for the room token.  Note that it has subtle handling w.r.t its case: if an existing room is
/// found, you get back a record with the found case (which could differ in case from what you
/// provided).  If you want to override to what you provided regardless of what is there you should
/// immediately set the name of the returned object to the case you prefer.  If a *new* record is
/// constructed, however, it will match the room token case as given here.
///
/// Note that this is all different from convo_info_volatile, which always forces the room token to
/// lower-case (because it does not preserve the case).
bool user_groups_get_or_construct_community(
        const config_object* conf,
        ugroups_community_info* comm,
        const char* base_url,
        const char* room,
        unsigned const char* pubkey) __attribute__((warn_unused_result));

/// Fills `group` with the conversation info given a legacy group ID (specified as a null-terminated
/// hex string), if the conversation exists, and returns true.  If the conversation does not exist
/// then `group` is left unchanged and false is returned.
bool user_groups_get_legacy_group(
        const config_object* conf, ugroups_legacy_group_info* group, const char* id)
        __attribute__((warn_unused_result));

/// Same as the above except that when the conversation does not exist, this sets all the group
/// fields to defaults and loads it with the given id.
///
/// Returns true as long as it is given a valid legacy group group id (i.e. same format as a session
/// id).  A false return is considered an error, and means the id was not a valid session id.
///
/// This is the method that should usually be used to create or update a conversation, followed by
/// setting fields in the group, and then giving it to user_groups_set().
bool user_groups_get_or_construct_legacy_group(
        const config_object* conf, ugroups_legacy_group_info* group, const char* id)
        __attribute__((warn_unused_result));

/// Adds or updates a conversation from the given group info
void user_groups_set_community(config_object* conf, const ugroups_community_info* group);
void user_groups_set_legacy_group(config_object* conf, const ugroups_legacy_group_info* group);

/// Erases a conversation from the conversation list.  Returns true if the conversation was found
/// and removed, false if the conversation was not present.  You must not call this during
/// iteration; see details below.
bool user_groups_erase_community(config_object* conf, const char* base_url, const char* room);
bool user_groups_erase_legacy_group(config_object* conf, const char* group_id);

/// Returns the number of conversations.
size_t user_groups_size(const config_object* conf);
/// Returns the number of conversations of the specific type.
size_t user_groups_size_communities(const config_object* conf);
size_t user_groups_size_legacy_group(const config_object* conf);

/// Functions for iterating through the entire conversation list.  Intended use is:
///
///     ugroups_community_info c2;
///     ugroups_legacy_group_info c3;
///     user_groups_iterator *it = user_groups_iterator_new(my_groups);
///     for (; !user_groups_iterator_done(it); user_groups_iterator_advance(it)) {
///         if (user_groups_it_is_community(it, &c2)) {
///             // use c2.whatever
///         } else if (user_groups_it_is_legacy_group(it, &c3)) {
///             // use c3.whatever
///         }
///     }
///     user_groups_iterator_free(it);
///
/// It is permitted to modify records (e.g. with a call to one of the `user_groups_set_*`
/// functions) and add records while iterating.
///
/// If you need to remove while iterating then usage is slightly different: you must advance the
/// iteration by calling either user_groups_iterator_advance if not deleting, or
/// user_groups_iterator_erase to erase and advance.  Usage looks like this:
///
///     ugroups_community_info comm;
///     ugroups_iterator *it = ugroups_iterator_new(my_groups);
///     while (!user_groups_iterator_done(it)) {
///         if (user_groups_it_is_community(it, &comm)) {
///             bool should_delete = /* ... */;
///             if (should_delete)
///                 user_groups_iterator_erase(it);
///             else
///                 user_groups_iterator_advance(it);
///         } else {
///             user_groups_iterator_advance(it);
///         }
///     }
///     user_groups_iterator_free(it);
///

typedef struct user_groups_iterator user_groups_iterator;

// Starts a new iterator that iterates over all conversations.
user_groups_iterator* user_groups_iterator_new(const config_object* conf);

// The same as `user_groups_iterator_new` except that this iterates *only* over one type of
// conversation. You still need to use `user_groups_it_is_community` (or the alternatives)
// to load the data in each pass of the loop.  (You can, however, safely ignore the bool return
// value of the `it_is_whatever` function: it will always be true for the particular type being
// iterated over).
user_groups_iterator* user_groups_iterator_new_communities(const config_object* conf);
user_groups_iterator* user_groups_iterator_new_legacy_groups(const config_object* conf);

// Frees an iterator once no longer needed.
void user_groups_iterator_free(user_groups_iterator* it);

// Returns true if iteration has reached the end.
bool user_groups_iterator_done(user_groups_iterator* it);

// Advances the iterator.
void user_groups_iterator_advance(user_groups_iterator* it);

// If the current iterator record is a community conversation this sets the details into `c` and
// returns true.  Otherwise it returns false.
bool user_groups_it_is_community(user_groups_iterator* it, ugroups_community_info* c);

// If the current iterator record is a legacy closed group conversation this sets the details into
// `c` and returns true.  Otherwise it returns false.
bool user_groups_it_is_legacy_closed(user_groups_iterator* it, ugroups_legacy_group_info* c);

// Erases the current group while advancing the iterator to the next group in the iteration.
void user_groups_iterator_erase(config_object* conf, user_groups_iterator* it);

#ifdef __cplusplus
}  // extern "C"
#endif
