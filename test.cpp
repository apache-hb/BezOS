#include <new>
#include <utility>

struct IInterface {
    virtual ~IInterface() = default;

    void operator delete(IInterface*, std::destroying_delete_t) {
        std::unreachable();
    }

    virtual void jeff(int id) = 0;
};

struct NotTrivial {
    int size;
    constexpr NotTrivial() { }
};

struct Sneaky final : public IInterface {
    NotTrivial inner;

    void jeff(int) override {

    }

    constexpr Sneaky() { }

    Sneaky(NotTrivial) { }
};

static Sneaky global;
