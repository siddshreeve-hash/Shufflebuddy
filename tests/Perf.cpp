#include "../Source/PluginProcessor.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <execinfo.h>
#include <cxxabi.h>
static bool traced = false;
static std::atomic<bool> counting { false };
static std::atomic<long> allocs { 0 };
void* operator new (std::size_t n) { if (counting) { ++allocs; if (! traced) { traced = true; counting = false; void* bt[24]; int c = backtrace (bt, 24); backtrace_symbols_fd (bt, c, 2); counting = true; } } if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[] (std::size_t n) { if (counting) ++allocs; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    for (int bs : { 64, 128, 256, 512 })
    {
        ShuffleBuddyProcessor proc; proc.setPlayConfigDetails (0, 2, sr, bs);
        auto set = [&] (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        proc.applyStyleFeel (2);   // funk: busiest pattern
        set ("slot", 2); set ("ghosts", 1); set ("variety", 1); set ("dust", 1); set ("play", 1); set ("squash", 1); set ("throw", 1); set ("space", 1); set ("echo", 1); set ("fxfilter", -0.6f); set ("chain", 2); proc.applyKit (10); set ("layer1", 1); set ("autofill", 1); set ("ghosts", 0.8f); set ("variety", 0.8f);
        proc.prepareToPlay (sr, bs);
        juce::AudioBuffer<float> b (2, bs); juce::MidiBuffer m; m.ensureSize (2048);
        std::vector<double> t; t.reserve (40000); long total = 0; const int blocks = (int) (sr * 20 / bs);
        for (int k = 0; k < blocks; ++k)
        {
            m.clear();
            if (k % 300 == 50) set ("fill", (k / 300) % 2 ? 1.0f : 0.0f);
            allocs = 0; counting = k > 20;
            auto s = std::chrono::steady_clock::now();
            proc.processBlock (b, m);
            const auto e = std::chrono::steady_clock::now();
            counting = false; total += allocs;
            t.push_back (std::chrono::duration<double, std::micro> (e - s).count());
        }
        std::sort (t.begin() + 20, t.end());
        double mean = 0; for (size_t i = 20; i < t.size(); ++i) mean += t[i]; mean /= (double) (t.size() - 20);
        const double dl = bs / sr * 1e6;
        std::cout << "buffer " << bs << ": mean " << mean / dl * 100 << "%  p99 " << t[(size_t) (t.size() * 0.99)] / dl * 100 << "% of deadline;  heap allocations on audio thread: " << total << "\n";
    }
}
