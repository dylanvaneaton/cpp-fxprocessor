#pragma once

#include "AudioNode.hpp"
#include "ControlNode.hpp"
#include <vector>

// This is a reverb that works by using 4 comb filters in parallel into 2 allpass filters in series.
// NEEDS WORK. SAMPLE RATE CURRENTLY HARD CODED, THIS IS A LATER ME PROBLEM.

// I should come back later and figure out how this works.
struct CombFilter {
    std::vector<float> buffer;
    int write_pos = 0;
    float feedback;
    float damp;
    float last = 0.0f;

    CombFilter(int delay_samples, float feedback, float damp)
        : buffer(delay_samples, 0.0f), feedback(feedback), damp(damp) {}

    float tick(float input) {
        float output = buffer[write_pos];
        last = (output * (1.0f - damp)) + (last * damp);
        buffer[write_pos] = input + last * feedback;
        write_pos = (write_pos + 1) % buffer.size();
        return output;
    }
};

struct AllpassFilter {
    std::vector<float> buffer;
    int write_pos = 0;
    float feedback;

    AllpassFilter(int delay_samples, float feedback)
        : buffer(delay_samples, 0.0f), feedback(feedback) {}

    float tick(float input) {
        float delayed = buffer[write_pos];
        float output = -input + delayed;
        buffer[write_pos] = input + delayed * feedback;
        write_pos = (write_pos + 1) % buffer.size();
        return output;
    }
};

struct ReverbNode : AudioNode {
    // Classic Schroeder tunings, scaled to sample rate. They are apparently chosen to be mutually
    // prime so echoes do not line up creating metallic artifacts.
    static constexpr float SCALE = 48000.0f / 48000.0f;

    CombFilter combs[4] = {{static_cast<int>(1116 * SCALE), 0.84f, 0.2f},
                           {static_cast<int>(1188 * SCALE), 0.84f, 0.2f},
                           {static_cast<int>(1277 * SCALE), 0.84f, 0.2f},
                           {static_cast<int>(1356 * SCALE), 0.84f, 0.2f}};

    AllpassFilter allpasses[2] = {{static_cast<int>(556 * SCALE), 0.5f},
                                  {static_cast<int>(441 * SCALE), 0.5f}};

    // wet/dry mix 1.0f being full wet, 0.0f being full dry
    ControlNode *mix_source = nullptr;
    float mix_default = 0.3f;

    void process(const float *in, float *out, uint32_t nframes) override {
        float mix = mix_source ? mix_source->get_value() : mix_default;

        for (uint32_t i = 0; i < nframes; i++) {
            float dry = in[i];
            float wet = 0.0f;

            // sum all 4 comb filters in parallel
            for (auto &c : combs)
                wet += c.tick(dry);

            // scale down, 4 combs summed = 4x louder
            wet *= 0.25f;

            // pass through allpass filters in series
            for (auto &a : allpasses)
                wet = a.tick(wet);

            out[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
};