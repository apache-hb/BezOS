#pragma once

#include <bezos/status.h>
#include "common/util/util.hpp"
#include <functional>
#include "panic.hpp"

namespace sm {
    template<typename Key, typename Value, typename Compare = std::less<Key>>
    class StaticFlatMap;

    template<typename Key, typename Value>
    class StaticFlatMapIterator;

    /// @brief A flat map with a fixed max capacity.
    ///
    /// A flat map implemented using sorted arrays. The map has a fixed maximum capacity and does not allocate memory.
    ///
    /// @tparam Key The type of the keys in the map.
    /// @tparam Value The type of the values in the map.
    /// @tparam Compare The comparison function for the keys.
    template<typename Key, typename Value, typename Compare>
    class StaticFlatMap {
        Key *mKeys;
        Value *mValues;
        size_t mCapacity;
        size_t mCount;
        [[no_unique_address]] Compare mCompare;

        static constexpr size_t computeMaxKeyCount(size_t storageSize) noexcept {
            return storageSize / (sizeof(Key) + sizeof(Value));
        }

        void destroy() noexcept [[clang::nonallocating]] {
            std::destroy_n(mKeys, mCount);
            std::destroy_n(mValues, mCount);
            mCount = 0;
        }

    public:
        using Iterator = StaticFlatMapIterator<Key, Value>;

        friend Iterator;

        StaticFlatMap() noexcept [[clang::nonallocating]]
            : mKeys(nullptr)
            , mValues(nullptr)
            , mCapacity(0)
            , mCount(0)
            , mCompare(Compare{})
        { }

        /// @brief Construct a new StaticFlatMap.
        ///
        /// @param storage A pointer to the memory to use for the map. Must be at least computeRequiredStorage(1) bytes.
        /// @param size The size of the storage in bytes.
        /// @param compare The comparison function for the keys.
        StaticFlatMap(void *storage [[gnu::nonnull]], size_t size, Compare compare = Compare{}) noexcept [[clang::nonallocating]]
            : mKeys(static_cast<Key*>(storage))
            , mValues(reinterpret_cast<Value*>(mKeys + computeMaxKeyCount(size)))
            , mCapacity(computeMaxKeyCount(size))
            , mCount(0)
            , mCompare(compare)
        {
            KM_CHECK(size >= computeRequiredStorage(1), "Storage size is too small");
        }

        ~StaticFlatMap() noexcept {
            destroy();
        }

        UTIL_NOCOPY(StaticFlatMap);

        StaticFlatMap(StaticFlatMap&& other) noexcept
            : mKeys(std::exchange(other.mKeys, nullptr))
            , mValues(std::exchange(other.mValues, nullptr))
            , mCapacity(std::exchange(other.mCapacity, 0))
            , mCount(std::exchange(other.mCount, 0))
            , mCompare(std::move(other.mCompare))
        { }

        StaticFlatMap& operator=(StaticFlatMap&& other) noexcept {
            if (this != &other) {
                destroy();

                mKeys = std::exchange(other.mKeys, nullptr);
                mValues = std::exchange(other.mValues, nullptr);
                mCapacity = std::exchange(other.mCapacity, 0);
                mCount = std::exchange(other.mCount, 0);
                mCompare = std::move(other.mCompare);
            }
            return *this;
        }

        static constexpr size_t computeRequiredStorage(size_t capacity) noexcept [[clang::nonblocking]] {
            return (capacity * sizeof(Key)) + (capacity * sizeof(Value));
        }

        /// @brief Get the maximum capacity of the map.
        ///
        /// @return The maximum number of entries the map can hold.
        size_t capacity() const noexcept [[clang::nonblocking]] {
            return mCapacity;
        }

        /// @brief Get the current number of entries in the map.
        ///
        /// @return The number of entries in the map.
        size_t count() const noexcept [[clang::nonblocking]] {
            return mCount;
        }

        /// @brief Check if the map is empty.
        ///
        /// @return True if the map is empty, false otherwise.
        bool isEmpty() const noexcept [[clang::nonblocking]] {
            return mCount == 0;
        }

        /// @brief Check if the map is full.
        ///
        /// @return True if the map is full, false otherwise.
        bool isFull() const noexcept [[clang::nonblocking]] {
            return mCount == mCapacity;
        }

        /// @brief Clear all entries from the map.
        void clear() noexcept {
            destroy();
        }

        /// @brief Insert a key-value pair into the map.
        ///
        /// If the key already exists, the value is updated.
        /// If the map is full, an error is returned.
        ///
        /// @param key The key to insert.
        /// @param value The value to insert.
        ///
        /// @return Status of the operation.
        /// @retval OsStatusSuccess The key-value pair was inserted or updated successfully.
        /// @retval OsStatusOutOfMemory The map is full and cannot accept new entries.
        OsStatus insert(const Key& key, const Value& value) noexcept [[clang::nonblocking]] {
            if (isFull()) {
                return OsStatusOutOfMemory;
            }

            Key *iter = std::lower_bound(mKeys, mKeys + mCount, key, mCompare);
            if (iter != mKeys + mCount && !mCompare(key, *iter) && !mCompare(*iter, key)) {
                // Key already exists, update the value
                size_t index = std::distance(mKeys, iter);
                mValues[index] = value;
                return OsStatusSuccess;
            }

            size_t index = std::distance(mKeys, iter);

            for (size_t i = mCount; i > index; i--) {
                mKeys[i] = mKeys[i - 1];
                mValues[i] = mValues[i - 1];
            }

            mKeys[index] = key;
            mValues[index] = value;
            mCount += 1;

            return OsStatusSuccess;
        }

