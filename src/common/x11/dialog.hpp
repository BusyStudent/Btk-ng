#pragma once

#include "build.hpp"
#include <Btk/context.hpp>
#include <Btk/string.hpp>
#include <Btk/event.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

BTK_NS_BEGIN

using namespace BTK_NAMESPACE;

class X11FileDialog : public AbstractFileDialog {
    public:
        X11FileDialog();
        ~X11FileDialog();

        int        run()    override;
        bool       initialize(bool save) override;
        void       set_dir(u8string_view dir) override;
        void       set_title(u8string_view title) override;
        void       set_allow_multi(bool v) override;
        StringList result() override;

        static bool  Supported();
    private:
        static FILE* Popen(const StringRefList &args, pid_t *_pi);

        int        client_fd  = -1;
        pid_t      client_pid = 0;

        bool       allow_multi;
        bool       is_save;
        StringList res;
        u8string   title;
};

static std::once_flag  native_once = {};
static const char     *native_client = nullptr;

static void find_native_client() {
    auto paths = u8string_view(::getenv("PATH")).split(";");

    for (auto &str : paths) {
        str.push_back('/');
        if (access((str.str() + "zenity").c_str(), X_OK) == 0) {
            native_client = "zenity";
            return;
        }
        if (access((str.str() + "kdialog").c_str(), X_OK) == 0) {
            native_client = "kdialog";
            return;
        }
    }
}

X11FileDialog::X11FileDialog() {

}
X11FileDialog::~X11FileDialog() {

}
int X11FileDialog::run() {
    assert(native_client != nullptr);
    if (strcmp(native_client, "zenity") == 0) {
        StringRefList args = {"zenity","--file-selection"};
        pid_t pid;

        if (allow_multi) {
            args.push_back("--multiple");
        }
        if (is_save) {
            args.push_back("--save");
        }
        args.push_back("--title");
        args.push_back(title);

        FILE *fp = Popen(args, &pid);
        if (!fp) {
            return EXIT_FAILURE;
        }

        // Wait the exit
        // FIXME :  Block here
        int stat;
        waitpid(pid, &stat, WNOHANG);

        u8string in;
        while (!feof(fp)) {
            auto ch = fgetc(fp);
            if (ch == EOF || ch == '\n') {
                break;
            }
            in.push_back(char(ch));
        }
        fclose(fp);

        res = in.split(" ");
        return stat;
    }
    return EXIT_FAILURE;
}
bool X11FileDialog::initialize(bool save) {
    is_save = save;
    return true;
}
void X11FileDialog::set_dir(u8string_view dir) {

}
void X11FileDialog::set_title(u8string_view t) {
    title = t;
}
void X11FileDialog::set_allow_multi(bool v) {
    allow_multi = v;
}
StringList X11FileDialog::result() {
    return res;
}

bool X11FileDialog::Supported() {
    std::call_once(native_once, find_native_client);
    return native_client;
}
FILE* X11FileDialog::Popen(const StringRefList &arr, pid_t *_pi) {
    char **args = nullptr;
    args = static_cast<char**>(alloca((arr.size() + 1) * sizeof(char*)));

    // Copy args
    size_t i;
    for(i = 0;i < arr.size(); i++) {
        auto &data = arr[i];
        args[i] = static_cast<char*>(alloca(data.size() + 1));
        memcpy(args[i], data.data(), data.size());
        args[i][data.size()] = '\0';
    }
    args[arr.size()] = nullptr;


    int fds[2];
    int err_fds[2];
    pipe(fds);
    pipe2(err_fds, O_CLOEXEC);

    pid_t pid = fork();
    if (pid == -1) {
        close(fds[0]);
        close(fds[1]);
        return nullptr;
    }
    else if (pid == 0) {
        close(fds[0]);
        close(err_fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        // Exec it
        execvp(args[0], args);
        // Failed, wrtie errno
        auto err = errno;
        write(err_fds[1], &err, sizeof(err));
        _Exit(-1);
    }
    else {
        auto handler = std::signal(SIGPIPE, SIG_IGN);
        close(fds[1]);
        close(err_fds[1]);
        int error;
        if (read(err_fds[0], &error, sizeof(error)) == sizeof(error)) {
            // has error
            // cleanup
            std::signal(SIGPIPE, handler);
            close(fds[0]);
            close(err_fds[0]);

            errno = error;
            return nullptr;
        }
        close(err_fds[0]);
        std::signal(SIGPIPE, handler);
        // Convert it to STD FILE*
        FILE *fptr = fdopen(fds[0], "r");
        if( fptr == nullptr) {
            // Error,close the fd
            close(fds[0]);
        }
        *_pi = pid;
        return fptr;
    }
}
BTK_NS_END