#include <sanitizer/asan_interface.h>

static constexpr uintptr_t kAsanShadowOffset = 0xdfff'e000'0000'0000;
static constexpr uintptr_t kAsanShadowSize = 0x8000'0000'0000;
static constexpr uintptr_t kAsanShift = 3;

static char *AsanShadowAddress(void *ptr) {
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t shadow = (address >> kAsanShift) + kAsanShadowOffset;
    return reinterpret_cast<char*>(shadow);
}

static void *AsanPointerFromShadow(char *shadow) {
    uintptr_t address = reinterpret_cast<uintptr_t>(shadow);
    uintptr_t ptr = (address - kAsanShadowOffset) << kAsanShift;
    return reinterpret_cast<void*>(ptr);
}

static void AsanReportStore(uintptr_t address, size_t size) {
    /* Empty... */
}

static void AsanReportLoad(uintptr_t address, size_t size) {
    /* Empty... */
}

extern "C" void __asan_report_load1_noabort(uintptr_t address) {
    AsanReportLoad(address, 1);
}

extern "C" void __asan_report_load2_noabort(uintptr_t address) {
    AsanReportLoad(address, 2);
}

extern "C" void __asan_report_load4_noabort(uintptr_t address) {
    AsanReportLoad(address, 4);
}

extern "C" void __asan_report_load8_noabort(uintptr_t address) {
    AsanReportLoad(address, 8);
}

extern "C" void __asan_report_load16_noabort(uintptr_t address) {
    AsanReportLoad(address, 16);
}

extern "C" void __asan_report_load_n_noabort(uintptr_t address, size_t size) {
    AsanReportLoad(address, size);
}

extern "C" void __asan_report_store1_noabort(uintptr_t address) {
    AsanReportStore(address, 1);
}

extern "C" void __asan_report_store2_noabort(uintptr_t address) {
    AsanReportStore(address, 2);
}

extern "C" void __asan_report_store4_noabort(uintptr_t address) {
    AsanReportStore(address, 4);
}

extern "C" void __asan_report_store8_noabort(uintptr_t address) {
    AsanReportStore(address, 8);
}

extern "C" void __asan_report_store16_noabort(uintptr_t address) {
    AsanReportStore(address, 16);
}

extern "C" void __asan_report_store_n_noabort(uintptr_t address, size_t size) {
    AsanReportStore(address, size);
}

extern "C" void __asan_handle_no_return() {
    /* Empty... */
}

extern "C" void __sanitizer_annotate_contiguous_container(const void *beg, const void *end, const void *old_mid, const void *new_mid) {
    /* Empty... */
}

extern "C" void __asan_poison_memory_region(void const volatile *addr, size_t size) {
    /* Empty... */
}

extern "C" void __asan_unpoison_memory_region(void const volatile *addr, size_t size) {
    /* Empty... */
}