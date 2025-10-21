#pragma once

#include <bezos/status.h>

#include "memory/range.hpp"
#include "std/rcuptr.hpp"
#include "std/std.hpp"

namespace obj {
    class Process;

    class AllocatePhysicalMemoryRequest {
        size_t mSize;
        size_t mAlign;
    public:
        AllocatePhysicalMemoryRequest();
        AllocatePhysicalMemoryRequest(size_t size, size_t align);
    };

    class AllocatePhysicalMemoryResult {
        km::MemoryRangeEx mRange;
    public:
        AllocatePhysicalMemoryResult();
        AllocatePhysicalMemoryResult(km::MemoryRangeEx range);

        km::MemoryRangeEx getRange() const { return mRange; }
    };

    class ReleasePhysicalMemoryRequest {
        km::MemoryRangeEx mRange;
    public:
        ReleasePhysicalMemoryRequest();
        ReleasePhysicalMemoryRequest(km::MemoryRangeEx range);
    };

    class ReleasePhysicalMemoryResult {
    public:
        ReleasePhysicalMemoryResult();
    };

    class RetainPhysicalMemoryRequest {
        km::MemoryRangeEx mRange;
    public:
        RetainPhysicalMemoryRequest();
        RetainPhysicalMemoryRequest(km::MemoryRangeEx range);

        km::MemoryRangeEx getRange() const { return mRange; }
    };

    class RetainPhysicalMemoryResult {
    public:
        RetainPhysicalMemoryResult();
    };

    /**
     * @brief Represents all physical memory available on the system.
     */
    class IPhysicalMemoryClient {
    public:
        virtual ~IPhysicalMemoryClient() = default;

        /**
         * @brief Allocate a region of physical memory.
         *
         * @param request The request parameters.
         * @param[out] result The result of the allocation.
         * @return OsStatus The status of the operation.
         */
        virtual OsStatus allocatePhysicalMemory(const AllocatePhysicalMemoryRequest& request, AllocatePhysicalMemoryResult *result [[outparam]]) = 0;

        /**
         * @brief Release a region of physical memory.
         *
         * @param request The request parameters.
         * @param[out] result The result of the release operation.
         * @return OsStatus The status of the operation.
         */
        virtual OsStatus releasePhysicalMemory(const ReleasePhysicalMemoryRequest& request, ReleasePhysicalMemoryResult *result [[outparam]]) = 0;

        /**
         * @brief Increment the reference count of a region of physical memory.
         *
         * @param request The request parameters.
         * @param[out] result The result of the acquire operation.
         * @return OsStatus The status of the operation.
         */
        virtual OsStatus retainPhysicalMemory(const RetainPhysicalMemoryRequest& request, RetainPhysicalMemoryResult *result [[outparam]]) = 0;
    };

    class IPhysicalMemoryService {
    public:
        virtual ~IPhysicalMemoryService() = default;

        virtual OsStatus createClient(sm::RcuSharedPtr<Process> process, IPhysicalMemoryClient **client [[outparam]]) noexcept = 0;
    };
}
