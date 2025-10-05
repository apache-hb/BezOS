#include <ktest/ktest.h>

#include "private.hpp"

#include "arch/cr0.hpp"
#include "arch/cr4.hpp"
#include "hypervisor.hpp"
#include "kernel.hpp"
#include "logger/categories.hpp"
#include "logger/serial_appender.hpp"
#include "setup.hpp"
#include "syscall.hpp"
#include "xsave.hpp"

#include <span>

extern "C" const km::testing::TestFactory __test_cases_start[];
extern "C" const km::testing::TestFactory __test_cases_end[];

static constexpr bool kEnableXSave = true;

constinit km::CpuLocal<x64::TaskStateSegment> km::tlsTaskState;

static constinit km::SystemGdt gBootGdt{};
constinit static km::SerialAppender gSerialAppender;

km::PageTables& km::GetProcessPageTables() {
    KM_PANIC("GetProcessPageTables is not implemented in the kernel test environment.");
}

sys::System *km::GetSysSystem() {
    KM_PANIC("GetSysSystem is not implemented in the kernel test environment.");
}

static km::SystemMemory *gMemory = nullptr;

km::SystemMemory *km::GetSystemMemory() {
    return gMemory;
}

void km::setupInitialGdt() {
    InitGdt(gBootGdt.entries, SystemGdt::eLongModeCode, SystemGdt::eLongModeData);
}

[[gnu::noinline, gnu::used]]
std::span<const km::testing::TestFactory> getTestCases() {
    return std::span(__test_cases_start, __test_cases_end);
}

void km::testing::Test::Run() {
    SetUp();
    TestBody();
    TearDown();
}

static void normalizeProcessorState() {
    x64::Cr0 cr0 = x64::Cr0::of(x64::Cr0::PG | x64::Cr0::WP | x64::Cr0::NE | x64::Cr0::ET | x64::Cr0::PE);
    x64::Cr0::store(cr0);

    x64::Cr4 cr4 = x64::Cr4::of(x64::Cr4::PAE);
    x64::Cr4::store(cr4);

    // Enable NXE bit
    IA32_EFER |= (1 << 11);

    IA32_GS_BASE = 0;
}

static km::SerialPortStatus initSerialPort(km::ComPortInfo info) {
    if (km::OpenSerialResult com = OpenSerial(info)) {
        return com.status;
    } else {
        km::SerialAppender::create(com.port, &gSerialAppender);
        km::LogQueue::addGlobalAppender(&gSerialAppender);
        return km::SerialPortStatus::eOk;
    }
}

static void enableUmip(bool enable) {
    if (enable) {
        x64::Cr4 cr4 = x64::Cr4::load();
        cr4.set(x64::Cr4::UMIP);
        x64::Cr4::store(cr4);
    }
}

static int gExitCode = 0;

void km::testing::detail::addError(AssertMessage message, std::source_location location) {
    gExitCode = 1;
    TestLog.errorf(message, " at ", stdx::StringView::ofString(location.file_name()), ":", location.line(), " in ", stdx::StringView::ofString(location.function_name()));
}

struct KernelLayout {
    /// @brief Virtual address space reserved for the kernel.
    /// Space for dynamic allocations, kernel heap, kernel stacks, etc.
    km::VirtualRangeEx system;

    /// @brief Non-pageable memory.
    km::AddressMapping committed;

    /// @brief All framebuffers.
    km::VirtualRangeEx framebuffers;

    /// @brief Statically allocated memory for the kernel.
    /// Includes text, data, bss, and other sections.
    km::AddressMapping kernel;

    void *getFrameBuffer(std::span<const boot::FrameBuffer> fbs, size_t index) const {
        size_t offset = 0;
        for (size_t i = 0; i < index; i++) {
            offset += fbs[i].size();
        }

        return (void*)((uintptr_t)framebuffers.front.address + offset);
    }

    uintptr_t committedSlide() const {
        return committed.slide();
    }
};

extern "C" {
    extern char __kernel_start[];
    extern char __kernel_end[];
}

static size_t GetKernelSize() {
    return (uintptr_t)__kernel_end - (uintptr_t)__kernel_start;
}

/// @brief Factory to reserve sections of virtual address space.
///
/// Grows down from the top of virtual address space.
class AddressSpaceBuilder {
    uintptr_t mOffset = 0;

public:
    AddressSpaceBuilder(uintptr_t top)
        : mOffset(top)
    { }

    /// @brief Reserve a section of virtual address space.
    ///
    /// The reserved memory is aligned to the smallest page size.
    ///
    /// @param memory The size of the memory to reserve.
    /// @return The reserved virtual address range.
    km::VirtualRangeEx reserve(size_t memory) {
        uintptr_t front = mOffset;
        mOffset += sm::roundup(memory, x64::kPageSize);

        return km::VirtualRangeEx { (void*)front, (void*)mOffset };
    }
};

