#include <argparse/argparse.hpp>

#include <subprocess.hpp>

#include "debug/packet.hpp"

#include <filesystem>
#include <vector>
#include <fstream>
#include <thread>

namespace sp = subprocess;
namespace fs = std::filesystem;

static int RunInstances(const argparse::ArgumentParser& parser, std::span<std::string> extra) {
    fs::create_directories(parser.get<std::string>("--output"));

    auto image = fs::absolute(parser.get<std::string>("--image"));

    auto instances = parser.get<unsigned int>("--instances");
    std::vector<std::jthread> threads;
    std::unique_ptr<int[]> results(new int[instances]);

    for (auto i = 0; i < instances; i++) {
        threads.emplace_back([image, i, &parser, &results, &extra] {
            auto output = fs::absolute(parser.get<std::string>("--output")) / std::format("instance{:04d}", i);
            fs::create_directories(output);

            std::vector<std::string> args = {
                "qemu-system-x86_64",
                "-M", "q35",
                "-no-reboot",
                "-m", "128M",
                "-smp", "4",
                "-chardev", std::format("file,id=log,path={},signal=off", (output / "uart.log").string()),
                "-chardev", std::format("file,id=events,path={},signal=off", (output / "events.bin").string()),
                "-serial", "chardev:log",
                "-serial", "chardev:events",
                "-cdrom", image.string(),
            };

            args.insert(args.end(), extra.begin(), extra.end());

            auto cwd = output.string();
            auto result = sp::call(args, sp::cwd{cwd});
            results[i] = result;
        });
    }

    threads.clear();

    bool err = false;
    for (auto i = 0; i < instances; i++) {
        if (results[i] != 0) {
            std::cerr << "Instance " << i << " failed with " << results[i] << std::endl;
            err = true;
        }
    }

    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int ParseEvents(const argparse::ArgumentParser& parser) {
    auto events = parser.get<std::vector<std::string>>("--parse");
    for (const auto& event : events) {
        std::ifstream file(event, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open " << event << std::endl;
            return 1;
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        size_t size = buffer.size() / sizeof(km::debug::EventPacket);
        auto packets = reinterpret_cast<const km::debug::EventPacket*>(buffer.data());

        ssize_t vmem = 0;
        ssize_t pmem = 0;

        for (size_t i = 0; i < size; i++) {
            const auto& packet = packets[i];
            switch (packet.event) {
            case km::debug::Event::eAck:
                std::cout << "Ack" << std::endl;
                break;
            case km::debug::Event::eAllocatePhysicalMemory:
                pmem += packet.data.allocatePhysicalMemory.size;
                break;
            case km::debug::Event::eAllocateVirtualMemory:
                vmem += packet.data.allocateVirtualMemory.size;
                break;
            case km::debug::Event::eReleasePhysicalMemory:
                pmem = std::clamp<ssize_t>(pmem - (packet.data.releasePhysicalMemory.end - packet.data.releasePhysicalMemory.begin), ssize_t(0), pmem);
                break;
            case km::debug::Event::eReleaseVirtualMemory:
                vmem = std::clamp<ssize_t>(vmem - (packet.data.releaseVirtualMemory.end - packet.data.releaseVirtualMemory.begin), ssize_t(0), vmem);
                break;
            case km::debug::Event::eScheduleTask:
                std::cout << "ScheduleTask("
                    << std::hex << (packet.data.scheduleTask.previous & 0x00FF'FFFF'FFFF'FFFF)
                    << ", " << (packet.data.scheduleTask.next & 0x00FF'FFFF'FFFF'FFFF) << ")"
                    << std::dec << std::endl;
                break;
            default:
                std::cout << "Unknown(" << static_cast<int>(packet.event) << ")" << std::endl;
                break;
            }
        }

        std::cout << "Virtual memory: " << vmem << std::endl;
        std::cout << "Physical memory: " << pmem << std::endl;
    }

    return 0;
}

int main(int argc, const char **argv) try {
    argparse::ArgumentParser parser{"BezOS soak test tool"};

    parser.add_argument("extra")
        .remaining();

    parser.add_argument("--image")
        .help("Path to the image to test");

    parser.add_argument("--output", "-o")
        .help("Path to the output folder");

    parser.add_argument("--parse")
        .help("Parse event log")
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--instances", "-j")
        .help("Number of instances to run")
        .default_value(std::thread::hardware_concurrency() / 4)
        .scan<'u', unsigned>();

    parser.add_argument("--help")
        .help("Print this help message")
        .action([&](const std::string &) { std::cout << parser; std::exit(0); });

    auto unknown = parser.parse_known_args(argc, argv);

    if (parser.present("--parse")) {
        return ParseEvents(parser);
    } else if (parser.present("--image")) {
        return RunInstances(parser, unknown);
    }
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
