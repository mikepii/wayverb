#pragma once

#include <dirent.h>

#include <memory>
#include <string>
#include <vector>

class dir final {
public:
    dir(const char* name)
            : ptr_{opendir(name)} {
        if (!ptr_) {
            throw std::runtime_error{"Unable to open directory. dir=" + std::string{name}};
        }
    }

    auto read() { return readdir(ptr_.get()); }

    void rewind() { rewinddir(ptr_.get()); }

private:
    struct destructor final {
        template <typename T>
        void operator()(T t) {
            closedir(t);
        }
    };

    std::unique_ptr<DIR, destructor> ptr_;
};

inline auto list_directory(const char* name) {
    dir directory{name};
    std::vector<std::string> ret{};

    while (const auto ptr = directory.read()) {
#ifdef _DIRENT_HAVE_D_NAMLEN 
        ret.emplace_back(ptr->d_name, ptr->d_name + ptr->d_namlen);
#else
        ret.emplace_back(std::string(ptr->d_name));
#endif
    }

    return ret;
}
