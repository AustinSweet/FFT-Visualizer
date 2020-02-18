#pragma once

// Setting up the general parameters in the class

class AnalyserComponent   : public AudioAppComponent,
                            private Timer
{
public:
    AnalyserComponent() : forwardFFT (fftOrder), window (fftSize, dsp::WindowingFunction<float>::hamming)
    {
        setOpaque (true);
        setAudioChannels (2, 0);
        startTimerHz (30);
        setSize (700, 500);
    }

    ~AnalyserComponent() override
    {
        shutdownAudio();
    }

    //It's worth noting that we have to keep the empty included functions in order to use the JUCE framework. 
    //Quit panicking. It's the way stuff works. 

    void prepareToPlay (int, double) override {}
    void releaseResources() override          {}

    //Setting up our I/O buffers

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override {
    
        if (bufferToFill.buffer->getNumChannels() > 0) {

            auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

            for (auto i = 0; i < bufferToFill.numSamples; ++i)
                pushNextSampleIntoFifo(channelData[i]);
        }
    }

    //Making our visuals pretty

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::darkslategrey);

        g.setOpacity (1.0f);
        g.setColour (Colours::ghostwhite);
        drawFrame (g);
    }

    void timerCallback() override {
    
        if (nextFFTBlockReady) {
            drawNextFrameOfSpectrum();
            nextFFTBlockReady = false;
            repaint();

        }
    
    }

    //More buffer stuffers, as well as the beginning of 
    //actually passing audio into the FFT calculator

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        if (fifoIndex == fftSize) {

            if (!nextFFTBlockReady) {
                zeromem(fftData, sizeof(fftData));
                memcpy(fftData, fifo, sizeof(fifo));
                nextFFTBlockReady = true;

            }
            fifoIndex = 0;
        }
        fifo[fifoIndex++] = sample;
    }

    //The instructions for how our visuals should be handled. 
    //This particular FFT is biased towards the human vocal range.
    //This is intentional to demonstrate understanding of the
    //FFT calculations. 

    void drawNextFrameOfSpectrum() {
    
        window.multiplyWithWindowingTable(fftData, fftSize);

        forwardFFT.performFrequencyOnlyForwardTransform(fftData);

        auto mindb = -115.0f;
        auto maxdb = -20.0f;

        for (int i = 0; i < scopeSize; ++i) {

            auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - i / (float)scopeSize) * 0.1f);
            auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionX * fftSize / 3));
            auto level = jmap(jlimit(mindb, maxdb, Decibels::gainToDecibels(fftData[fftDataIndex])
                - Decibels::gainToDecibels((float)fftSize)),
                mindb, maxdb, 0.0f, 1.0f);
            scopeData[i] = level;

        }
    
    
    }
    void drawFrame (Graphics& g)     {
        
        for (int i = 1; i < scopeSize; ++i) {
            auto width = getLocalBounds().getWidth();
            auto height = getLocalBounds().getHeight();

            g.drawLine({ (float)jmap(i - 1, 0, scopeSize - 1, 0, width),
                jmap(scopeData[i - 1], 0.1f, 1.0f, (float)height, 0.0f),
                (float)jmap(i, 0, scopeSize - 1, 0, width),
                jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f) });

        }
    }

    enum
    {
        fftOrder = 11,
        fftSize = 1 << fftOrder,
        scopeSize = 256
    };

private:
   
    //Importing the JUCE DSP I/O Modules we will need

    dsp::FFT forwardFFT;
    dsp::WindowingFunction<float> window;

   //Defining the values we will use for the above code blocks

    float fifo [fftSize];
    float fftData[2 * fftSize];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float scopeData[scopeSize];



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserComponent)
};
