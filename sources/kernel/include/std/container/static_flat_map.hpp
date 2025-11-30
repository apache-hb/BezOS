#pragma once

#include <bezos/status.h>
#include "common/util/util.hpp"
#include "panic.hpp"

#include <functional>

namespace sm {
    template<typename Key, typename Value, typename Compare = std::less<Key>>
    class StaticFlatMap;

    template<typename Key, typename Value, bool Mutable>
    class StaticFlatMapIterator;

    /// @brief A flat map with a fixed max capacity.
    ///
    /// A flat map implemented using sorted arrays. The map has a fixed maximum capacity and does not allocate memory.
    /// The storage for the map must be provided by the caller and must remain valid for the lifetime of the map.
    ///
    /// @tparam Key The type of the keys in the map.
    /// @tparam Value The type of the values in the map.
    /// @tparam Compare The comparison function for the keys.
    template<typename Key, typename Value, typename Compare>
    class StaticFlatMap {
        std::byte *mStorage;
        size_t mStorageSize;
        size_t mCount;
        [[no_unique_address]] Compare mCompare;

        static constexpr size_t computeMaxKeyCount(size_t storageSize) noexcept {
            return storageSize / (sizeof(Key) + sizeof(Value));
        }

        constexpr size_t computeValueOffset() const noexcept {
            return sm::roundup(computeMaxKeyCount(mStorageSize) * sizeof(Key), alignof(Value));
        }

        constexpr void destroy() noexcept [[clang::nonallocating]] {
            std::destroy_n(keys(), mCount);
            std::destroy_n(values(), mCount);
            mCount = 0;
        }

        Key *keys() noexcept [[clang::nonblocking]] {
            return std::bit_cast<Key*>(mStorage);
        }

        const Key *keys() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<const Key*>(mStorage);
        }

        Value *values() noexcept [[clang::nonblocking]] {
            return std::bit_cast<Value*>(mStorage + computeValueOffset());
        }

