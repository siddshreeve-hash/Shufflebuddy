#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "DrumVoices.h"
#include "Groove.h"
#include "DubDelay.h"
#include "DubReverb.h"
#include "FxFilter.h"
#include "SoundLibrary.h"

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
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static const char* padName (int p) { static const char* n[] = { "KICK", "SNARE", "CLAP", "RIM", "HAT", "OPEN", "PERC", "SHAKER", "TOMS", "CRASH" }; return n[juce::jlimit (0, NumPads - 1, p)]; }
    static juce::String pid (const char* base, int pad) { return juce::String (base) + juce::String (pad + 1); }

    // quick level view (0 off, 1 ghost, 2 normal, 3 big) + the full step data (velocity, chance, nudge, ratchet, tom pitch)
    juce::uint8 cell (int slot, int pad, int step) const { return (juce::uint8) groove::levelOfVel (cellData (slot, pad, step).vel); }
    void setCell (int slot, int pad, int step, juce::uint8 lvl)
    {
        auto c = cellData (slot, pad, step);
        if (lvl == 0) c = groove::Cell(); else c.vel = groove::velOfLevel (lvl);
        setCellData (slot, pad, step, c);
    }
    groove::Cell cellData (int slot, int pad, int step) const { return groove::unpack (cells[(size_t) slot][(size_t) pad][(size_t) step].load()); }
    void setCellData (int slot, int pad, int step, groove::Cell c) { cells[(size_t) slot][(size_t) pad][(size_t) step].store (groove::pack (c)); }
    groove::Grid grid (int slot) const;
    void setGrid (int slot, const groove::Grid&);
    void rollPattern();
    void varyPattern();
    void clearPattern();
    void applyStyleFeel (int style);
    int currentSlot() const { return (int) apvts.getRawParameterValue ("slot")->load(); }
    int playingSlot() const { return playSlotNow.load(); }
    // what the grid shows: the playing pattern while CHAIN runs, else the selected one
    // what the sequencer is ACTUALLY playing this bar, and why each hit differs from the stored pattern
    enum HitSource : juce::uint8 { FromPattern = groove::FromPattern, FromVariety = groove::FromVariety, FromGhosts = groove::FromGhosts, FromFill = groove::FromFill, Dropped = groove::Dropped };
    juce::uint8 playedCell (int pad, int step) const { return played[(size_t) pad][(size_t) step].load(); }
    juce::uint8 playedSource (int pad, int step) const { return playedSrc[(size_t) pad][(size_t) step].load(); }
    int playedSlot() const { return playedSlotNow.load(); }
    int viewSlot() const { return (apvts.getRawParameterValue ("chain")->load() > 0.5f && apvts.getRawParameterValue ("play")->load() > 0.5f) ? playingSlot() : currentSlot(); }

    // ---- sounds
    Patch kickPatch (double bpm) const;
    const RealSet* realSet (int index) const;
    void applyKickPreset (int index);
    void applyKit (int index);
    void surpriseKick();
    void setParamValue (const juce::String& id, float value);
    juce::String kickName { "Garage Thud" }, kitName { "2-Step Classic" };

    // ---- MIDI: what goes out to the DAW, RECORD (what actually played) and PRINT (render N bars offline)
    int noteFor (int lane, int alt) const;
    juce::MidiMessageSequence printBars (int bars);
    void startRecording()  { recSeq.clear(); recArmed.store (true); recStartBar.store (-1); recording = true; }
    void stopRecording();
    bool isRecording() const { return recording; }
    bool recordWaiting() const { return recording && recStartBar.load() < 0; }
    int recordedBars() const { return recBars; }
    const juce::MidiMessageSequence& recordedSequence() const { return recSeq; }
    void pollRecording();
    static bool writeMidiFile (const juce::MidiMessageSequence&, double bpm, const juce::File&);
    double tempoNow() const { return bpmNow; }

    void pushUndo (int slot);
    bool undo();
    bool canUndo() const { return ! history.empty(); }
    void copySlot (int from, int to);

    void auditionPad (int pad, float vel) { pendingAudition[(size_t) pad].store (vel); }
    bool loadSample (int pad, const juce::File&);
    void clearSample (int pad);
    juce::String samplePath (int pad) const { auto s = sampleSlots[(size_t) pad]; return s != nullptr ? s->path : juce::String(); }

    void startLearn (int paramIndex) { learnParam.store (paramIndex); }
    void cancelLearn() { learnParam.store (-1); }
    int  learningParam() const { return learnParam.load(); }
    void clearMapping (int paramIndex);
    int  ccFor (int paramIndex) const;
    int  mappingVersion() const { return mapVersion.load(); }

    int playStep() const        { return stepNow.load(); }
    int playBar() const         { return barNow.load(); }
    int padHits (int pad) const { return hitCount[(size_t) pad].load(); }
    float padLastVel (int pad) const { return hitVel[(size_t) pad].load(); }
    float outLevel() const      { return outLvl.load(); }
    float squashGR() const      { return grNow.load(); }
    float echoLevel() const     { return echoLvl.load(); }

    juce::AudioFormatManager formats;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void buildSlots();
    void loadRealSets();
    std::vector<RealSet> realSets;
    void runChunk (float* L, float* R, int n);
    void trigger (int pad, float vel, int alt = 1);
    groove::EngineIn engineIn() const;
    void planBar (groove::BarPlan& plan, juce::int64 bar, bool fillNow, bool prevFilled, const groove::EngineIn& in);
    groove::BarPlan* planFor (juce::int64 bar);

    struct Slot { juce::uint32 h = 0; std::atomic<float>* raw = nullptr; };
    std::array<Slot, 256> slots {};
    static juce::uint32 fnv (const char* s) { juce::uint32 h = 2166136261u; while (*s) { h ^= (juce::uint8) *s++; h *= 16777619u; } return h | 1u; }
    float p (const char* id) const { const auto h = fnv (id); size_t i = h & 255u; while (slots[i].h != h) i = (i + 1) & 255u; return slots[i].raw->load(); }
    std::array<std::atomic<float>*, NumPads> padLevel {}, padTune {}, padDecay {}, padPush {}, padEcho {}, padSpace {}, padLayer {}, padSnd {}, padReal {}, padRoom {}, padMute {}, padSolo {}, padSwing {}, padNote {};

    std::array<std::array<std::array<std::atomic<juce::uint64>, groove::numSteps>, NumPads>, groove::numSlots> cells;
    std::vector<std::pair<int, groove::Grid>> history;

    std::array<DrumVoice, 32> voices;
    std::array<std::atomic<SampleData*>, NumPads> liveSample {};
    std::array<SampleData::Ptr, NumPads> sampleSlots;
    struct Trash { SampleData::Ptr p; juce::uint32 at; };
    std::vector<Trash> trash;
    std::array<std::atomic<float>, NumPads> pendingAudition {};

    struct Event { juce::int64 t; int pad; float vel; int alt; bool seq; };
    std::array<Event, 512> queue {};
    int queued = 0;
    juce::int64 sampleClock = 0;
    std::array<groove::BarPlan, 2> plans;
    std::array<double, 2> emittedUpTo { -1.0e9, -1.0e9 };
    juce::int64 publishedBar = -1;
    bool fillWasOn = false;
    juce::uint32 engineSeed = 1;
    // MIDI out
    juce::MidiBuffer outMidi;
    int chunkBase = 0;
    struct NoteOff { juce::int64 t; int note; };
    std::array<NoteOff, 256> noteOffs {};
    int nNoteOffs = 0;
    // RECORD: audio thread -> message thread
    struct RecHit { double pos16; int note; float vel; };
    juce::AbstractFifo recFifo { 2048 };
    std::array<RecHit, 2048> recBuf {};
    std::atomic<bool> recArmed { false };
    std::atomic<juce::int64> recStartBar { -1 };
    juce::MidiMessageSequence recSeq;
    double recLastPos = 0.0;
    int recBars = 0;
    bool recording = false;
    double stepPos = 0.0, sps = 6000.0, bpmNow = 120.0;
    bool wasPlaying = false, hostRolling = false;
    double hostPpq = 0.0;
    int activeSlot = 0;
    juce::Random rng, uiRng;

    static constexpr int maxChunk = 512;
    juce::AudioBuffer<float> eBuf, sBuf, kBuf, rBuf;
    int rumbleTail = 0;
    DubReverb rumbleVerb;
    Svf rumbleLp1, rumbleLp2;
    float duckEnv = 0.0f; int duckHold = 0;
    Patch kickNow;
    DubDelay echo;
    DubReverb space;
    FxFilter fxFilter;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    float throwAmt = 0.0f, throwTail = 0.0f;
    float compEnv = 0.0f, compGain = 1.0f;

    float dustLpL = 0, dustLpR = 0, crackle = 0, crackleEnv = 0, hissLp = 0;
    juce::int64 crushHold = 0; float crushL = 0, crushR = 0;

    std::array<std::atomic<int>, 128> ccMap;
    std::atomic<int> learnParam { -1 }, mapVersion { 0 };

    std::atomic<int> stepNow { 0 }, barNow { 0 }, playSlotNow { 0 }, playedSlotNow { -1 };
    std::array<std::array<std::atomic<juce::uint8>, groove::numSteps>, NumPads> played, playedSrc;
    void publishBar (const groove::BarPlan&);
    std::array<std::atomic<int>, NumPads> hitCount {};
    std::array<std::atomic<float>, NumPads> hitVel {};
    std::atomic<float> outLvl { 0.0f }, grNow { 0.0f }, echoLvl { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShuffleBuddyProcessor)
};
