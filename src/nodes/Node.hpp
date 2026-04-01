// This is necessary because if multiple different subnodes are included, multiple of them could try
// and include this same file at the same time, which would create a compile error from a duplicate
// definition. This tells the preprocessor to skip this file after it has been included once.
#pragma once

#include <cstdint>

// The Base Node. All Nodes inherit from this. Say a node is instantiated as type Node, but is a
// GainNode. If you then call Node.tick, it will be able to look up and know that you don't want
// this Node.tick, but the SineNode's overridden Node.tick.

struct Node {
    // This ensures that derived destructors run when deleting a base pointer. Important to not
    // forget when using virtuals because it can cause memory leaks.
    virtual ~Node() = default;

    // This runs once before audio starts. Can be overridden to allocate buffers or cache values
    // that depend on sample rate and buffer size, like the state of an lfo. Empty body means that
    // subnodes do not have to override this if they do not need to.
    virtual void prepare(double sample_rate, uint32_t max_frames) {}

    // This runs once per audio cycle before process(). Control nodes advance their internal state
    // here, audio only nodes can leave it alone as it is unnecessary for them.
    virtual void tick() {}
};