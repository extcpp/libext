// Copyright - 2015 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics

// Collection of functions that allows loading of libraries
// cross platform for linux and windows. Other platforms are
// not supported yet. All strings are expected to be be utf-8
// encoded.
// For more information please refer to:
// - man 3 dlopen or http://linux.die.net/man/3/dlopen
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms684175.aspx
// (LoadLibrary)
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms683212.aspx
// (GetProcAdress)
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms683152.aspx
// (FreeLibrary)

#ifndef EXT_UTIL_LOAD_LIBRARY_HEADER
#define EXT_UTIL_LOAD_LIBRARY_HEADER
#include <ext/macros/platform.hpp>
#ifdef EXT_UNIX
    #include <dlfcn.h>
#elif EXT_WINDOWS
    #include "windows_strings.hpp"
    #include <strsafe.h>
    #include <windows.h>
#endif // EXT_UNIX
#include <stdexcept>
#include <string>

namespace ext { namespace util {
// types
#ifdef EXT_UNIX
typedef void* dl_handle;
typedef void* dl_address;
typedef int dl_rv;
#elif EXT_WINDOWS
typedef HMODULE dl_handle;
typedef FARPROC dl_address;
typedef BOOL dl_rv;
#endif // EXT_UNIX
typedef char* utf8_e_str;
typedef const char* const_utf8_e_str;

//! open library
/*!
 * Function that opens dynamic library and loads it into the address space of
 * current process
 * @param[in]   filename    path to library (null terminated string)
 * @param[in]   flag        binding strategy - defaults to RTLD_LAZY (ignored on
 * windows)
 * @return                  dl_handle or NULL on fail
 */
dl_handle dl_open(const_utf8_e_str filename, int flag = RTLD_LAZY) {
#ifdef EXT_UNIX
    return ::dlopen(filename, flag);
#elif EXT_WINDOWS
    #ifdef UNICODE
    std::wstring const tmp = string_to_win(filename);
    return ::LoadLibrary(tmp.c_str());
    #else
    return ::LoadLibrary(filename);
    #endif // UNICODE
#endif     // EXT_UNIX
}

//! open library - overload for std::string
dl_handle dl_open(const std::string& filename, int flag = RTLD_LAZY) {
    return dl_open(filename.c_str(), flag);
}

//! get error
/*!
 * Functions that returns textual error
 * @return  textual description of the error in utf-8 encoded std::string
 */
std::string dl_error(void) {
#ifdef EXT_UNIX
    // returns a static buffer - do not free!!!!
    char const* buffer = ::dlerror();
    if (buffer) {
        return std::string(buffer);
    } else {
        return std::string("");
    }
#elif EXT_WINDOWS
    LPVOID lpMsgBuf;
    DWORD dw_error_num = ::GetLastError();

    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dw_error_num,
                    ::MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
    #ifdef UNICODE
    std::string rv = string_from_win(lpMsgBuf)
    #else
    std::string rv(lpMsgBuf);
    #endif // UNICODE
    ::LocalFree(lpMsgBuf);
    return rv;
#endif     // EXT_UNIX
}

//! get address of symbol
/*!
 *  Function that receives address of symbol in dynamic library
 *  @param[in]  handle      Handle to dynamic library
 *  @param[in]  symbol      function or variable name to look up
 *  @return     dl_address  pointer to requested symbol
 *                          or NULL if symbol is not found
 */
dl_address dl_sym(dl_handle handle, const_utf8_e_str symbol) {
#ifdef EXT_UNIX
    return ::dlsym(handle, symbol);
#elif EXT_WINDOWS
    #ifdef UNICODE
    std::wstring tmp = string_to_win(symbol);
    return ::GetProcAddress(handle, tmp.c_str());
    #else
    return ::GetProcAddress(handle, symbol);
    #endif // UNICODE
#endif     // EXT_UNIX
}

//! get address of symbol - overload for std::string
dl_address dl_sym(dl_handle handle, const std::string& symbol) {
    return dl_sym(handle, symbol.c_str());
}

//! get address of symbol or throw logic_error
dl_address dl_sym_e(dl_handle handle, const std::string& symbol) {
    dl_address address = dl_sym(handle, symbol.c_str());
    if (address == NULL) {
        throw std::logic_error(dl_error());
    } else {
        return address;
    }
}

//! close library
/*!
 *  Function that decreases reference count on dynamic library and
 *  and closes the file if reference count drops to 0.
 *  @param[in]  handle  handle of lib to close
 *  @return             returns NULL on fail
 */
dl_rv dl_close(dl_handle handle) {
#ifdef EXT_UNIX
    return ::dlclose(handle);
#elif EXT_WINDOWS
    return ::FreeLibrary(handle);
#endif // EXT_UNIX
}
}}     // namespace ext::util
#endif // EXT_UTIL_LOAD_LIBRARY_HEADER
