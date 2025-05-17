#include "xsave.hpp"

#include "arch/cr4.hpp"
#include "arch/xcr0.hpp"
#include "logger/logger.hpp"
#include "panic.hpp"
#include "util/cpuid.hpp"

#include <algorithm>

constinit static km::Logger Save { "XSAVE" };

class NoSave final : public km::IFpuSave {
    km::SaveMode mode() const noexcept override { return km::SaveMode::eNoSave; }

public:
    size_t size() const noexcept override { return 0; }
    void save(void *) const noexcept override { }
    void restore(void *) const noexcept override { }

    static constinit NoSave gInstance;
};

class FxSave final : public km::IFpuSave {
    km::SaveMode mode() const noexcept override { return km::SaveMode::eFxSave; }

public:
    size_t size() const noexcept override { return sizeof(x64::FxSave); }

    void save(void *buffer) const noexcept override {
        __fxsave(reinterpret_cast<x64::FxSave *>(buffer));
    }

    void restore(void *buffer) const noexcept override {
        __fxrstor(reinterpret_cast<x64::FxSave *>(buffer));
    }

    void init() const noexcept override {
        //
        // Before continuing on we need to inform the processor
        // that we will be using fxsave.
        //
        x64::Cr4 cr4 = x64::Cr4::load();
        cr4.set(x64::Cr4::OSFXSR | x64::Cr4::OSXMMEXCPT);
        x64::Cr4::store(cr4);
    }

    static constinit FxSave gInstance;

    static void initInstance() {
        gInstance.init();
    }
};

class XSave final : public km::IFpuSave {
    /// @brief The size of the XSAVE buffer
    uint32_t mBufferSize = 0;

    /// @brief The bitmask of supported features in xcr0
    uint64_t mUserFeatures = 0;

    /// @brief The bitmask of supported features in IA32_XSS
    uint64_t mSystemFeatures = 0;

    /// @brief All currently enabled components
    uint64_t mEnabled = 0;

    km::SaveMode mode() const noexcept override { return km::SaveMode::eXSave; }

    uint64_t queryBufferSize() {
        sm::CpuId cpuid = sm::CpuId::count(0xD, 1);
        return cpuid.ebx;
    }

    uint64_t storeComponentState(uint64_t mask) const {
        //
        // The XSAVE bitmask is split across xcr0 and IA32_XSS depending on if the state is
        // user visible or not. So we need to pick the correct place to enable state.
        //
        uint64_t xcr0Mask = (mask & x64::kXcr0Mask & mUserFeatures);
        uint64_t xssMask = (mask & x64::kXssMask & mSystemFeatures);

        x64::Xcr0::store(x64::Xcr0::of(xcr0Mask));
        IA32_XSS = xssMask;

        return (xcr0Mask | xssMask);
    }

    void setComponentMask(uint64_t mask) {
        mEnabled = storeComponentState(mask);
    }

    void loadUserFeatureMask() {
        sm::CpuId leaf = sm::CpuId::count(0xD, 0);
        mUserFeatures = (uint64_t(leaf.edx) << 32) | (uint64_t(leaf.eax));
    }

    void loadSystemFeatureMask() {
        sm::CpuId leaf = sm::CpuId::count(0xD, 1);
        mSystemFeatures = (uint64_t(leaf.edx) << 32) | (uint64_t(leaf.ecx));
    }

    void loadBufferSize() {
        sm::CpuId leaf = sm::CpuId::count(0xD, 1);
        mBufferSize = leaf.ebx;
    }

    void initBootCore(uint64_t mask) {
        loadUserFeatureMask();
        loadSystemFeatureMask();
        setComponentMask(mask);
        loadBufferSize();

        Save.logf("Buffer size: ", mBufferSize, " bytes\n");
        Save.logf("Features: ", km::Hex(mUserFeatures), "\n");
        Save.logf("Enabled: ", km::Hex(mEnabled), "\n");
    }

