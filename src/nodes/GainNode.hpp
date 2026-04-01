#pragma once

#include "AudioNode.hpp"
#include "ControlNode.hpp"

// Inherits from AudioNode, multiplying the input audio by a gain factor and outputting it. It's
// only control input is the gain factor.

struct GainNode : AudioNode {
    // This is a pointer to a ControlNode that is attached to the gain_factor. It is worth
    // mentioning this is a raw pointer, which is relavent because we should be using unique
    // pointers in the graph so they get destroyed properly.
    ControlNode *gain_source = nullptr;

    // C++ literals without the suffix are treated as a double, not a float. Hence the f suffix.
    // This is the value for the gain factor in case the gain_factor is nullptr, which could mean it
    // isn't connected to any ControlNode.
    float gain_default = 1.0f;

    void process(const float *in, float *out, uint32_t nframes) override {
        // Ternary that decides which gain_factor to use. Basically, if gain_source is not null, run
        // get_value() and use that. If it is null, use the default.
        float gain_factor = gain_source ? gain_source->get_value() : gain_default;

        // Iterate over every sample in the buffer and multiply it by the gain_factor. Worth noting
        // this runs for every single sample, therefore keeping it nice and simple is good for
        // performance.
        for (uint32_t i = 0; i < nframes; i++)
            out[i] = in[i] * gain_factor;
    }
};