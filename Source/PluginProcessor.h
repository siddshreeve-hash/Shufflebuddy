#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include "DrumVoices.h"
#include "Groove.h"

class ShuffleBuddyProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    ShuffleBuddyProcessor();
    ~ShuffleBuddyProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Shuffle Buddy"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static const char* padName (int p) { static const char* n[] = { "KICK", "SNARE", "CLAP", "RIM", "HAT", "OPEN", "PERC", "SHAKER" }; return n[juce::jlimit (0, 7, p)]; }
    static juce::String pid (const char* base, int pad) { return juce::String (base) + juce::String (pad + 1); }

    // ---- patterns (UI thread edits, audio thread reads; each cell is atomic)
    juce::uint8 cell (int slot, int pad, int step) const { return cells[(size_t) slot][(size_t) pad][(size_t) step].load(); }
    void setCell (int slot, int pad, int step, juce::uint8 v) { cells[(size_t) slot][(size_t) pad][(size_t) step].store (v); }
    groove::Grid grid (int slot) const;
    void setGrid (int slot, const groove::Grid&);
    void rollPattern();      // ROLL: new pattern in the current style, current slot
    void varyPattern();      // VARY: nudge current slot
    void applyStyleFeel (int style);   // style button: tempo, swing, ghosts, loosen, dust, pushes
    int currentSlot() const { return (int) apvts.getRawParameterValue ("slot")->load(); }

    // ---- pads
    void auditionPad (int pad, float vel) { pendingAudition[(size_t) pad].store (vel); }
    bool loadSample (int pad, const juce::File&);
    void clearSample (int pad);
    juce::String samplePath (int pad) const { auto s = sampleSlots[(size_t) pad]; return s != nullptr ? s->path : juce::String(); }

    // ---- telemetry for the UI
    int playStep() const        { return stepNow.load(); }
    int playBar() const         { return barNow.load(); }
    int padHits (int pad) const { return hitCount[(size_t) pad].load(); }
    float padLastVel (int pad) const { return hitVel[(size_t) pad].load(); }
    float outLevel() const      { return outLvl.load(); }

    juce::AudioFormatManager formats;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;   // frees replaced samples once the audio thread can't be using them
    void buildSlots();
    void scheduleStep (juce::int64 step, juce::int64 at, double sps);
    void prepareBar (juce::int64 bar);
    void trigger (int pad, float vel, int offset);

    // real-time-safe parameter access (hashed once at startup, no strings on the audio thread)
    struct Slot { juce::uint32 h = 0; std::atomic<float>* raw = nullptr; };
    std::array<Slot, 128> slots {};
    static juce::uint32 fnv (const char* s) { juce::uint32 h = 2166136261u; while (*s) { h ^= (juce::uint8) *s++; h *= 16777619u; } return h | 1u; }
    float p (const char* id) const { const auto h = fnv (id); size_t i = h & 127u; while (slots[i].h != h) i = (i + 1) & 127u; return slots[i].raw->load(); }
    std::array<std::atomic<float>*, NumPads> padLevel {}, padTune {}, padDecay {}, padPush {};

    std::array<std::array<std::array<std::atomic<juce::uint8>, groove::numSteps>, NumPads>, groove::numSlots> cells;

    // voices
    std::array<DrumVoice, 32> voices;
    std::array<std::atomic<SampleData*>, NumPads> liveSample {};
    std::array<SampleData::Ptr, NumPads> sampleSlots;
    struct Trash { SampleData::Ptr p; juce::uint32 at; };
    std::vector<Trash> trash;
    std::array<std::atomic<float>, NumPads> pendingAudition {};

    // sequencer
    struct Event { juce::int64 t; int pad; float vel; };
    std::array<Event, 512> queue {};
    int queued = 0;
    juce::int64 sampleClock = 0, lastScheduled = -1, preparedBar = -1;
    double stepPos = 0.0;
    bool wasPlaying = false;
    int activeSlot = 0;
    groove::Grid barGrid {};     // this bar's playing copy, with variation / fill / auto-ghosts applied
    juce::Random rng, uiRng;

    // dust
    float dustLpL = 0, dustLpR = 0, crackle = 0, crackleEnv = 0, hissLp = 0;
    bool fillApplied = false;
    juce::int64 crushHold = 0; float crushL = 0, crushR = 0;

    std::atomic<int> stepNow { 0 }, barNow { 0 };
    std::array<std::atomic<int>, NumPads> hitCount {};
    std::array<std::atomic<float>, NumPads> hitVel {};
    std::atomic<float> outLvl { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShuffleBuddyProcessor)
};
