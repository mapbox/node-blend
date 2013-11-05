#ifndef NODE_BLEND_MAVERICKS_H
#define NODE_BLEND_MAVERICKS_H

#if defined(__has_include) // clang
    #if __has_include("tr1/unordered_map") // non libc++
        #include <tr1/unordered_map>
        #include <tr1/memory>
        #define HASH_FUN_H <tr1/functional>
        #define HASH_NAMESPACE std::tr1
    #else // libc++
        #include <unordered_map>
        #include <memory>
        #define HASH_FUN_H <functional>
        #define HASH_NAMESPACE std
    #endif
#else // gcc, hope for the best
    #include <tr1/unordered_map>
    #include <tr1/memory>
    #define HASH_FUN_H <tr1/functional>
    #define HASH_NAMESPACE std::tr1
#endif

#endif
