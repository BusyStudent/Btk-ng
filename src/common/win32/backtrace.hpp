#pragma once

// Import backtrace on MSVC (for debugging)
#if !defined(NDEBUG) && defined(_MSC_VER)
    #pragma comment(lib, "advapi32.lib")
    #include "libs/StackWalker.cpp"
#endif

#if !defined(NDEBUG) && defined(_MSC_VER)

#include <csignal>
#include <string>

static void Win32BacktraceInit() {
    // Initialize the debug 
    #pragma comment(lib, "dbghelp.lib")
    SetUnhandledExceptionFilter([](_EXCEPTION_POINTERS *info) -> LONG {
        // Dump callstack
        static bool recursive = false;
        if (recursive) {
            ExitProcess(EXIT_FAILURE);
            return EXCEPTION_CONTINUE_SEARCH;
        }
        recursive = true;

        class Walk : public StackWalker {
            public:
                Walk(std::string &out) : output(out) {}
                void OnCallstackEntry(CallstackEntryType,CallstackEntry &addr) override {
                    char buffer[1024] = {};
                    // Format console output
                    if (addr.lineNumber == 0) {
                        // No line number
                        sprintf(buffer, "#%d %p at %s [%s]\n", n, addr.offset, addr.name, addr.moduleName);
                    }
                    else {
                        sprintf(buffer, "#%d %p at %s [%s:%d]\n", n, addr.offset, addr.name, addr.lineFileName, addr.lineNumber);
                    }
                    fputs(buffer, stderr);
                    // Format message box output
                    sprintf(buffer, "#%d %p at %s\n", n, addr.offset, addr.name);
                    output.append(buffer);
                    n += 1;
                }
            private:
                std::string &output;
                int       n = 0;
        };
        std::string output = "-- Callstack summary --\n";
        // Set Console color to red
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED);
        fputs("--- Begin Callstack ---\n", stderr);
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        Walk(output).ShowCallstack();
        output.append("-- Detail callstack is on the console. ---\n");
        
        // Set Console color to white
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED);
        fputs("--- End Callstack ---\n", stderr);
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        // Show MessageBox
        MessageBoxA(nullptr, output.c_str(), "Unhandled Exception", MB_ICONERROR);
        return EXCEPTION_CONTINUE_SEARCH;
    });
    // Hook SIGABRT to dump callstack.
    signal(SIGABRT, [](int) {
        // Dump callstack
        // Raise SEH exception to dump callstack.
        printf("SIGABRT\n");
        RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
    });
}

#elif 0

// MinGW
#include <dbgeng.h>
#include <cxxabi.h>
#include <wrl.h>

__CRT_UUID_DECL(IDebugClient,0x27fe5639,0x8407,0x4f47,0x83,0x64,0xee,0x11,0x8f,0xb0,0x8a,0xc8)
__CRT_UUID_DECL(IDebugControl,0x5182e668,0x105e,0x416e,0xad,0x92,0x24,0xef,0x80,0x04,0x24,0xba)
__CRT_UUID_DECL(IDebugSymbols,0x8c31e98c,0x983a,0x48a5,0x90,0x16,0x6f,0xe5,0xd6,0x67,0xa9,0x50)

#define C_RED FOREGROUND_RED
#define C_GREEN FOREGROUND_GREEN
#define C_BLUE FOREGROUND_BLUE

#define C_YELLOW (C_GREEN | C_RED)
#define C_PURPLE (C_RED | C_BLUE)
#define C_TBLUE  (C_GREEN | C_BLUE)
#define C_WHITE (C_RED | C_GREEN | C_BLUE)

#define SET_COLOR(C) \
    SetConsoleTextAttribute( \
        GetStdHandle(STD_ERROR_HANDLE), \
        C \
    );


static HMODULE dbg_eng = nullptr;
static decltype(DebugCreate) *dbg_crt;

static void init_dbg_help(){
    if(dbg_crt == nullptr){
        dbg_eng = LoadLibraryA("DBGENG.dll");
        if(dbg_eng == nullptr){
            std::abort();
        }
        dbg_crt = (decltype(DebugCreate)*)GetProcAddress(dbg_eng,"DebugCreate");
    }
}