static size_t TotalDisplaySize(std::span<const boot::FrameBuffer> displays) {
    return std::accumulate(displays.begin(), displays.end(), 0zu, [](size_t sum, const boot::FrameBuffer& framebuffer) {
        return sum + framebuffer.size();
    });
}

struct MemoryMap {
    stdx::Vector3<boot::MemoryRegion> memmap;

    MemoryMap(mem::IAllocator *allocator)
        : memmap(allocator)
    { }

    void sortMemoryMap() {
        std::sort(memmap.begin(), memmap.end(), [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
            return a.range().front < b.range().front;
        });
    }

    /// @brief Find and reserve a section of physical memory.
    ///
    /// @param size The size of the memory to reserve.
    /// @return The reserved physical memory range.
    km::MemoryRange reserve(size_t size) {
        for (boot::MemoryRegion& entry : memmap) {
            if (!entry.isUsable())
                continue;

            if (entry.size() < size)
                continue;

            if (IsLowMemory(entry.range()))
                continue;

            km::MemoryRange range = { entry.range().front, entry.range().front + size };
            entry.setRange(entry.range().cut(range));

            memmap.add({ boot::MemoryRegionType::eKernelRuntimeData, range });

            sortMemoryMap();

            return range;
        }

        return km::MemoryRange{};
    }

    void insert(boot::MemoryRegion memory) {
        for (size_t i = 0; i < memmap.count(); i++) {
            boot::MemoryRegion& entry = memmap[i];
            if (entry.range().intersects(memory.range())) {
                entry.setRange(entry.range().cut(memory.range()));
            }
        }

        memmap.add(memory);

        sortMemoryMap();
    }
};

static inline constexpr size_t kCommittedRegionSize = sm::megabytes(16).bytes();

static KernelLayout BuildKernelLayout(
    uintptr_t vaddrbits,
    km::PhysicalAddress kernelPhysicalBase,
    const void *kernelVirtualBase,
    MemoryMap& memory,
    std::span<const boot::FrameBuffer> displays
) {
    size_t framebuffersSize = TotalDisplaySize(displays);

    // reserve the top 1/4 of the address space for the kernel
    uintptr_t dataFront = -(1ull << (vaddrbits - 1));
    uintptr_t dataBack = -(1ull << (vaddrbits - 2));

    km::VirtualRangeEx data = { (void*)dataFront, (void*)dataBack };

    AddressSpaceBuilder builder { dataBack };

    km::MemoryRange committedPhysicalMemory = memory.reserve(kCommittedRegionSize);
    if (committedPhysicalMemory.isEmpty()) {
        InitLog.fatalf("Failed to reserve memory for committed region ", sm::bytes(kCommittedRegionSize), ".");
        KM_PANIC("Failed to reserve memory for committed region.");
    }

    km::VirtualRangeEx committedVirtualRange = builder.reserve(kCommittedRegionSize);
    km::AddressMapping committed = { committedVirtualRange.front, committedPhysicalMemory.front, kCommittedRegionSize };

    km::VirtualRangeEx framebuffers = builder.reserve(framebuffersSize);

    km::AddressMapping kernel = { kernelVirtualBase, kernelPhysicalBase, GetKernelSize() };

    InitLog.dbgf("Kernel layout:");
    InitLog.dbgf("Data         : ", data);
    InitLog.dbgf("Committed    : ", committed);
    InitLog.dbgf("Framebuffers : ", framebuffers);
    InitLog.dbgf("Kernel       : ", kernel);

    KM_CHECK(data.isValid(), "Invalid data range.");
    KM_CHECK(committedVirtualRange.isValid(), "Invalid committed range.");
    KM_CHECK(framebuffers.isValid(), "Invalid framebuffers range.");
    KM_CHECK(kernel.physicalRange().isValid(), "Invalid kernel range.");

    KM_CHECK(!data.intersects(committedVirtualRange), "Committed range overlaps with data range.");
    KM_CHECK(!data.intersects(framebuffers), "Framebuffers overlap with data range.");

    KernelLayout layout = {
        .system = data,
        .committed = committed,
        .framebuffers = framebuffers,
        .kernel = kernel,
    };

    return layout;
}

static mem::TlsfAllocator *MakeEarlyAllocator(km::AddressMapping memory) {
    mem::TlsfAllocator allocator { (void*)memory.vaddr, memory.size };

    void *mem = allocator.allocateAligned(sizeof(mem::TlsfAllocator), alignof(mem::TlsfAllocator));

    return new (mem) mem::TlsfAllocator(std::move(allocator));
}