        /// @brief Remove a key-value pair from the map.
        ///
        /// If the key does not exist, an error is returned.
        ///
        /// @param key The key to remove.
        ///
        /// @return Status of the operation.
        /// @retval OsStatusSuccess The key-value pair was removed successfully.
        /// @retval OsStatusNotFound The key was not found in the map.
        OsStatus remove(const Key& key) noexcept [[clang::nonblocking]] {
            Key *iter = std::lower_bound(mKeys, mKeys + mCount, key, mCompare);
            if (iter == mKeys + mCount || mCompare(key, *iter) || mCompare(*iter, key)) {
                return OsStatusNotFound;
            }

            size_t index = std::distance(mKeys, iter);
            for (size_t i = index; i < mCount - 1; i++) {
                mKeys[i] = mKeys[i + 1];
                mValues[i] = mValues[i + 1];
            }

            mCount -= 1;

            return OsStatusSuccess;
        }

        /// @brief Find a value by its key.
        ///
        /// @param key The key to find.
        ///
        /// @return A pointer to the value if found, nullptr otherwise.
        Value *find(const Key& key) noexcept [[clang::nonblocking]] {
            Key *iter = std::lower_bound(mKeys, mKeys + mCount, key, mCompare);
            if (iter == mKeys + mCount || mCompare(key, *iter) || mCompare(*iter, key)) {
                return nullptr;
            }

            size_t index = std::distance(mKeys, iter);
            return &mValues[index];
        }

        /// @brief Find a value by its key (const version).
        ///
        /// @param key The key to find.
        ///
        /// @return A pointer to the value if found, nullptr otherwise.
        const Value *find(const Key& key) const noexcept {
            const Key *iter = std::lower_bound(mKeys, mKeys + mCount, key, mCompare);
            if (iter == mKeys + mCount || mCompare(key, *iter) || mCompare(*iter, key)) {
                return nullptr;
            }

            size_t index = std::distance(mKeys, iter);
            return &mValues[index];
        }

        /// @brief Get an iterator to the beginning of the map.
        ///
        /// @return An iterator to the beginning of the map.
        Iterator begin() noexcept [[clang::nonblocking]] {
            return Iterator{this, 0};
        }

        /// @brief Get an iterator to the end of the map.
        ///
        /// @return An iterator to the end of the map.
        Iterator end() noexcept [[clang::nonblocking]] {
            return Iterator{this, mCount};
        }

        Iterator upperBound(const Key& key) noexcept [[clang::nonblocking]] {
            Key *iter = std::upper_bound(mKeys, mKeys + mCount, key, mCompare);
            size_t index = std::distance(mKeys, iter);
            return Iterator{this, index};
        }

        Iterator lowerBound(const Key& key) noexcept [[clang::nonblocking]] {
            Key *iter = std::lower_bound(mKeys, mKeys + mCount, key, mCompare);
            size_t index = std::distance(mKeys, iter);
            return Iterator{this, index};
        }

        friend void swap(StaticFlatMap& a, StaticFlatMap& b) noexcept [[clang::nonblocking]] {
            std::swap(a.mKeys, b.mKeys);
            std::swap(a.mValues, b.mValues);
            std::swap(a.mCapacity, b.mCapacity);
            std::swap(a.mCount, b.mCount);
            std::swap(a.mCompare, b.mCompare);
        }
    };

    /// @brief An iterator for @a StaticFlatMap.
    ///
    /// @tparam Key The type of the keys in the map.
    /// @tparam Value The type of the values in the map.
    template<typename Key, typename Value>
    class StaticFlatMapIterator {
        using Container = StaticFlatMap<Key, Value>;

        Container *mContainer;
        size_t mIndex;

    public:
        /// @brief Construct an iterator for a given container and index.
        ///
        /// @param container The container to iterate over.
        /// @param index The starting index for the iterator.
        StaticFlatMapIterator(Container *container, size_t index) noexcept
            : mContainer(container)
            , mIndex(index)
        { }

        /// @brief Dereference the iterator to get the current key-value pair.
        ///
        /// @return A pair containing a reference to the key and a reference to the value.
        std::pair<const Key&, Value&> operator*() noexcept [[clang::nonblocking]] {
            return {mContainer->mKeys[mIndex], mContainer->mValues[mIndex]};
        }

        /// @brief Dereference the iterator to get the current key-value pair (const version).
        ///
        /// @return A pair containing a reference to the key and a reference to the value.
        std::pair<const Key&, const Value&> operator*() const noexcept [[clang::nonblocking]] {
            return {mContainer->mKeys[mIndex], mContainer->mValues[mIndex]};
        }

        const Key& key() const noexcept {
            return mContainer->mKeys[mIndex];
        }

        Value& value() noexcept {
            return mContainer->mValues[mIndex];
        }

        /// @brief Increment the iterator to point to the next element.
        ///
        /// @return A reference to the incremented iterator.
        StaticFlatMapIterator& operator++() noexcept {
            mIndex += 1;
            return *this;
        }

        /// @brief Compare two iterators for equality.
        ///
        /// @param other The other iterator to compare with.
        ///
        /// @return True if the iterators are equal, false otherwise.
        bool operator==(const StaticFlatMapIterator& other) const noexcept [[clang::nonblocking]] {
            return mContainer == other.mContainer && mIndex == other.mIndex;
        }

        /// @brief Compare two iterators for inequality.
        ///
        /// @param other The other iterator to compare with.
        ///
        /// @return True if the iterators are not equal, false otherwise.
        bool operator!=(const StaticFlatMapIterator& other) const noexcept [[clang::nonblocking]] {
            return mContainer != other.mContainer || mIndex != other.mIndex;
        }
    };
}
