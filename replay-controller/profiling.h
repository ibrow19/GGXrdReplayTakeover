#pragma once

#include <chrono>

#ifdef PROFILING

#define STATS(name) name ## ProfilingStats

#define DECLARE_PROFILING_CATEGORY(name) \
    struct STATS(name) \
    { \
        static double min; \
        static double max; \
        static double avg; \
        static bool bStarted; \
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

#define SCOPE_COUNTER(name) name ## ProfilingScopeCounter __LINE__ ## name ## ProfilingScopeCounter;

#else // PROFILING

#define DECLARE_PROFILING_CATEGORY(name)
#define DEFINE_PROFILING_CATEGORY(name)
#define SCOPE_COUNTER(name)

#endif
