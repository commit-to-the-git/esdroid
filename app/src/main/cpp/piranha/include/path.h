#ifndef PIRANHA_PATH_H
#define PIRANHA_PATH_H

#include <string>

// Forward-declare std::filesystem::path to avoid including <filesystem>
// which is broken on Android < API 30.
namespace std { namespace filesystem { class path; } }

namespace piranha {

    class Path {        
    protected: explicit Path(const std::filesystem::path &path);
    public:
        Path(const std::string &path);
        Path(const char *path);
        Path(const Path &path);
        Path();
        ~Path();

        std::string toString() const { return m_pathString; }

        void setPath(const std::string &path);
        bool operator==(const Path &path) const { return m_pathString == path.m_pathString; }
        Path append(const Path &path) const;

        void getParentPath(Path *path) const;

        const Path &operator =(const Path &b);

        std::string getExtension() const;
        std::string getStem() const;

        bool isAbsolute() const;
        Path canonicalize() const;
        bool exists() const;

    protected:
        void *m_path;  // Opaque pointer - never used on Android
        std::string m_pathString;

    protected:
        const void *getBoostPath() const { return m_path; }
    };

} /* namespace piranha */

#endif /* PIRANHA_PATH_H */
