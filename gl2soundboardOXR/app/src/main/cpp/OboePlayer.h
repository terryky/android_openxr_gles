#ifndef OBOEPLAYER_H
#define OBOEPLAYER_H

#include <math.h>
#include <algorithm>
#include <oboe/Oboe.h>

#define NOTE_NUM   12 * 4

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

        LOGI ("-----------------------------------");
        LOGI ("  mChannelCount = %d", mChannelCount);
        LOGI ("  mSampleRate   = %f", mSampleRate  );   /* 48,000 */
        LOGI ("-----------------------------------");

        mFrequency[0] = 130.813f;   /* C3 */
        for (int inote = 1; inote < NOTE_NUM; inote ++)
        {
            mFrequency[inote] = mFrequency[inote - 1] * 1.059463094f;
            mAmplitude[inote] = 0.0f;
            mCurEnable[inote] = false;
        }
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

    /*
     *  request 192 [frame] of audio data.
     *  If the sample rate is 48,000[frame/sec], 192/48,000 = 4[ms]
     */
    oboe::DataCallbackResult
    onAudioReady (oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override
    {
        float *floatData = static_cast<float*>(audioData);

        /* [Monoral] Generate wave values */
        memset (floatData, 0, sizeof (float) * numFrames);

        for (int inote = 0; inote < NOTE_NUM; inote ++)
        {
            double phaseIncrement  = mFrequency[inote] * (2 * M_PI) / mSampleRate;
            double amplitudeScaler = 1.0f - mAmplitudeScaler[inote] * mDumper;
            for (int i = 0; i < numFrames; ++i)
            {
                mAmplitude[inote] *= amplitudeScaler;   /* fade     */
                if (mAmplitude[inote] < 0.01f)          /* cut off  */
                {
                    mAmplitude[inote] = 0.0f;
                    break;
                }

                floatData[i] += mAmplitude[inote] * sinf (mPhase[inote]);

                mPhase[inote] += phaseIncrement;
                if (mPhase[inote] >= (2 * M_PI))
                    mPhase[inote] -= (2 * M_PI);
            }
        }

        /* [Stereo] */
        convertMonoToStereo (floatData, numFrames);

        return oboe::DataCallbackResult::Continue;
    }

    void
    enable (int id, bool enable)
    {
        if (mCurEnable[id] == enable)
            return;

        if (enable)
        {
            mPhase[id]           = 0;
            mAmplitude[id]       = 0.5f;
            mAmplitudeScaler[id] = 0.00001f;
        }
        else
        {
            mAmplitudeScaler[id] = 0.00010f;
        }
        mCurEnable[id] = enable;
    }

    void
    setDumper (float dumper)
    {
        /* 
         *  0 -> 1.0f
         *  1 -> 0.1f
         */
        mDumper = 1.0f - 0.9f * dumper;
    }

private:
    oboe::ManagedStream mStream;

    int     mChannelCount;
    float   mSampleRate;

    float   mFrequency[NOTE_NUM]       = {};
    float   mAmplitude[NOTE_NUM]       = {};
    float   mAmplitudeScaler[NOTE_NUM] = {};

    float   mPhase   [NOTE_NUM]        = {};
    bool    mCurEnable[NOTE_NUM]       = {};

    float   mDumper                    = 1.0f;
};

#endif // OBOEPLAYER_H
