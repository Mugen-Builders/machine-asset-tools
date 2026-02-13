
#ifndef INTERPROCESS_HPP
#define INTERPROCESS_HPP

// These are the minimum compiler versions verified to work
#if defined(__clang__) && defined(__clang_major__)
#if __clang_major__ < 19
#error "Clang 19 or newer is required"
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#if __GNUC__ < 14
#error "GCC 14 or newer is required"
#endif
#elif !defined(__cppcheck__)
#error "Unsupported compiler: GCC 14+ or Clang 19+ required"
#endif

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp> // IWYU pragma: keep
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/unordered/unordered_flat_map.hpp> // IWYU pragma: keep
#include <boost/unordered/unordered_flat_set.hpp> // IWYU pragma: keep
#include <boost/unordered/unordered_node_map.hpp> // IWYU pragma: keep
#include <boost/interprocess/containers/string.hpp>
#include <boost/version.hpp>

// Fixes misc-include-cleaner check
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/segment_manager_helper.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/predef.h>
#include <boost/unordered/unordered_flat_map_fwd.hpp>
#include <boost/unordered/unordered_flat_set_fwd.hpp>
#include <boost/unordered/unordered_node_map_fwd.hpp>

// Older versions of Boost.Interprocess have alignment issues, ensure it is recent
constexpr auto MINIMUM_BOOST_VERSION = 108900;
static_assert(BOOST_VERSION >= MINIMUM_BOOST_VERSION, "Boost 1.89.0 or higher required");

namespace libcma::interprocess {

// Ensure we are using a compatible 64-bit architecture with little-endian byte ordering.
// This is necessary because interprocess data structures must have consistent memory layout
// when their state is accessed across different architecture processes.
static_assert(sizeof(std::size_t) == sizeof(std::uint64_t) && sizeof(std::ptrdiff_t) == sizeof(std::uint64_t) &&
        sizeof(std::uintptr_t) == sizeof(std::uint64_t) && sizeof(void *) == sizeof(std::uint64_t) &&
        sizeof(decltype(0L)) == sizeof(std::uint64_t),
    "code assumes 64-bit architecture");
static_assert(std::endian::native == std::endian::little, "code assumes little-endian byte order");

using managed_memory = boost::interprocess::managed_mapped_file;
using void_allocator = boost::interprocess::allocator<void, managed_memory::segment_manager>;

using boost::interprocess::open_or_create;
using boost::interprocess::unique_instance;

using basic_string = boost::interprocess::basic_string<char, std::char_traits<char>, void_allocator>;

template <typename PointedType>
using offset_ptr = boost::interprocess::offset_ptr<PointedType, std::int64_t, std::uint64_t, sizeof(std::uint64_t)>;

/// @brief Hasher for offset pointers.
class offset_ptr_hasher {
public:
    /// @brief Hash an offset pointer.
    /// @param ptr The offset pointer to hash.
    /// @returns The hash value.
    template <typename PointedType>
    constexpr auto operator()(
        const boost::interprocess::offset_ptr<PointedType, std::int64_t, std::uint64_t, sizeof(std::uint64_t)> &ptr)
        const noexcept -> std::size_t {
        // Hash the offset to the hasher pointer,
        // this should be always reproducible if the hasher is also allocated in same interprocess mapped memory
        const auto offset = reinterpret_cast<std::intptr_t>(ptr.get()) - reinterpret_cast<std::intptr_t>(this);
        return boost::hash<std::intptr_t>{}(offset);
    }
};

template <typename Key, typename Value, class Hash = boost::hash<Key>, class KeyEqual = std::equal_to<>>
using unordered_node_map = boost::unordered_node_map<Key, Value, Hash, KeyEqual,
    boost::interprocess::allocator<std::pair<const Key, Value>, managed_memory::segment_manager>>;

template <typename Key, typename Value, class Hash = boost::hash<Key>, class KeyEqual = std::equal_to<>>
using unordered_flat_map = boost::unordered_flat_map<Key, Value, Hash, KeyEqual,
    boost::interprocess::allocator<std::pair<const Key, Value>, managed_memory::segment_manager>>;

template <typename Key, class Hash = boost::hash<Key>, class KeyEqual = std::equal_to<>>
using unordered_flat_set = boost::unordered_flat_set<Key, Hash, KeyEqual,
    boost::interprocess::allocator<Key, managed_memory::segment_manager>>;

template <typename Key, typename Value, class Compare>
using flat_map = boost::container::flat_map<Key, Value, Compare,
    boost::interprocess::allocator<std::pair<Key, Value>, managed_memory::segment_manager>>;

template <typename Key, class Compare>
using flat_set =
    boost::container::flat_set<Key, Compare, boost::interprocess::allocator<Key, managed_memory::segment_manager>>;

using boost::unordered::erase_if;

inline auto remove(const char *file_name) -> void {
    boost::interprocess::file_mapping::remove(file_name);
}

} // namespace libcma::interprocess

#endif // INTERPROCESS_HPP
