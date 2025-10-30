#include "pkgtool/pkgtool.hpp"
#include "src/xml.hpp"

#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include <vector>

using pkg::IPackage;

namespace fs = std::filesystem;

using XmlDocument = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;

class PackageName {
    std::string mName;
    std::string mVersion;

public:
    PackageName(const std::string& name, const std::string& version)
        : mName(name)
        , mVersion(version)
    { }

    const std::string& name() const {
        return mName;
    }

    const std::string& version() const {
        return mVersion;
    }

    static PackageName ofXmlNode(const XmlNode& node) {
        auto name = node.expect("name");
        auto version = node.expect("version");

        return PackageName(name, version);
    }
};

class PackageImpl final : public IPackage {
    fs::path mFolder;
    std::vector<PackageName> mDependencies;
    std::vector<PackageName> mBuildDependencies;
    std::vector<PackageName> mTestDependencies;
public:
    PackageImpl(const fs::path& folder)
        : mFolder(folder)
    {
        auto pkginfo = mFolder / "pkg.xml";
        if (!fs::exists(pkginfo)) {
            throw std::runtime_error("Package folder " + folder.string() + " does not contain pkg.xml");
        }

        xmlDocPtr document = xmlReadFile(pkginfo.string().c_str(), nullptr, 0);
        if (document == nullptr) {
            throw std::runtime_error("Failed to parse " + pkginfo.string());
        }

        XmlDocument doc = {document, xmlFreeDoc};

        if (xmlXIncludeProcess(doc.get()) == -1) {
            throw std::runtime_error("Failed to process xinclude in " + pkginfo.string());
        }

        XmlNode root = xmlDocGetRootElement(doc.get());
        if (root.name() != "package") {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Invalid root element <{}> in {}, expected <package>", root.path(), root.line(), root.name(), pkginfo.string()));
        }

        auto maybeName = root.expect("name");
        auto maybeVersion = root.expect("version");

        for (auto child : root.children()) {
            auto name = child.name();
            if (name == "dependency") {
                mDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "build-dependency") {
                mBuildDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "test-dependency") {
                mTestDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else {
                throw std::runtime_error(std::format("ERROR [{}:{}]: Unknown element <{}> in {}", child.path(), child.line(), name, pkginfo.string()));
            }
        }
    }
};

std::shared_ptr<IPackage> IPackage::of(const std::filesystem::path& folder) {
    return std::make_shared<PackageImpl>(folder);
}