static MemoryMap* CreateEarlyAllocator(const boot::LaunchInfo& launch, mem::TlsfAllocator *allocator) {
    const auto& memmap = launch.memmap;
    km::AddressMapping bootMemory = launch.earlyMemory;
    km::MemoryRange range = bootMemory.physicalRange();

    MemoryMap *memory = allocator->construct<MemoryMap>(allocator);
    std::copy(memmap.begin(), memmap.end(), std::back_inserter(memory->memmap));

    memory->insert({ boot::MemoryRegionType::eKernelRuntimeData, range });

    return memory;
}

static void MapDisplayRegions(km::PageTables& vmm, std::span<const boot::FrameBuffer> framebuffers, km::VirtualRangeEx range) {
    uintptr_t framebufferBase = (uintptr_t)range.front.address;
    for (const boot::FrameBuffer& framebuffer : framebuffers) {
        // remap the framebuffer into its final location
        km::AddressMapping fb = { (void*)framebufferBase, framebuffer.paddr.address, framebuffer.size() };
        if (OsStatus status = vmm.map(fb, km::PageFlags::eData, km::MemoryType::eWriteCombine)) {
            InitLog.fatalf("Failed to map framebuffer: ", fb, " ", OsStatusId(status));
            KM_PANIC("Failed to map framebuffer.");
        }

        framebufferBase += framebuffer.size();
    }
}

static void MapKernelRegions(km::PageTables &vmm, const KernelLayout& layout) {
    auto [kernelVirtualBase, kernelPhysicalBase, _] = layout.kernel;
    MapKernel(vmm, kernelPhysicalBase, kernelVirtualBase);
}

static void MapDataRegion(km::PageTables &vmm, km::AddressMapping mapping) {
    if (OsStatus status = vmm.map(mapping, km::PageFlags::eData)) {
        InitLog.fatalf("Failed to map data region: ", mapping, " ", OsStatusId(status));
        KM_PANIC("Failed to map data region.");
    }
}

/// Everything in this structure is owned by the stage1 memory allocator.
/// Once that allocator is destroyed this memory is no longer mapped.
struct Stage1MemoryInfo {
    mem::TlsfAllocator *allocator;
    MemoryMap *earlyMemory;
    KernelLayout layout;
    std::span<boot::FrameBuffer> framebuffers;
};

static boot::FrameBuffer *CloneFrameBuffers(mem::IAllocator *alloc, std::span<const boot::FrameBuffer> framebuffers) {
    boot::FrameBuffer *fbs = alloc->allocateArray<boot::FrameBuffer>(framebuffers.size());
    std::copy(framebuffers.begin(), framebuffers.end(), fbs);
    return fbs;
}

