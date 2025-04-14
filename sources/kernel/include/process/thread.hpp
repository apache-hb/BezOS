#pragma once

#include "process/object.hpp"

#include "memory/layout.hpp"
#include "arch/xsave.hpp"
#include "isr/isr.hpp"
#include "std/string.hpp"

namespace km {
    struct Process;

    struct Thread : public BaseObject {
        Thread() = default;

        Thread(ThreadId id, stdx::String name, Process *process)
            : BaseObject(std::to_underlying(id) | (uint64_t(eOsHandleThread) << 56), std::move(name))
            , process(process)
        { }

        void init(ThreadId id, stdx::String name, Process *process, AddressMapping kernelStack);

        Process *process;
        HandleWait wait;
        km::IsrContext state;

        /// @brief Pointer to the xsave state for this thread
        //
        // This points to a block of memory that will be larger than sizeof(x64::XSave) as the xsave
        // area changes size based on enabled features and processor feature support.
        std::unique_ptr<x64::XSave, decltype(&free)> xsave{nullptr, &free};

        uint64_t tlsAddress = 0;
        AddressMapping userStack;
        AddressMapping kernelStack;
        AddressMapping tlsMapping;
        OsThreadState threadState = eOsThreadRunning;

        OsStatus waitOnHandle(BaseObject *object, OsInstant timeout);

        bool isComplete() const {
            return threadState != eOsThreadFinished;
        }

        void *getSyscallStack() const {
            return (void*)((uintptr_t)kernelStack.vaddr + kernelStack.size - x64::kPageSize);
        }
    };
}
