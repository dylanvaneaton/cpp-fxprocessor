#pragma once

#include "Node.hpp"

// Inherits from Node, and adds a pure virtual function called process(). It being pure virtual
// means that every single subnode of AudioNode must implement process(), it does not have a base
// implementation.

struct AudioNode : Node {
    // in is the pointer to the read only buffer of input samples. const makes this read only.
    // out is the pointer to the buffer of output samples, this is where audio is written to.
    // nframes is the buffer size.
    virtual void process(const float *in, float *out, uint32_t nframes) = 0;
};