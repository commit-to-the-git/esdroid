#include "../include/path.h"
#include "../include/memory_tracker.h"
#include <unistd.h>
#include <string>

#if defined(__ANDROID__)
extern "C" void esdroid_wtflog(const char*,...);
#endif

#if defined(__ANDROID__) || defined(__linux__)

// std::filesystem is broken on android < api 30
// we dont use it at all m_path is void* always nullptr
// everything uses m_pathstring

piranha::Path::Path() : m_path(nullptr), m_pathString("") {}
piranha::Path::Path(const std::string &path) : m_path(nullptr), m_pathString(path) {}
piranha::Path::Path(const char *path) : m_path(nullptr), m_pathString(path ? path : "") {}
piranha::Path::Path(const std::filesystem::path &path) : m_path(nullptr), m_pathString("") {}
piranha::Path::Path(const Path &path) : m_path(nullptr), m_pathString(path.m_pathString) {}
piranha::Path::~Path() {}

bool piranha::Path::exists() const {
    if (m_pathString.empty()) return false;
    return access(m_pathString.c_str(), F_OK) == 0;
}

piranha::Path piranha::Path::append(const Path &path) const {
    if (m_pathString.empty()) return Path(path.m_pathString);
    if (m_pathString.back() == '/') return Path(m_pathString + path.m_pathString);
    return Path(m_pathString + "/" + path.m_pathString);
}

void piranha::Path::getParentPath(Path *path) const {
    path->m_path = nullptr;
    size_t pos = m_pathString.find_last_of('/');
    if (pos == std::string::npos) path->m_pathString = "";
    else path->m_pathString = m_pathString.substr(0, pos);
}

const piranha::Path &piranha::Path::operator=(const Path &b) {
    m_path = nullptr;
    m_pathString = b.m_pathString;
    return *this;
}

std::string piranha::Path::getExtension() const {
    size_t pos = m_pathString.find_last_of('.');
    if (pos == std::string::npos) return "";
    return m_pathString.substr(pos);
}

std::string piranha::Path::getStem() const {
    size_t slashPos = m_pathString.find_last_of('/');
    size_t dotPos = m_pathString.find_last_of('.');
    size_t start = (slashPos == std::string::npos) ? 0 : slashPos + 1;
    size_t end = (dotPos == std::string::npos || dotPos < start) ? m_pathString.length() : dotPos;
    return m_pathString.substr(start, end - start);
}

void piranha::Path::setPath(const std::string &path) {
    m_path = nullptr;
    m_pathString = path;
}

piranha::Path piranha::Path::canonicalize() const {
    // resolve and in the path
    std::string result;
    std::vector<std::string> parts;
    std::string current;
    for (size_t i = 0; i <= m_pathString.length(); ++i) {
        if (i == m_pathString.length() || m_pathString[i] == '/') {
            if (current == "..") {
                if (!parts.empty()) parts.pop_back();
            } else if (current == "." || current.empty()) {
                // skip
            } else {
                parts.push_back(current);
            }
            current.clear();
        } else {
            current += m_pathString[i];
        }
    }
    // rebuild path
    if (m_pathString[0] == '/') result = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += "/";
        result += parts[i];
    }
    return Path(result);
}

bool piranha::Path::isAbsolute() const {
    return !m_pathString.empty() && m_pathString[0] == '/';
}

#else

#include <filesystem>

piranha::Path::Path() { m_path = new std::filesystem::path; m_pathString = ""; }
piranha::Path::Path(const std::string &path) { m_path = new std::filesystem::path(path); m_pathString = path; }
piranha::Path::Path(const char *path) { m_path = new std::filesystem::path(path); m_pathString = path ? path : ""; }
piranha::Path::Path(const std::filesystem::path &path) { m_path = new std::filesystem::path(path); m_pathString = path.string(); }
piranha::Path::Path(const Path &path) { m_path = new std::filesystem::path(*static_cast<std::filesystem::path*>(path.m_path)); m_pathString = path.m_pathString; }
piranha::Path::~Path() { if (m_path != nullptr) delete static_cast<std::filesystem::path*>(m_path); }

bool piranha::Path::exists() const { return std::filesystem::exists(*static_cast<std::filesystem::path*>(m_path)); }
piranha::Path piranha::Path::append(const Path &path) const { return Path(*static_cast<std::filesystem::path*>(m_path) / *static_cast<std::filesystem::path*>(path.m_path)); }
void piranha::Path::getParentPath(Path *path) const { auto *p = new std::filesystem::path; *p = static_cast<std::filesystem::path*>(m_path)->parent_path(); path->m_path = p; path->m_pathString = p->string(); }
const piranha::Path &piranha::Path::operator=(const Path &b) { if (m_path) delete static_cast<std::filesystem::path*>(m_path); m_path = new std::filesystem::path(*static_cast<std::filesystem::path*>(b.m_path)); m_pathString = b.m_pathString; return *this; }
std::string piranha::Path::getExtension() const { return static_cast<std::filesystem::path*>(m_path)->extension().string(); }
std::string piranha::Path::getStem() const { return static_cast<std::filesystem::path*>(m_path)->stem().string(); }
void piranha::Path::setPath(const std::string &path) { if (m_path) delete static_cast<std::filesystem::path*>(m_path); m_path = new std::filesystem::path(path); m_pathString = path; }
bool piranha::Path::isAbsolute() const { return static_cast<std::filesystem::path*>(m_path)->is_absolute(); }

#endif
