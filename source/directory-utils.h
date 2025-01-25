#pragma once

#define NOMINMAX
#include <Windows.h>
#include <string_view>
#include <map>
#include <filesystem>

class DirectoryNode {
public:
    using ChildrenMap = std::map<std::wstring, DirectoryNode>;

    bool readFromString(std::wstring_view contents) {
        if (contents.empty())
            return false;

        m_children.clear();

        std::size_t lineEnd{};
        while (lineEnd != std::wstring_view::npos) {
            lineEnd = contents.find_first_of(L'\n');
            this->insertPath(contents.substr(0, lineEnd));
            contents.remove_prefix(lineEnd + 1);
        }

        flattenPaths();
        return true;
    }

    DirectoryNode(const std::wstring_view name = TEXT("/"), DirectoryNode* parent = nullptr)
        : m_name{ name }
        , m_parent{ parent }
    {}

    DirectoryNode(DirectoryNode& other)
        : m_name{ other.m_name }
        , m_children{ other.m_children }
        , m_parent{ nullptr }
    {
        for (auto& [_, node] : m_children) {
            node.m_parent = this;
        }
    }

    DirectoryNode(DirectoryNode&& other) noexcept
        : m_name{ std::move(other.m_name) }
        , m_children{ std::move(other.m_children) }
        , m_parent{ nullptr }
    {
        for (auto& [_, node] : m_children) {
            node.m_parent = this;
        }
    }

    DirectoryNode& operator=(const DirectoryNode& other) {
        if (this == &other)
            return *this;

        m_name = other.m_name;
        m_children = other.m_children;
        m_parent = nullptr;

        for (auto& [_, node] : m_children) {
            node.m_parent = this;
        }
        return *this;
    }

    DirectoryNode& operator=(DirectoryNode&& other) noexcept {
        if (this == &other)
            return *this;

        m_name = std::move(other.m_name);
        m_children = std::move(other.m_children);
        m_parent = nullptr;

        for (auto& [_, node] : m_children) {
            node.m_parent = this;
        }
        return *this;
    }

    std::wstring getName() const {
        return m_name;
    }

    ChildrenMap::iterator begin() {
        return m_children.begin();
    }

    ChildrenMap::iterator end() {
        return m_children.end();
    }

    std::size_t getChildCount() const {
        return m_children.size();
    }

    const ChildrenMap& getChildren() const {
        return m_children;
    }

    DirectoryNode& getChild(const std::wstring& childName) {
        return m_children.at(childName);
    }

    const DirectoryNode& getChild(const std::wstring& childName) const {
        return m_children.at(childName);
    }

    DirectoryNode* getParent() {
        return m_parent;
    }

    const DirectoryNode* getParent() const {
        return m_parent;
    }

    std::wstring_view getLongestChildName() const {
        return m_longestChildName;
    }

    struct Rect {
        float width{};
        float height{};
    };

    void setLongestChildSize(float width, float height) {
        m_longestChildSize.width  = width;
        m_longestChildSize.height = height;
    }

    Rect getLongestChildSize() const {
        return m_longestChildSize;
    }

    std::wstring getFullPath() {
        std::wstring path{};
        DirectoryNode* node{ this };
        while (node->getParent()) {
            auto nodeName{ node->getName() };
            if (nodeName.ends_with(explicitPathEnding)) {
                nodeName.pop_back();
            }
            path = nodeName + L'\\' + path;
            node = node->getParent();
        }
        return path;
    }

private:
    DirectoryNode* appendChild(const std::wstring_view name) {

        // Spectre mitigation
        bool isNewLongest{ name.length() > m_longestChildName.length() };
        if (isNewLongest) {
            m_longestChildName = name;
        }
        return &m_children.try_emplace(
            std::wstring{ name },
            name,
            this
        ).first->second;
    }

    void insertPath(std::wstring_view path) noexcept {
        std::wstring segment{};

        DirectoryNode* node{ this };

        std::size_t backslashPos{};
        while (backslashPos != std::wstring_view::npos) {
            backslashPos = path.find_first_of(L'\\');

            if (path.front() != L'*') {
                node = node->appendChild(path.substr(0, backslashPos));
                path.remove_prefix(backslashPos + 1);
                continue;
            }

            for (const auto& dir : std::filesystem::directory_iterator{ node->getFullPath() }) {
                if (!dir.is_directory())
                    continue;

                node->appendChild(dir.path().filename().wstring());
            }
            break;
        }
        node->m_name += explicitPathEnding;
    }

    bool flattenPaths() noexcept {
        if (m_children.size() == 1) {
            auto& node{ m_children.begin()->second };
            node.flattenPaths();
            if (m_name.ends_with(explicitPathEnding)) {
                m_name.pop_back();
                return false;
            }
            m_name += TEXT("\\") + node.m_name;

            auto newChildren{ node.m_children };
            m_children = std::move(newChildren);
            for (auto& [_, childNode] : m_children) {
                childNode.m_parent = this;
            }
            return true;
        }

        for (auto it{ m_children.begin() }; it != m_children.end(); ++it) {
            auto& node{ it->second };
            if (!node.flattenPaths())
                continue;

            m_children.try_emplace(node.m_name, node);
            node.m_parent = this;
            it = m_children.erase(it);
            const auto& name{ it->second.m_name };

            // Spectre mitigation
            bool isNewLongest{ name.length() > m_longestChildName.length() };
            if (isNewLongest) {
                m_longestChildName = name;
            }
        }
        if (m_name.ends_with(explicitPathEnding)) {
            m_name.pop_back();
        }
        return false;
    }

    std::wstring m_name{};
    ChildrenMap m_children{};
    DirectoryNode* m_parent{};
    std::wstring m_longestChildName{};
    Rect m_longestChildSize{};

    // Fixes
    // D:\OBS
    // D:\OBS\ttv-yt
    // Being flattened into D:\OBS\ttv-yt
    static const char explicitPathEnding{ L'|' };
};


class DirectoryNavigator {
public:
    DirectoryNavigator(DirectoryNode* const root)
        : m_currentNode{ root }
        , m_childIterator{ m_currentNode->begin() }
    {
        std::advance(m_childIterator, m_currentNode->getChildCount() / 2);
    }

    void selectionDown() {
        ++m_childIterator;
        if (m_childIterator == m_currentNode->end()) {
            m_childIterator = m_currentNode->begin();
        }
    }

    void selectionUp() {
        if (m_childIterator == m_currentNode->begin()) {
            m_childIterator = m_currentNode->end();
        }
        --m_childIterator;
    }

    bool enterSelected() {
        if (!getSelectedChild()->getChildCount())
            return false;

        m_currentNode = getSelectedChild();
        m_childIterator = m_currentNode->begin();
        std::advance(m_childIterator, m_currentNode->getChildCount() / 2);
        return true;
    }

    bool enterParent() {
        if (!m_currentNode->getParent())
            return false;

        m_currentNode = m_currentNode->getParent();
        m_childIterator = m_currentNode->begin();
        std::advance(m_childIterator, m_currentNode->getChildCount() / 2);
        return true;
    }

    DirectoryNode* getCurrentNode() const {
        return m_currentNode;
    }

    DirectoryNode* getSelectedChild() const {
        return &m_childIterator->second;
    }

private:
    DirectoryNode* m_currentNode;
    DirectoryNode::ChildrenMap::iterator m_childIterator{};
};