        const Value *values() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<const Value*>(mStorage + computeValueOffset());
        }

        Key& keyAt(size_t index) noexcept [[clang::nonblocking]] {
            KM_CHECK(index < count(), "Index out of bounds");
            return keys()[index];
        }

        const Key& keyAt(size_t index) const noexcept [[clang::nonblocking]] {
            KM_CHECK(index < count(), "Index out of bounds");
            return keys()[index];
        }

        Value& valueAt(size_t index) noexcept [[clang::nonblocking]] {
            KM_CHECK(index < count(), "Index out of bounds");
            return values()[index];
        }

        const Value& valueAt(size_t index) const noexcept [[clang::nonblocking]] {
            KM_CHECK(index < count(), "Index out of bounds");
            return values()[index];
        }

        auto *findImpl(this auto&& self, const Key& key) noexcept [[clang::nonblocking]] {
            auto allKeys = self.keys();

            auto iter = std::lower_bound(allKeys, allKeys + self.count(), key, self.mCompare);
            if (iter == allKeys + self.count() || self.mCompare(key, *iter) || self.mCompare(*iter, key)) {
                return (decltype(self.values()))nullptr;
            }

            size_t index = std::distance(allKeys, iter);
            return &self.valueAt(index);
        }

        size_t upperBoundImpl(this auto&& self, const Key& key) noexcept [[clang::nonblocking]] {
            auto allKeys = self.keys();

            auto iter = std::upper_bound(allKeys, allKeys + self.count(), key, self.mCompare);
            return std::distance(allKeys, iter);
        }

        size_t lowerBoundImpl(this auto&& self, const Key& key) noexcept [[clang::nonblocking]] {
            auto allKeys = self.keys();

            auto iter = std::lower_bound(allKeys, allKeys + self.count(), key, self.mCompare);
            return std::distance(allKeys, iter);
        }

    public:
        using Iterator = StaticFlatMapIterator<Key, Value, true>;
        using ConstIterator = StaticFlatMapIterator<Key, Value, false>;

        friend Iterator;
        friend ConstIterator;

        constexpr StaticFlatMap() noexcept [[clang::nonallocating]]
            : mStorage(nullptr)
            , mStorageSize(0)
            , mCount(0)
            , mCompare(Compare{})
        { }

        /// @brief Construct a new StaticFlatMap.
        ///
        /// @param storage A pointer to the memory to use for the map. Must be at least computeRequiredStorage(1) bytes.
        /// @param size The size of the storage in bytes.
        /// @param compare The comparison function for the keys.
        StaticFlatMap(void *storage [[gnu::nonnull]], size_t size, Compare compare = Compare{}) noexcept [[clang::nonallocating]]
            : mStorage((std::byte*)storage)
            , mStorageSize(size)
            , mCount(0)
            , mCompare(compare)
        {
            KM_CHECK(size >= computeRequiredStorage(1), "Storage size is too small");
        }

        constexpr ~StaticFlatMap() noexcept {
            destroy();
        }

        UTIL_NOCOPY(StaticFlatMap);

        constexpr StaticFlatMap(StaticFlatMap&& other) noexcept
            : mStorage(std::exchange(other.mStorage, nullptr))
            , mStorageSize(std::exchange(other.mStorageSize, 0))
            , mCount(std::exchange(other.mCount, 0))
            , mCompare(std::move(other.mCompare))
        { }

        constexpr StaticFlatMap& operator=(StaticFlatMap&& other) noexcept {
            if (this != &other) {
                destroy();

                mStorage = std::exchange(other.mStorage, nullptr);
                mStorageSize = std::exchange(other.mStorageSize, 0);
                mCount = std::exchange(other.mCount, 0);
                mCompare = std::move(other.mCompare);
            }
            return *this;
        }

        /**
         * @brief Calculate the required storage size for a given capacity.
         *
         * @param capacity The desired capacity.
         * @return The required storage size in bytes.
         */
        static constexpr size_t computeRequiredStorage(size_t capacity) noexcept [[clang::nonblocking]] {
            return (capacity * sizeof(Key)) + (capacity * sizeof(Value));
        }

        /// @brief Get the maximum capacity of the map.
        ///
        /// @return The maximum number of entries the map can hold.
        constexpr size_t capacity() const noexcept [[clang::nonblocking]] {
            return computeMaxKeyCount(mStorageSize);
        }

        /// @brief Get the current number of entries in the map.
        ///
        /// @return The number of entries in the map.
        constexpr size_t count() const noexcept [[clang::nonblocking]] {
            return mCount;
        }

        /// @brief Check if the map is empty.
        ///
        /// @return True if the map is empty, false otherwise.
        constexpr bool isEmpty() const noexcept [[clang::nonblocking]] {
            return mCount == 0;
        }

        /// @brief Check if the map is full.
        ///
        /// @return True if the map is full, false otherwise.
        constexpr bool isFull() const noexcept [[clang::nonblocking]] {
            return mCount == capacity();
        }

        /// @brief Clear all entries from the map.
        constexpr void clear() noexcept {
            destroy();
        }

        /// @brief Insert a key-value pair into the map.
        ///
        /// If the key already exists, the value is updated.
        /// If the map is full, an error is returned.
        /// Invalidates any existing iterators.
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

            Key *allKeys = keys();
            Value *allValues = values();

            Key *iter = std::lower_bound(allKeys, allKeys + mCount, key, mCompare);
            if (iter != allKeys + mCount && !mCompare(key, *iter) && !mCompare(*iter, key)) {
                // Key already exists, update the value
                size_t index = std::distance(allKeys, iter);
                valueAt(index) = value;
                return OsStatusSuccess;
            }

            size_t index = std::distance(allKeys, iter);

            std::move_backward(allKeys + index, allKeys + mCount, allKeys + mCount + 1);
            std::move_backward(allValues + index, allValues + mCount, allValues + mCount + 1);

            mCount += 1;

            std::construct_at(&keyAt(index), key);
            std::construct_at(&valueAt(index), value);

            return OsStatusSuccess;
        }

        /// @brief Remove a key-value pair from the map.
        ///
        /// If the key does not exist, an error is returned.
        /// Invalidates any existing iterators.
        ///
        /// @param key The key to remove.
        ///
        /// @return Status of the operation.
        /// @retval OsStatusSuccess The key-value pair was removed successfully.
        /// @retval OsStatusNotFound The key was not found in the map.
        OsStatus remove(const Key& key) noexcept [[clang::nonblocking]] {
            Key *allKeys = keys();
            Value *allValues = values();

            Key *iter = std::lower_bound(allKeys, allKeys + mCount, key, mCompare);
            if (iter == allKeys + mCount || mCompare(key, *iter) || mCompare(*iter, key)) {
                return OsStatusNotFound;
            }

            size_t index = std::distance(allKeys, iter);

            std::move(allKeys + index + 1, allKeys + mCount, allKeys + index);
            std::move(allValues + index + 1, allValues + mCount, allValues + index);

            mCount -= 1;

            return OsStatusSuccess;
        }

        /// @brief Find a value by its key.
        ///
        /// @param key The key to find.
        ///
        /// @return A pointer to the value if found, nullptr otherwise.
        Value *find(const Key& key) noexcept [[clang::nonblocking]] {
            return findImpl(key);
        }

        /// @brief Find a value by its key (const version).
        ///
        /// @param key The key to find.
        ///
        /// @return A pointer to the value if found, nullptr otherwise.
        const Value *find(const Key& key) const noexcept {
            return findImpl(key);
        }

        /// @brief Get an iterator to the beginning of the map.
        ///
        /// @return An iterator to the beginning of the map.
        ConstIterator begin() const noexcept [[clang::nonblocking]] {
            return ConstIterator{this, 0};
        }

        /// @brief Get an iterator to the end of the map.
        ///
        /// @return An iterator to the end of the map.
        ConstIterator end() const noexcept [[clang::nonblocking]] {
            return ConstIterator{this, mCount};
        }

        ConstIterator upperBound(const Key& key) const noexcept [[clang::nonblocking]] {
            return ConstIterator{this, upperBoundImpl(key)};
        }

        ConstIterator lowerBound(const Key& key) const noexcept [[clang::nonblocking]] {
            return ConstIterator{this, lowerBoundImpl(key)};
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
            return Iterator{this, upperBoundImpl(key)};
        }

        Iterator lowerBound(const Key& key) noexcept [[clang::nonblocking]] {
            return Iterator{this, lowerBoundImpl(key)};
        }

        friend void swap(StaticFlatMap& a, StaticFlatMap& b) noexcept [[clang::nonblocking]] {
            std::swap(a.mStorage, b.mStorage);
            std::swap(a.mStorageSize, b.mStorageSize);
            std::swap(a.mCount, b.mCount);
            std::swap(a.mCompare, b.mCompare);
        }
    };

    /// @brief An iterator for @a StaticFlatMap.
    ///
    /// @tparam Key The type of the keys in the map.
    /// @tparam Value The type of the values in the map.
    /// @tparam Mutable Whether the iterator allows modification of the values.
    template<typename Key, typename Value, bool Mutable>
    class StaticFlatMapIterator {
        using Container = StaticFlatMap<Key, Value>;
        using Pointer = std::conditional_t<Mutable, Container*, const Container*>;

        Pointer mContainer;
        size_t mIndex;

    public:
        /// @brief Construct an iterator for a given container and index.
        ///
        /// @param container The container to iterate over.
        /// @param index The starting index for the iterator.
        StaticFlatMapIterator(Pointer container, size_t index) noexcept [[clang::nonblocking]]
            : mContainer(container)
            , mIndex(index)
        { }

        /// @brief Dereference the iterator to get the current key-value pair.
        ///
        /// @return A pair containing a reference to the key and a reference to the value.
        std::pair<const Key&, Value&> operator*() noexcept [[clang::nonblocking]] requires (Mutable) {
            return {mContainer->keyAt(mIndex), mContainer->valueAt(mIndex)};
        }

        /// @brief Dereference the iterator to get the current key-value pair (const version).
        ///
        /// @return A pair containing a reference to the key and a reference to the value.
        std::pair<const Key&, const Value&> operator*() const noexcept [[clang::nonblocking]] {
            return {mContainer->keyAt(mIndex), mContainer->valueAt(mIndex)};
        }

        const Key& key() const noexcept [[clang::nonblocking]] {
            return mContainer->keyAt(mIndex);
        }

        Value& value() noexcept [[clang::nonblocking]] requires (Mutable) {
            return mContainer->valueAt(mIndex);
        }

        const Value& value() const noexcept [[clang::nonblocking]] {
            return mContainer->valueAt(mIndex);
        }

        /// @brief Increment the iterator to point to the next element.
        ///
        /// @return A reference to the incremented iterator.
        StaticFlatMapIterator& operator++() noexcept [[clang::nonblocking]] {
            mIndex += 1;
            return *this;
        }

        /// @brief Decrement the iterator to point to the previous element.
        ///
        /// @return A reference to the decremented iterator.
        StaticFlatMapIterator& operator--() noexcept [[clang::nonblocking]] {
            mIndex -= 1;
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
