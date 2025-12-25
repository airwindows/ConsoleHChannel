#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor, public juce::AudioProcessorParameter::Listener
{
public:
    PluginProcessor();
    ~PluginProcessor() override;
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool starting) override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    bool supportsDoublePrecisionProcessing() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    // updateTrackProperties may be called by the host if it feels like it
    // this method calls a similar one in the editor class that updates the editor
    void updateTrackProperties(const TrackProperties& properties) override;
    void updatePluginSize(int w, int h);
    
    TrackProperties trackProperties;
    int pluginWidth;
    int pluginHeight;

    //now we can declare variables used in the audio thread
    static const int dscBuf = 256;
    
    enum Parameters
    {
        KNOBTRM,
        KNOBMOR,
        KNOBHIG,
        KNOBMID,
        KNOBLOW,
        KNOBCRS,
        KNOBTRF,
        KNOBTRG,
        KNOBTRB,
        KNOBHMF,
        KNOBHMG,
        KNOBHMB,
        KNOBLMF,
        KNOBLMG,
        KNOBLMB,
        KNOBBSF,
        KNOBBSG,
        KNOBBSB,
        KNOBTHR,
        KNOBATK,
        KNOBRLS,
        KNOBGAT,
        KNOBLOP,
        KNOBHIP,
        KNOBPAN,
        KNOBFAD,
    };
    static constexpr int n_params = KNOBFAD + 1;
    std::array<juce::AudioParameterFloat *, n_params> params;
    //This is where we're defining things that go into the plugin's interface.
    
    struct UIToAudioMessage
    {
        enum What
        {
            NEW_VALUE,
            BEGIN_EDIT,
            END_EDIT
        } what{NEW_VALUE};
        Parameters which;
        float newValue = 0.0;
    };
    //This is things the interface can tell the audio thread about.
    
    struct AudioToUIMessage
    {
        enum What
        {
            NEW_VALUE,
            RMS_LEFT,
            RMS_RIGHT,
            PEAK_LEFT,
            PEAK_RIGHT,
            SLEW_LEFT,
            SLEW_RIGHT,
            ZERO_LEFT,
            ZERO_RIGHT,
            BLINKEN_COMP,
            BLINKEN_GATE,
            BLINKEN_ATK,
            BLINKEN_RLS,
            BLINKEN_PAN,
            INCREMENT
        } what{NEW_VALUE};
        Parameters which;
        float newValue = 0.0;
    };
    //This is kinds of information the audio thread can give the interface.
    
    template <typename T, int qSize = 4096> class LockFreeQueue
    {
      public:
        LockFreeQueue() : af(qSize) {}
        bool push(const T &ad)
        {
            auto ret = false;
            int start1, size1, start2, size2;
            af.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 > 0)
            {
                dq[start1] = ad;
                ret = true;
            }
            af.finishedWrite(size1 + size2);
            return ret;
        }
        bool pop(T &ad)
        {
            bool ret = false;
            int start1, size1, start2, size2;
            af.prepareToRead(1, start1, size1, start2, size2);
            if (size1 > 0)
            {
                ad = dq[start1];
                ret = true;
            }
            af.finishedRead(size1 + size2);
            return ret;
        }
        juce::AbstractFifo af;
        T dq[qSize];
    };
    
    LockFreeQueue<UIToAudioMessage> uiToAudio;
    LockFreeQueue<AudioToUIMessage> audioToUI;
    
	enum {
		biq_freq,
		biq_reso,
		biq_a0,
		biq_a1,
		biq_a2,
		biq_b1,
		biq_b2,
		biq_sL1,
		biq_sL2,
		biq_sR1,
		biq_sR2,
		biq_total
	}; //coefficient interpolating filter, stereo
	double highFast[biq_total];
	double lowFast[biq_total];
	double highFastLIIR;
	double highFastRIIR;
	double lowFastLIIR;
	double lowFastRIIR;
	//SmoothEQ3
	
	enum { 
		biqs_freq, biqs_reso, biqs_level,
		biqs_temp, biqs_bit,
		biqs_a0, biqs_a1, biqs_b1, biqs_b2,
		biqs_c0, biqs_c1, biqs_d1, biqs_d2,
		biqs_aL1, biqs_aL2, biqs_aR1, biqs_aR2,
		biqs_cL1, biqs_cL2, biqs_cR1, biqs_cR2,
		biqs_outL, biqs_outR, biqs_total
	};
	double high[biqs_total];
	double hmid[biqs_total];
	double lmid[biqs_total];	
	double bass[biqs_total];	
	//HipCrush with four bands
	
	enum {
		bez_A,
		bez_B,
		bez_C,
		bez_Ctrl,
		bez_cycle,
		bez_total
	}; //the new undersampling. bez signifies the bezier curve reconstruction
	double bezComp[bez_total];
	double bezMax;
	double bezMin;
	double bezGate;
	//Dynamics3
	
	double iirHPositionL[23];
	double iirHAngleL[23];
	double iirHPositionR[23];
	double iirHAngleR[23];
	bool hBypass;
	double iirLPositionL[15];
	double iirLAngleL[15];
	double iirLPositionR[15];
	double iirLAngleR[15];
	bool lBypass;
	double lFreqA;
	double lFreqB; //the lowpass
	double hFreqA;
	double hFreqB; //the highpass
	//Cabs2
	
	double dBaL[dscBuf+5];
	double dBaPosL;
	int dBaXL;
	double dBaR[dscBuf+5];
	double dBaPosR;
	int dBaXR;
	//Discontapeity
	
	double avg32L[33];
	double avg32R[33];
	double avg16L[17];
	double avg16R[17];
	double avg8L[9];
	double avg8R[9];
	double avg4L[5];
	double avg4R[5];
	double avg2L[3];
	double avg2R[3];
	int avgPos;
	double lastSlewL;
	double lastSlewR;
	double lastSlewpleL;
	double lastSlewpleR;
	//preTapeHack	
	
	double panA;
	double panB;
	double inTrimA;
	double inTrimB;
	
    uint32_t fpdL;
    uint32_t fpdR;
    //default stuff
    
    double peakLeft = 0.0;
    double peakRight = 0.0;
    double slewLeft = 0.0;
    double slewRight = 0.0;
    double rmsLeft = 0.0;
    double rmsRight = 0.0;
    double previousLeft = 0.0;
    double previousRight = 0.0;
    double zeroLeft = 0.0;
    double zeroRight = 0.0;
    double longestZeroLeft = 0.0;
    double longestZeroRight = 0.0;
    double maxComp = 1.0;
    double maxGate = 1.0;
    double maxAttack = 0.0;
    double maxRelease = 0.0;
    double baseComp = 0.0;
    double baseGate = 0.0;
    bool wasPositiveL = false;
    bool wasPositiveR = false;
    int rmsCount = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
