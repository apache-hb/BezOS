#pragma once

#include "delay.hpp" // IWYU pragma: export

#include <stdint.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace kmtest {
    class PeripheralRegistry;

    class IPeripheral {
        std::string mName;
    public:
        IPeripheral(std::string name)
            : mName(std::move(name))
        { }

        virtual ~IPeripheral() = default;

        virtual void write8(uint16_t port, uint8_t value);
        virtual void write16(uint16_t port, uint16_t value);
        virtual void write32(uint16_t port, uint32_t value);

        virtual uint8_t read8(uint16_t port);
        virtual uint16_t read16(uint16_t port);
        virtual uint32_t read32(uint16_t port);

        virtual void connect(PeripheralRegistry& device) = 0;

        virtual void delay() { }
    };

    class PeripheralRegistry {
        std::vector<IPeripheral*> mPeripherals;
        IPeripheral *mDefault = nullptr;
        std::unordered_map<uint16_t, IPeripheral*> mPortMap;

    public:
        PeripheralRegistry() = default;

        void add(IPeripheral *device) {
            mPeripherals.push_back(device);
            device->connect(*this);
        }

        void remove(IPeripheral *device) {
            std::erase(mPeripherals, device);
            for (auto it = mPortMap.begin(); it != mPortMap.end();) {
                if (it->second == device) {
                    it = mPortMap.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void setDefault(IPeripheral *device) {
            mDefault = device;
        }

        void wire(uint16_t port, IPeripheral *device) {
            mPortMap[port] = device;
        }

        void delay() {
            for (IPeripheral *device : mPeripherals) {
                device->delay();
            }
        }

        void write8(uint16_t port, uint8_t value);
        void write16(uint16_t port, uint16_t value);
        void write32(uint16_t port, uint32_t value);

        uint8_t read8(uint16_t port);
        uint16_t read16(uint16_t port);
        uint32_t read32(uint16_t port);
    };

    PeripheralRegistry& devices();
}
