#pragma once

#ifdef PROFILING

#include <chrono>

#define STATS(name) name ## ProfilingStats

#define DECLARE_PROFILING_CATEGORY(name) \
    struct STATS(name) \
    { \
        static double min; \
        static double max; \
        static double avg; \
        static bool bStarted; \
        static constexpr unsigned int recentCount = 1 << 6; \
        static double recent[recentCount]; \
        static unsigned int recentPos; \
        static unsigned int recentUsed; \
    }; \
    struct name ## ProfilingScopeCounter\
    { \
        name ## ProfilingScopeCounter() \
        { \
            start = std::chrono::steady_clock::now(); \
        } \
        std::chrono::time_point<std::chrono::steady_clock> start; \
        ~name ## ProfilingScopeCounter() \
        { \
            double time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() / 1000000.0; \
            double& min = STATS(name)::min; \
            double& max = STATS(name)::max; \
            double& avg = STATS(name)::avg; \
            bool& bStarted = STATS(name)::bStarted; \
            STATS(name)::recent[STATS(name)::recentPos++ % STATS(name)::recentCount] = time; \
            if (STATS(name)::recentUsed < STATS(name)::recentCount) \
            { \
                ++STATS(name)::recentUsed; \
            } \
            if (!bStarted) \
            { \
                min = time; \
                max = time; \
                avg = time; \
                bStarted = true; \
            } \
            else \
            { \
                min = time < min ? time : min; \
                max = time > max ? time : max; \
                avg = avg + (time - avg) * 0.05; \
            } \
        } \
    };

#define DEFINE_PROFILING_CATEGORY(name) \
    double STATS(name)::min = 0.; \
    double STATS(name)::max = 0.; \
    double STATS(name)::avg = 0.; \
    bool STATS(name)::bStarted = false;  \
    double STATS(name)::recent[STATS(name)::recentCount] = {0.}; \
    unsigned int STATS(name)::recentPos = 0;  \
    unsigned int STATS(name)::recentUsed = 0;

#define SCOPE_COUNTER(name) name ## ProfilingScopeCounter __LINE__ ## name ## ProfilingScopeCounter;

#define RECENT_RESULT(name, index) (STATS(name)::recent[(STATS(name)::recentPos + index) % STATS(name)::recentCount])

#ifdef USE_IMGUI_OVERLAY
#include <implot.h>
#define STAT_GRAPH(name) \
    ImPlot::SetNextAxesLimits(0, STATS(name)::recentCount, 0, STATS(name)::max, ImPlotCond_Always); \
    if (ImPlot::BeginPlot(#name, 0, "Time(ms)", ImVec2(-1, 200), 0, ImPlotAxisFlags_None, ImPlotAxisFlags_None)) \
    { \
        ImPlot::SetNextLineStyle(ImVec4(0, 200, 50, 255)); \
        ImPlot::PlotLineG(#name, \
            [](int index, void* data) \
            { \
                return ImPlotPoint(index, RECENT_RESULT(name, index)); \
            }, \
            nullptr, \
            STATS(name)::recentCount); \
        ImPlot::EndPlot(); \
    } \

#endif // USE_IMGUI_OVERLAY

#else // PROFILING

#define DECLARE_PROFILING_CATEGORY(name)
#define DEFINE_PROFILING_CATEGORY(name)
#define SCOPE_COUNTER(name)

#endif