static void _Btk_Backtrace(){
    auto callstack = BtkUnwind::GetCallStack();

    // DWORD n = RtlCaptureStackBackTrace(
    //     0,
    //     callstack.size(),
    //     callstack.data(),
    //     0
    // );
    // BTK_ASSERT(n == callstack.size());

    init_dbg_help();
    //create debug interface
    Microsoft::WRL::ComPtr<IDebugClient> client;
    Microsoft::WRL::ComPtr<IDebugControl> control;
    
    dbg_crt(__uuidof(IDebugClient),reinterpret_cast<void**>(&client));
    client->QueryInterface(__uuidof(IDebugControl),reinterpret_cast<void**>(&control));
    //attach
    client->AttachProcess(
        0,
        GetCurrentProcessId(),
        DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND
    );
    control->WaitForEvent(DEBUG_WAIT_DEFAULT,INFINITE);
    //Get symbols
    Microsoft::WRL::ComPtr<IDebugSymbols> symbols;
    client->QueryInterface(__uuidof(IDebugSymbols),reinterpret_cast<void**>(&symbols));
    //Done

    //Print Stack
    bool has_fn_line;
    char  buf[256];
    char  file[256];

    ULONG size;
    ULONG  line;

    int naddr = callstack.size();

    SET_COLOR(C_RED);
    fputs("--Backtrace Begin--\n",stderr);
    fflush(stderr);

    for(auto addr:callstack){
        //try get name
        if(symbols->GetNameByOffset(
            reinterpret_cast<ULONG64>(addr),
            buf,
            sizeof(buf),
            &size,
            0
        ) == S_OK){
            buf[size] = '\0';
        }
        else{
            std::strcpy(buf,"???");
        }
        //try find !
        char *n_start = std::strchr(buf,'!');
        if(n_start != nullptr){
            *n_start = '_';
            //To __Znxxx
            //Begin demangle
            char *ret_name = abi::__cxa_demangle(n_start,nullptr,nullptr,nullptr);
            if(ret_name != nullptr){
                //has it name
                strcpy(n_start + 1,ret_name);
                std::free(ret_name);
            }
            *n_start = '!';
        }
        else{
            //didnot has function name
            strcat(buf,"!Unknown");
        }
        //Try get source file
        if(symbols->GetLineByOffset(
            reinterpret_cast<ULONG64>(addr),
            &line,
            file,
            sizeof(file),
            &size,
            0
        ) == S_OK){
            file[size] = '\0';
            has_fn_line = true;
            // fprintf(stderr,"[%d] %s at %s:%d\n",naddr,buf,file,line);
        }
        else{
            has_fn_line = false;
            // fprintf(stderr,"[%d] %s\n",naddr,buf);
        }

        SET_COLOR(C_WHITE);
        fputc('[',stderr);
        fflush(stderr);
        
        SET_COLOR(C_YELLOW);
        //Print number
        fprintf(stderr,"%d",naddr);
        fflush(stderr);

        SET_COLOR(C_WHITE);
        fputc(']',stderr);
        fflush(stderr);

        //Print fn name
        fprintf(stderr," %s",buf);
        fflush(stderr);

        if(has_fn_line){
            fprintf(stderr," at %s:",file);
            fflush(stderr);
            SET_COLOR(C_TBLUE);
            fprintf(stderr,"%d",line);
            fflush(stderr);
        }


        fputc('\n',stderr);
        naddr--;
    }

    SET_COLOR(C_RED);
    fputs("--Backtrace End--\n",stderr);
    fflush(stderr);

    SET_COLOR(C_WHITE);
}

static void Win32BacktraceInit() {
    SetUnhandledExceptionFilter([](_EXCEPTION_POINTERS *info) -> LONG {
        _Btk_Backtrace();
        return EXCEPTION_CONTINUE_SEARCH;
    });
}

#else

// Undebug or Unsupport
#define Win32BacktraceInit() do {} while(0)

#endif


// Generic macro name
#define BtkBacktraceInit Win32BacktraceInit