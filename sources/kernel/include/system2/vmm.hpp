#pragma once

#include "std/rcuptr.hpp"
#include "std/std.hpp"
#include "common/physical_address.hpp"

#include <bezos/status.h>

namespace obj {
    class Process;

    class CreateVirtualMemoryRequest {

    };

    class CreateVirtualMemoryResult {

    };

    class ReleaseVirtualMemoryRequest {

    };

    class ReleaseVirtualMemoryResult {

    };

    class MapVirtualMemoryRequest {

    };

    class MapVirtualMemoryResult {

    };

    class GetPageTableAddressRequest {
    public:
        GetPageTableAddressRequest();
    };

    class GetPageTableAddressResult {
        sm::PhysicalAddress mAddress;
    public:
        GetPageTableAddressResult();
        GetPageTableAddressResult(sm::PhysicalAddress address);

        sm::PhysicalAddress getAddress() const { return mAddress; }
    };

    /**
     * @brief Represents virtual memory management functionalities.
     */
    class IVirtualMemoryClient {
    public:
        virtual ~IVirtualMemoryClient() = default;

        virtual OsStatus createVirtualMemory(const CreateVirtualMemoryRequest& request, CreateVirtualMemoryResult* result) = 0;

        virtual OsStatus releaseVirtualMemory(const ReleaseVirtualMemoryRequest& request, ReleaseVirtualMemoryResult* result) = 0;

        virtual OsStatus mapVirtualMemory(const MapVirtualMemoryRequest& request, MapVirtualMemoryResult* result) = 0;

        /**
         * @brief Get the Page table address for the current process.
         *
         * @param request The request parameters.
         * @param[out] result The result containing the page table address.
         * @return OsStatus The status of the operation.
         */
        virtual OsStatus getPageTableAddress(const GetPageTableAddressRequest& request, GetPageTableAddressResult *result [[outparam]]) = 0;
    };

    class IVirtualMemoryService {
    public:
        virtual ~IVirtualMemoryService() = default;

        virtual OsStatus createClient(sm::RcuSharedPtr<Process> process, IVirtualMemoryClient **client [[outparam]]) noexcept = 0;
    };
}
