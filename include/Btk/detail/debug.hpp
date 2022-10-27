#pragma once

#if !defined(NDEBUG)

#include <Btk/detail/macro.hpp>
#include <Btk/detail/types.hpp>
#include <sstream>
#include <cstdio>

BTK_NS_BEGIN2(BTK_NAMESPACE::Debug)

class StdFile {
    public:
        StdFile(FILE *p) : fp(p) {}

        void operator <<(const char *s) {
            fputs(s, fp);
        }
        void operator <<(float v) {
            fprintf(fp, "%f", v);
        }
        void operator <<(int v) {
            fprintf(fp, "%d", v);
        }
    private:
        FILE *fp;
};


template <
    typename Stream, 
    typename Container
>
Stream &Show(Stream &s, const Container &container) {
    bool first = true;
    s << Btk_typeinfo_name(typeid(Container));
    s << "(";
    for (auto &v : container) {
        if (!first) {
            s << ", ";
        }
        first = false;
        s << v;
    }
    s << ")";
    return s;
}

// template <typename Stream, typename First, typename ...Args>
// void Print(Stream &s, First &&first, Args &&...args) {
//     s << first;
//     s << ", ";
//     Print(s, std::forward<Args>(args)...);
// }

BTK_NS_END2(BTK_NAMESPACE::Debug)

#endif