static Stage1MemoryInfo InitStage1Memory(const boot::LaunchInfo& launch, const km::ProcessorInfo& processor) {
    km::PageMemoryTypeLayout pat = km::SetupPat();

    km::AddressMapping stack = launch.stack;
    km::PageBuilder pm = km::PageBuilder { processor.maxpaddr, processor.maxvaddr, pat };

    WriteMtrrs(pm);

    mem::TlsfAllocator *allocator = MakeEarlyAllocator(launch.earlyMemory);
    MemoryMap *earlyMemory = CreateEarlyAllocator(launch, allocator);

    KernelLayout layout = BuildKernelLayout(
        processor.maxvaddr,
        launch.kernelPhysicalBase,
        launch.kernelVirtualBase,
        *earlyMemory,
        launch.framebuffers
    );

    boot::FrameBuffer *framebuffers = CloneFrameBuffers(allocator, launch.framebuffers);

    static constexpr size_t kPtAllocSize = sm::kilobytes(512).bytes();
    void *pteMemory = allocator->allocateAligned(kPtAllocSize, x64::kPageSize);
    KM_CHECK(pteMemory != nullptr, "Failed to allocate memory for page tables.");

    size_t offset = (uintptr_t)pteMemory - (uintptr_t)launch.earlyMemory.vaddr;
    KM_CHECK(offset < launch.earlyMemory.size, "Page table memory is out of bounds.");

    km::AddressMapping pteMapping {
        .vaddr = pteMemory,
        .paddr = launch.earlyMemory.paddr + offset,
        .size = kPtAllocSize
    };

    km::PageTables vmm;
    if (OsStatus status = km::PageTables::create(&pm, pteMapping, km::PageFlags::eAll, &vmm)) {
        InitLog.fatalf("Failed to create page tables: ", OsStatusId(status));
        KM_PANIC("Failed to create page tables.");
    }

    // initialize our own page tables and remap everything into it
    MapKernelRegions(vmm, layout);

    // move our stack out of reclaimable memory
    MapDataRegion(vmm, stack);

    // map the early allocator region
    MapDataRegion(vmm, launch.earlyMemory);

    // map the non-pageable memory
    MapDataRegion(vmm, layout.committed);

    // remap framebuffers
    MapDisplayRegions(vmm, launch.framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    UpdateRootPageTable(pm, vmm);

    return Stage1MemoryInfo { allocator, earlyMemory, layout, std::span(framebuffers, launch.framebuffers.size()) };
}

struct Stage2MemoryInfo {
    km::SystemMemory *memory;
    KernelLayout layout;
};

static Stage2MemoryInfo *InitStage2Memory(const boot::LaunchInfo& launch, const km::ProcessorInfo& processor) {
    Stage1MemoryInfo stage1 = InitStage1Memory(launch, processor);
    km::AddressMapping stack = launch.stack;
    KernelLayout layout = stage1.layout;
    std::span<boot::FrameBuffer> framebuffers = stage1.framebuffers;
    MemoryMap *earlyMemory = stage1.earlyMemory;

    km::PageBuilder pm = km::PageBuilder { processor.maxpaddr, processor.maxvaddr, km::GetDefaultPatLayout() };

    km::InitGlobalAllocator((void*)layout.committed.vaddr, layout.committed.size);

    Stage2MemoryInfo *stage2 = new Stage2MemoryInfo();

    // TODO: rather than increasing this size we should use 1g pages for the big things
    // pcie ecam regions are the culprit here
    static constexpr size_t kPtAllocSize = sm::megabytes(3).bytes();
    void *pteMemory = aligned_alloc(x64::kPageSize, kPtAllocSize);
    KM_CHECK(pteMemory != nullptr, "Failed to allocate memory for page tables.");

    size_t offset = (uintptr_t)pteMemory - (uintptr_t)layout.committed.vaddr;
    KM_CHECK(offset < layout.committed.size, "Page table memory is out of bounds.");

    km::AddressMapping pteMapping {
        .vaddr = pteMemory,
        .paddr = layout.committed.paddr + offset,
        .size = kPtAllocSize
    };

    InitLog.dbgf("Page table memory: ", pteMapping);

    km::SystemMemory *memory = new km::SystemMemory;
    KM_CHECK(memory != nullptr, "Failed to allocate memory for SystemMemory.");

    OsStatus status = km::SystemMemory::create(earlyMemory->memmap, stage1.layout.system.cast<sm::VirtualAddress>(), pm, pteMapping, memory);
    KM_CHECK(status == OsStatusSuccess, "Failed to create SystemMemory.");

    stage2->memory = memory;
    stage2->layout = layout;

    // carry forward the mappings made in stage 1
    // that are still required for kernel runtime
    memory->reserve(stack);
    // memory->reserve(layout.committed);
    // memory->reserve(layout.kernel);
    // memory->reservePhysical({ 0zu, kLowMemory });

    // initialize our own page tables and remap everything into it
    MapKernelRegions(memory->systemTables(), layout);

    // map the stack again
    MapDataRegion(memory->systemTables(), stack);

    // carry forward the non-pageable memory
    MapDataRegion(memory->systemTables(), layout.committed);

    // remap framebuffers
    MapDisplayRegions(memory->systemTables(), framebuffers, layout.framebuffers);

    // once it is safe to remap the boot memory, do so
    UpdateRootPageTable(memory->getPageManager(), memory->systemTables());

    return stage2;
}

void km::testing::detail::initKernelTest(boot::LaunchInfo launch) {
    normalizeProcessorState();

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
    };

    initSerialPort(com1Info);

    ProcessorInfo processor = GetProcessorInfo();
    enableUmip(processor.umip());

    gBootGdt = getBootGdt();
    setupInitialGdt();

    //
    // Once we have the initial gdt setup we create the global allocator.
    // The IDT depends on the allocator to create its global ISR table.
    //
    Stage2MemoryInfo *stage2 = InitStage2Memory(launch, processor);
    gMemory = stage2->memory;

    LogQueue::initGlobalLogQueue(128);

    XSaveConfig xsaveConfig {
        .target = kEnableXSave ? SaveMode::eXSave : SaveMode::eFxSave,
        .features = XSaveMask(x64::FPU, x64::SSE, x64::AVX) | x64::kSaveAvx512,
        .cpuInfo = &processor,
    };

    initFpuSave(xsaveConfig);

    TestLog.infof("Kernel test environment initialized.");
}

int km::testing::detail::runAllTests() {
    auto tests = getTestCases();
    TestLog.infof("Running ", tests.size(), " test cases.");
    for (const km::testing::TestFactory factory : tests) {
        std::unique_ptr<km::testing::Test> test(factory());
        test->Run();
    }

    return gExitCode;
}
