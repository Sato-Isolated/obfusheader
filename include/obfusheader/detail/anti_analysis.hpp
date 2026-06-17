#ifndef OBFUSHEADER_DETAIL_ANTI_ANALYSIS_HPP
#define OBFUSHEADER_DETAIL_ANTI_ANALYSIS_HPP

#include <cstdlib>
#include <cstring>
#include <cstdio>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace oh::anti_analysis {

struct result {
    bool debugger_present = false;
    bool tracer_present = false;

    bool detected() const {
        return debugger_present || tracer_present;
    }
};

inline result probe() {
    result value{};
#if defined(OH_FORCE_ANALYSIS_DETECTED) && !defined(OH_DISABLE_ANTI_ANALYSIS)
    value.debugger_present = true;
#elif !defined(OH_DISABLE_ANTI_ANALYSIS) && defined(_WIN32)
    value.debugger_present = (::IsDebuggerPresent() != 0);
    BOOL remote_debugger = FALSE;
    if (::CheckRemoteDebuggerPresent(::GetCurrentProcess(), &remote_debugger) != 0) {
        value.tracer_present = remote_debugger != FALSE;
    }
#elif !defined(OH_DISABLE_ANTI_ANALYSIS) && defined(__linux__)
    if (auto* status = std::fopen("/proc/self/status", "r")) {
        char line[128]{};
        while (std::fgets(line, sizeof(line), status) != nullptr) {
            if (std::strncmp(line, "TracerPid:", 10) == 0) {
                value.tracer_present = std::atoi(line + 10) != 0;
                break;
            }
        }
        std::fclose(status);
    }
#endif
    return value;
}

namespace detail {

inline bool analysis_detected() {
    return probe().detected();
}

} // namespace detail

} // namespace oh::anti_analysis

#endif
