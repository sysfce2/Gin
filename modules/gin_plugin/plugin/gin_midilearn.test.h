//==============================================================================
#if GIN_UNIT_TESTS

// Minimal mock Processor with a couple of real parameters
class MockProcessorForMidiLearnTests : public gin::Processor
{
public:
    MockProcessorForMidiLearnTests() : gin::Processor (false)
    {
        volume = addExtParam ("volume", "Volume", "Vol", "", { 0.0f, 1.0f }, 0.0f, { 0.0f });
        cutoff = addExtParam ("cutoff", "Cutoff", "Cut", "", { 0.0f, 1.0f }, 0.0f, { 0.0f });
    }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    gin::Parameter* volume = nullptr;
    gin::Parameter* cutoff = nullptr;
};

class MidiLearnTests : public juce::UnitTest
{
public:
    MidiLearnTests() : juce::UnitTest ("MidiLearn", "gin_plugin") {}

    void runTest() override
    {
        const juce::MessageManagerLock mmLock;

        testValidCC();
        testMappedCCAppliesToParameter();
        testLearning();
        testIgnoredCCs();
    }

private:
    static void sendCC (gin::MidiLearn& ml, int cc, int value)
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
        ml.processBlock (midi, 512);
    }

    static void endGestures (gin::MidiLearn& ml)
    {
        juce::MidiBuffer midi;
        ml.processBlock (midi, 44100); // run past the gesture timeout
    }

    static void clearAllMappings (gin::MidiLearn& ml)
    {
        // Load an empty state so mappings saved to settings by earlier runs don't leak in
        juce::ValueTree vt ("state");
        vt.getOrCreateChildWithName ("MIDILEARN", nullptr);
        ml.loadState (vt);
    }

    void testValidCC()
    {
        beginTest ("Valid CCs");

        expect (! gin::MidiLearn::isValidCC (0),   "CC 0 (Bank Select MSB) should be invalid");
        expect (! gin::MidiLearn::isValidCC (32),  "CC 32 (Bank Select LSB) should be invalid");
        expect (! gin::MidiLearn::isValidCC (120), "CC 120+ (Channel Mode) should be invalid");
        expect (! gin::MidiLearn::isValidCC (127), "CC 127 should be invalid");
        expect (gin::MidiLearn::isValidCC (1),     "CC 1 (Mod Wheel) should be valid");
        expect (gin::MidiLearn::isValidCC (64),    "CC 64 (Sustain) should be valid");
    }

    void testMappedCCAppliesToParameter()
    {
        beginTest ("Mapped CC Applies To Parameter");

        MockProcessorForMidiLearnTests proc;
        gin::MidiLearn ml (proc);
        ml.setSampleRate (44100.0);
        clearAllMappings (ml);

        ml.setMapping (7, proc.volume);
        expect (ml.getMapping (7) == proc.volume, "Mapping should be stored");
        expectEquals (ml.getMappedCC (proc.volume), 7, "Mapped CC should be found from parameter");

        sendCC (ml, 7, 127);
        expectWithinAbsoluteError (proc.volume->getValue(), 1.0f, 0.001f, "CC 127 should set parameter to max");

        sendCC (ml, 7, 0);
        expectWithinAbsoluteError (proc.volume->getValue(), 0.0f, 0.001f, "CC 0 should set parameter to min");

        endGestures (ml);
    }

    void testLearning()
    {
        beginTest ("Learning");

        MockProcessorForMidiLearnTests proc;
        gin::MidiLearn ml (proc);
        ml.setSampleRate (44100.0);
        clearAllMappings (ml);

        ml.startLearning (proc.cutoff);
        expect (ml.isLearning(), "Should be learning");

        sendCC (ml, 20, 80); // mid range value, moved well past the threshold
        expect (! ml.isLearning(), "Learning should complete");
        expect (ml.getMapping (20) == proc.cutoff, "CC 20 should map to the learning parameter");

        endGestures (ml);
    }

    void testIgnoredCCs()
    {
        beginTest ("Ignored CCs");

        MockProcessorForMidiLearnTests proc;
        gin::MidiLearn ml (proc);
        ml.setSampleRate (44100.0);
        clearAllMappings (ml);

        ml.setIgnoredCCs ({ 64 });
        expect (ml.isIgnoredCC (64), "CC 64 should be ignored");
        expect (! ml.isIgnoredCC (7), "CC 7 should not be ignored");

        // Ignored CCs don't get learned
        ml.startLearning (proc.volume);
        sendCC (ml, 64, 127);
        expect (ml.isLearning(), "Ignored CC should not complete learning");
        expect (ml.getMapping (64) == nullptr, "Ignored CC should not be mapped");

        // But other CCs still do
        sendCC (ml, 10, 100);
        expect (! ml.isLearning(), "Other CCs should still learn");
        expect (ml.getMapping (10) == proc.volume, "CC 10 should map");
        endGestures (ml);

        // Ignored CCs don't change parameters even if a mapping is forced
        ml.setMapping (64, proc.cutoff);
        proc.cutoff->setValue (0.5f);
        sendCC (ml, 64, 127);
        expectWithinAbsoluteError (proc.cutoff->getValue(), 0.5f, 0.001f, "Ignored CC should not move the parameter");

        // Setting the ignore list drops existing mappings on those CCs
        ml.setIgnoredCCs ({ 10, 64 });
        expect (ml.getMapping (10) == nullptr, "Ignoring a CC should drop its mapping");
        expect (ml.getMapping (64) == nullptr, "Ignoring a CC should drop its mapping");
    }
};

static MidiLearnTests midiLearnTests;

#endif
