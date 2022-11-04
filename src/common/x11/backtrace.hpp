#pragma once

#include <execinfo.h>
#include <unistd.h>
#include <cxxabi.h>
#include <string>
#include <cstdio>

using abi::__cxa_demangle;

inline void _Btk_Backtrace() {
    void *address[100];
    int ret = backtrace(address,100);

    fprintf(stderr,"\033[1;31m--Backtrace Begin-- pid:%d\033[0m\n",int(getpid()));
    //get address
    char **str = backtrace_symbols(address,ret);
    if(str == nullptr){
        fputs("  Error :Fail to get symbols\n",stderr);
    }
    //Function name to demangle
    std::string funcname;
    //File name
    std::string filename;
    //File symbols end position
    size_t file_end;
    size_t func_end;

    size_t func_plus;//'+' positions

    int demangle_status;

    char *func;
    //Buffer for __cxa_demangle
    char *outbuffer = nullptr;
    size_t outbuflen = 0;
    for (int i = 0;i < ret;i++) {
        std::string_view view(str[i]);

        file_end = view.find_first_of('(');
        //Get filename
        filename = view.substr(0,file_end);
        //Get function
        func_end = view.find_first_of(')',file_end + 1);
        func_plus = view.find_first_of('+',file_end + 1);

        if(func_plus > func_end or func_plus == std::string_view::npos){
            //Not found it
            funcname = view.substr(file_end + 1,func_end - file_end);
        }
        else{
            // We cannot get the funcion name
            if(func_plus == file_end + 1){
                funcname = std::string_view("???");
            }
            else{
                funcname = view.substr(file_end + 1,func_plus - file_end - 1);
            }
        }
        //To demangle the symbols
        func = __cxa_demangle(funcname.c_str(),outbuffer,&outbuflen,&demangle_status);
        if(func == nullptr){
            //demangle failed
            //using the raw name
            func = funcname.data();
        }
        else{
            outbuffer = func;
        }
        fprintf(stderr,"  \x1B[34mat\x1B[0m %p: %s (in %s)\n",address[i],func,filename.c_str());
    }
    free(str);
    free(outbuffer);
    fputs("\033[1;31m--Backtrace End--\033[0m\n",stderr);
    fflush(stderr);
};


#if !defined(NDEBUG)

#include <csignal>

__attribute__((constructor))
static void X11BacktraceInit() {
    auto cb = [](int sig) -> void {
        switch (sig) {
            case SIGSEGV : fputs("\033[1;31m  SIGSEGV \033[0m\n", stderr); break;
            case SIGABRT : fputs("\033[1;31m  SIGABRT \033[0m\n", stderr); break;
            case SIGILL  : fputs("\033[1;31m  SIGILL \033[0m\n", stderr); break;
        }

        _Btk_Backtrace();
        ::signal(sig, SIG_DFL);
        ::raise(sig);
    };
    ::signal(SIGSEGV, cb);
    ::signal(SIGABRT, cb);
    ::signal(SIGILL, cb);
}

#else

// Disabled
#define X11BacktraceInit() do {} while(0)

#endif

// Generic macro name
#define BtkBacktraceInit() X11BacktraceInit()