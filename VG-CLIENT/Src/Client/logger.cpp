#include "../common.hpp"
#include "logger.hpp"
#include <cstdarg>
#include <filesystem>
#include <fstream>

extern "C" void S4Log(const char* format, ...) {
    if (!format) return;
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    const int length = std::vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    if (length < 0) { va_end(args); return; }
    std::vector<char> message(static_cast<std::size_t>(length) + 1);
    std::vsnprintf(message.data(), message.size(), format, args);
    va_end(args);
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    std::string text(message.data(), static_cast<std::size_t>(length));
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    std::size_t start = 0;
    do {
        const auto end = text.find('\n', start);
        const auto line = text.substr(start, end == std::string::npos ? end : end - start);
        static std::ofstream file = [] {
            std::error_code error;
            const auto directory = std::filesystem::current_path(error)/"vg-assets"/"logs";
            if (!error) std::filesystem::create_directories(directory, error);
            return error ? std::ofstream{} : std::ofstream(directory/"S4-Client.log", std::ios::binary|std::ios::app);
        }();
        if (file) { file << "[S4-CLIENT] // " << line << '\n'; file.flush(); }
#ifdef _WIN32
        const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        CONSOLE_SCREEN_BUFFER_INFO info{};
        const bool console = GetConsoleMode(output, &mode) != 0;
        const bool vt = console && (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        if (vt) std::fputs("\x1b[38;2;255;215;0m[S4-CLIENT] //\x1b[0m ", stdout);
        else if (console && GetConsoleScreenBufferInfo(output, &info)) {
            SetConsoleTextAttribute(output, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            std::fputs("[S4-CLIENT] //", stdout);
            SetConsoleTextAttribute(output, info.wAttributes);
            std::fputc(' ', stdout);
        } else std::fputs("[S4-CLIENT] // ", stdout);
#else
        std::fputs("[S4-CLIENT] // ", stdout);
#endif
        std::fwrite(line.data(), 1, line.size(), stdout);
        std::fputc('\n', stdout);
        if (end == std::string::npos) break;
        start = end + 1;
    } while (start < text.size());
    std::fflush(stdout);
}
