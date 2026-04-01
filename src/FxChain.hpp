#pragma once

#include "nodes/AudioNode.hpp"
#include "nodes/ControlNode.hpp"
#include <cstring>
#include <memory>
#include <vector>

// This is a linear effects chain. The audio goes linearly through each audio node to the output.
// All nodes in use are owned by this, using unique pointers. It will use two scratch buffers for
// the audio, switching them between the input and output buffer for each node in the processing
// chain. The process() function runs on JACK's rt thread, so no allocation, I/O, or blocking calls
// in it.

struct FxChain {
    // Vector of all Control Nodes, owned by this chain with their tick run each cycle.
    std::vector<std::unique_ptr<ControlNode>> control_nodes;

    // Vector of all Audio Nodes, owned by this chain and processed in order each cycle.
    std::vector<std::unique_ptr<AudioNode>> audio_nodes;

    // Scratch buffers as mentioned above. Allocated in prepare() for use in process(). std::vector
    // manages heap memory here.
    std::vector<float> buf_a;
    std::vector<float> buf_b;

    // The prepare is to be called after building the effects chain, but before activating JACK. For
    // pre-processing initialization.
    void prepare(double sample_rate, uint32_t max_frames) {
        // Sizes the buffers and zeros them according to the buffer size.
        buf_a.resize(max_frames);
        buf_b.resize(max_frames);

        // Runs prepare() on all control nodes.
        for (auto &node : control_nodes)
            node->prepare(sample_rate, max_frames);
        // Runs prepare() on all audio nodes.
        for (auto &node : audio_nodes)
            node->prepare(sample_rate, max_frames);
    }

    // This is used to transfer ownership of a control node into the chain. The unique pointer for
    // an instantiated node is moved into the input of this function using std::move, where it is
    // then moved into the ControlNodes vector from there.
    void add_control_node(std::unique_ptr<ControlNode> node) {
        control_nodes.push_back(std::move(node));
    }

    // Same as above, but for audio nodes.
    void add_audio_node(std::unique_ptr<AudioNode> node) { audio_nodes.push_back(std::move(node)); }

    // This is the process(), aka the callback. It takes a pointer to the JACK input buffer to read,
    // a pointer to the JACK output buffer to write to, and the buffer size.
    void process(const float *in, float *out, uint32_t nframes) {
        // Runs tick on all control nodes to make their state current to be used in this cycle.
        for (auto &node : control_nodes)
            node->tick();

        // If there are no audio nodes, write the audio straight from the input to the output.
        if (audio_nodes.empty()) {
            std::memcpy(out, in, sizeof(float) * nframes);
            return;
        }

        // If only one audio node, getting the buffers involved is unnecessary. Have the node read
        // and write directly to and from the JACK buffer.
        if (audio_nodes.size() == 1) {
            audio_nodes[0]->process(in, out, nframes);
            return;
        }

        // Start dealing with the first in a chain of multiple nodes. Start by passing the JACK
        // input buffer to the first node and having it write to buf_a.
        audio_nodes[0]->process(in, buf_a.data(), nframes);

        // This is the middle nodes. Odd index nodes read from buf_a and write to buf_b, Even index
        // nodes read from buf_a and write to buf_b.
        for (size_t i = 1; i < audio_nodes.size() - 1; i++) {
            float *src = (i % 2 == 1) ? buf_a.data() : buf_b.data();
            float *dst = (i % 2 == 1) ? buf_b.data() : buf_a.data();
            audio_nodes[i]->process(src, dst, nframes);
        }

        // The last middle node processed is the one before the last node. This means for an odd
        // amount of nodes, the last buffer written to by the second to last node is buf_a, and vice
        // versa for an even amount of nodes.
        float *last_src = (audio_nodes.size() % 2 == 1) ? buf_a.data() : buf_b.data();
        // .back() returns a reference to the last element of the vector.
        audio_nodes.back()->process(last_src, out, nframes);
    }
};
