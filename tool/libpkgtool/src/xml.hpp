#pragma once

#include <filesystem>
#include <generator>
#include <string>
#include <optional>

#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include <libxml/tree.h>

class XmlNode {
    xmlNodePtr mNode;

public:
    XmlNode(xmlNodePtr node)
        : mNode(node)
    { }

    xmlNodePtr get() const {
        return mNode;
    }

    std::generator<XmlNode> children() const {
        for (xmlNodePtr child = mNode->children; child != nullptr; child = child->next) {
            co_yield child;
        }
    }

    std::generator<XmlNode> elements() const {
        for (xmlNodePtr child = mNode->children; child != nullptr; child = child->next) {
            if (child->type == XML_ELEMENT_NODE) {
                co_yield child;
            }
        }
    }

    std::optional<std::string> property(const std::string& name) const {
        xmlChar *value = xmlGetProp(mNode, reinterpret_cast<const xmlChar *>(name.c_str()));
        if (value == nullptr) {
            return std::nullopt;
        }

        std::string result = reinterpret_cast<const char *>(value);
        xmlFree(value);
        return result;
    }

    std::generator<std::pair<std::string, std::string>> properties() const {
        for (xmlAttrPtr attr = mNode->properties; attr != nullptr; attr = attr->next) {
            const xmlChar *name = attr->name;
            xmlChar *value = xmlGetProp(mNode, name);

            co_yield {reinterpret_cast<const char *>(name), reinterpret_cast<const char *>(value)};

            xmlFree(value);
        }
    }

    std::string expect(const std::string& prop) const {
        auto result = property(prop);
        if (!result.has_value()) {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Node <{}> is missing required property {}", path(), line(), name(), prop));
        }

        return *result;
    }

    long line() const {
        return xmlGetLineNo(mNode);
    }

    std::string path() const {
        xmlChar *path = xmlGetNodePath(mNode);
        std::string result = reinterpret_cast<const char *>(path);
        xmlFree(path);
        return result;
    }

    std::string file() const {
        xmlDocPtr doc = mNode->doc;

        if (doc->URL == nullptr) {
            return ":memory:";
        }

        if (doc->URL == nullptr) {
            return ":memory:";
        }

        return reinterpret_cast<const char *>(doc->URL);
    }

    xmlElementType type() const {
        return mNode->type;
    }

    std::string_view name() const {
        return reinterpret_cast<const char *>(mNode->name);
    }

    operator xmlNodePtr() const { return get(); }
};

inline std::string locationToString(const XmlNode& node) {
    return std::format("[{}:{}] {}", node.file(), node.line(), node.path());
}

class XmlDocument {
    using XmlDoc = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
    XmlDoc mDocument;

public:
    XmlDocument(xmlDocPtr doc)
        : mDocument(doc, xmlFreeDoc)
    { }

    XmlNode root() const {
        return xmlDocGetRootElement(mDocument.get());
    }

    std::string path() const {
        if (mDocument->URL == nullptr) {
            return ":memory:";
        }

        return reinterpret_cast<const char *>(mDocument->URL);
    }

    static XmlDocument parse(const std::filesystem::path& path) {
        xmlDocPtr document = xmlReadFile(path.string().c_str(), nullptr, 0);
        if (document == nullptr) {
            throw std::runtime_error("Failed to parse " + path.string());
        }

        if (xmlXIncludeProcess(document) == -1) {
            xmlFreeDoc(document);
            throw std::runtime_error("Failed to process xinclude in " + path.string());
        }

        return XmlDocument { document };
    }

    static XmlDocument of(const std::string& content) {
        xmlDocPtr document = xmlReadMemory(content.c_str(), static_cast<int>(content.size()), "document.xml", nullptr, 0);
        if (document == nullptr) {
            throw std::runtime_error("Failed to parse XML content");
        }

        return XmlDocument { document };
    }
};
