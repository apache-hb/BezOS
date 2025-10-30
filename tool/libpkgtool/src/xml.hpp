#pragma once

#include <generator>
#include <string>
#include <optional>

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

    std::optional<std::string> property(const std::string& name) const {
        xmlChar *value = xmlGetProp(mNode, reinterpret_cast<const xmlChar *>(name.c_str()));
        if (value == nullptr) {
            return std::nullopt;
        }

        std::string result = reinterpret_cast<const char *>(value);
        xmlFree(value);
        return result;
    }

    std::string expect(const std::string& name) const {
        auto prop = property(name);
        if (!prop.has_value()) {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Node <{}> is missing required property {}", mNode->doc->name, mNode->line, this->name(), name));
        }

        return *prop;
    }

    unsigned line() const {
        return xmlGetLineNo(mNode);
    }

    std::string_view path() const {
        return reinterpret_cast<const char *>(mNode->doc->name);
    }

    std::string_view name() const {
        return reinterpret_cast<const char *>(mNode->name);
    }

    operator xmlNodePtr() const { return get(); }
};