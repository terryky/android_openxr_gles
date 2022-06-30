#ifndef OBOEPLAYER_H
#define OBOEPLAYER_H

#include <math.h>
#include <algorithm>
#include <oboe/Oboe.h>


class OboeSinePlayer: public oboe::AudioStreamCallback {
public:

    OboeSinePlayer()
    {
        oboe::AudioStreamBuilder builder;
        builder.setSharingMode     (oboe::SharingMode::Exclusive);
        builder.setPerformanceMode (oboe::PerformanceMode::LowLatency);
        builder.setFormat          (oboe::AudioFormat::Float);
        builder.setCallback        (this);
        builder.openManagedStream  (mStream);

        mChannelCount = mStream->getChannelCount();
        mSampleRate   = mStream->getSampleRate();

        mStream->requestStart();
    }

    void
    convertMonoToStereo (float *audioData, int32_t numFrames)
    {
        for (int i = numFrames - 1; i >= 0; i--)
        {
            for (int j = 0; j < mChannelCount; j++)
            {
                audioData[i * mChannelCount + j] = audioData[i];
            }
        }
    }

    oboe::DataCallbackResult
    onAudioReady (oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override
    {
        float *floatData = static_cast<float*>(audioData);
        double phaseIncrement = mFrequency * (2 * M_PI) / mSampleRate;

        /* [Monoral] Generate wave values */
        memset (audioData, 0, sizeof (float) * numFrames);
        for (int i = 0; i < numFrames; ++i)
        {
            mAmplitude *= mAmplitudeScaler;     /* fade     */
            if (mAmplitude < 0.01f)             /* cut off  */
            {
                mAmplitude = 0.0f;
                continue;
            }

            floatData[i] = mAmplitude * sinf (mPhase);

            mPhase += phaseIncrement;
            if (mPhase >= (2 * M_PI))
                mPhase -= (2 * M_PI);
        }

        /* [Stereo] */
        convertMonoToStereo (floatData, numFrames);

        return oboe::DataCallbackResult::Continue;
    }

    void
    enable (bool enable)
    {
        if (mCurEnable == enable)
            return;

        if (enable)
        {
            mPhase           = 0;
            mAmplitude       = 0.5f;
            mAmplitudeScaler = 1.0f;
        }
        else
        {
            mAmplitudeScaler = 0.999f;
        }
        mCurEnable = enable;
    }

    void
    setFrequency (float freq)
    {
        mFrequency = freq;
    }

private:
    oboe::ManagedStream mStream;

    int     mChannelCount;
    float   mSampleRate;

    float   mFrequency       = 440;
    float   mAmplitude       = 0.0f;
    float   mAmplitudeScaler = 1.0f;

    float   mPhase           = 0.0;
    bool    mCurEnable       = false;
};

#endif // OBOEPLAYER_H