    void initControlState() const {
        //
        // Before continuing on we need to enable these 3 bits to inform the processor
        // that we can support fxsave.
        //
        x64::Cr4 cr4 = x64::Cr4::load();
        cr4.set(x64::Cr4::OSFXSR | x64::Cr4::OSXMMEXCPT | x64::Cr4::OSXSAVE);
        x64::Cr4::store(cr4);
    }

public:
    size_t size() const noexcept override { return sizeof(x64::XSave) + mBufferSize; }

    [[gnu::target("xsave")]]
    void save(void *buffer) const noexcept override {
        __xsave(reinterpret_cast<x64::XSave *>(buffer), mEnabled);
    }

    [[gnu::target("xsave")]]
    void restore(void *buffer) const noexcept override {
        __xrstor(reinterpret_cast<x64::XSave *>(buffer), mEnabled);
    }

    void init() const noexcept override {
        initControlState();
        storeComponentState(mEnabled);
    }

    static constinit XSave gInstance;

    static void initInstance(uint64_t mask) {
        gInstance.initControlState();
        gInstance.initBootCore(mask);
    }
};

constinit NoSave NoSave::gInstance{};
constinit FxSave FxSave::gInstance{};
constinit XSave XSave::gInstance{};

static constinit km::IFpuSave *gFpuSave = nullptr;

km::IFpuSave *km::InitFpuSave(const XSaveConfig& config) {
    SaveMode choice = config.target;
    const ProcessorInfo *cpuInfo = config.cpuInfo;

    if (cpuInfo->xsave()) {
        choice = std::min(config.target, SaveMode::eXSave);
    } else if (cpuInfo->fxsave()) {
        choice = std::min(config.target, SaveMode::eFxSave);
    } else {
        choice = SaveMode::eNoSave;
    }

    if (choice != config.target) {
        Save.warnf("[XSAVE] Mode ", config.target, " not supported, falling back to ", choice, " instead.\n");
    }

    detail::SetupXSave(choice, config.features);

    return gFpuSave;
}

void km::detail::SetupXSave(SaveMode mode, uint64_t features) {
    switch (mode) {
    case SaveMode::eNoSave:
        gFpuSave = &NoSave::gInstance;
        break;
    case SaveMode::eFxSave:
        FxSave::initInstance();
        gFpuSave = &FxSave::gInstance;
        break;
    case SaveMode::eXSave:
        XSave::initInstance(features);
        gFpuSave = &XSave::gInstance;
        break;
    default:
        KM_PANIC("Invalid FPU save mode.");
    }
}

void km::XSaveInitApCore() {
    gFpuSave->init();
}

size_t km::XSaveSize() {
    return gFpuSave->size();
}

void km::XSaveStoreState(x64::XSave *area) {
    gFpuSave->save(area);
}

void km::XSaveLoadState(x64::XSave *area) {
    gFpuSave->restore(area);
}

/// @brief Sentinel value to indicate that xsave isnt supported.
/// Callers of km::CreateXSave check for nullptr to determine if an allocation failed.
/// When xsave isnt supported we return the address of this sentinel value so the nullptr
/// checks still pass, despite no structure being allocated.
static constexpr std::monostate __attribute__((noderef)) *kInvalidXSave = nullptr;

x64::XSave *km::detail::EmptyXSave() {
    return std::bit_cast<x64::XSave*>(&kInvalidXSave);
}

x64::XSave *km::CreateXSave() {
    size_t size = XSaveSize();
    if (size > 0) {
        void *result = aligned_alloc(alignof(x64::XSave), size);

        if (result) {
            std::memset(result, 0, km::XSaveSize());
        }

        return (x64::XSave*)result;
    }

    return detail::EmptyXSave();
}

void km::DestroyXSave(x64::XSave *area) {
    if (area != detail::EmptyXSave()) {
        free(area);
    }
}

void km::Format<km::SaveMode>::format(km::IOutStream &out, km::SaveMode mode) {
    switch (mode) {
    case km::SaveMode::eNoSave:
        out.write("None");
        break;
    case km::SaveMode::eFxSave:
        out.write("FXSAVE");
        break;
    case km::SaveMode::eXSave:
        out.write("XSAVE");
        break;
    }
}
