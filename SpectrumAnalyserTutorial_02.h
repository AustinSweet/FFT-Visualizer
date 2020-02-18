/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SpectrumAnalyserTutorial
 version:          2.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Displays an FFT spectrum analyser.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_dsp, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        AnalyserComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class AnalyserComponent   : public AudioAppComponent,
                            private Timer
{
public:
    AnalyserComponent()
        : forwardFFT (fftOrder),
          window (fftSize, dsp::WindowingFunction<float>::hann)
    {
        setOpaque (true);
        setAudioChannels (2, 0);  // we want a couple of input channels but no outputs
        startTimerHz (30);
        setSize (700, 500);
    }

    ~AnalyserComponent() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int, double) override {}
    void releaseResources() override          {}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        if (bufferToFill.buffer->getNumChannels() > 0)
        {
            auto* channelData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);

            for (auto i = 0; i < bufferToFill.numSamples; ++i)
                pushNextSampleIntoFifo (channelData[i]);
        }
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);

        g.setOpacity (1.0f);
        g.setColour (Colours::white);
        drawFrame (g);
    }

    void timerCallback() override
    {
        if (nextFFTBlockReady)
        {
            drawNextFrameOfSpectrum();
            nextFFTBlockReady = false;
            repaint();
        }
    }

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        // if the fifo contains enough data, set a flag to say
        // that the next frame should now be rendered..
        if (fifoIndex == fftSize)
        {
            if (! nextFFTBlockReady)
            {
                zeromem (fftData, sizeof (fftData));
                memcpy (fftData, fifo, sizeof (fifo));
                nextFFTBlockReady = true;
            }

            fifoIndex = 0;
        }

        fifo[fifoIndex++] = sample;
    }

    void drawNextFrameOfSpectrum()
    {
        // first apply a windowing function to our data
        window.multiplyWithWindowingTable (fftData, fftSize);

        // then render our FFT data..
        forwardFFT.performFrequencyOnlyForwardTransform (fftData);

        auto mindB = -100.0f;
        auto maxdB =    0.0f;

        for (int i = 0; i < scopeSize; ++i)
        {
            auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - i / (float) scopeSize) * 0.2f);
            auto fftDataIndex = jlimit (0, fftSize / 2, (int) (skewedProportionX * fftSize / 2));
            auto level = jmap (jlimit (mindB, maxdB, Decibels::gainToDecibels (fftData[fftDataIndex])
                                                   - Decibels::gainToDecibels ((float) fftSize)),
                               mindB, maxdB, 0.0f, 1.0f);

            scopeData[i] = level;
        }
    }

    void drawFrame (Graphics& g)
    {
        for (int i = 1; i < scopeSize; ++i)
        {
            auto width  = getLocalBounds().getWidth();
            auto height = getLocalBounds().getHeight();

            g.drawLine ({ (float) jmap (i - 1, 0, scopeSize - 1, 0, width),
                                  jmap (scopeData[i - 1], 0.0f, 1.0f, (float) height, 0.0f),
                          (float) jmap (i,     0, scopeSize - 1, 0, width),
                                  jmap (scopeData[i],     0.0f, 1.0f, (float) height, 0.0f) });
        }
    }

    enum
    {
        fftOrder  = 11,
        fftSize   = 1 << fftOrder,
        scopeSize = 512
    };

private:
    dsp::FFT forwardFFT;
    dsp::WindowingFunction<float> window;

    float fifo [fftSize];
    float fftData [2 * fftSize];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float scopeData [scopeSize];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserComponent)
};
