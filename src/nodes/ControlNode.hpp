#pragma once

#include "Node.hpp"

// Inherits from Node, adding a pure virtual function called get_value(), which is used primarily by
// AudioNodes to get their parameters from ControlNodes. It being pure virtual means there is no
// base version of it, all Nodes that inherit ControlNode need to implement their own version of it.

struct ControlNode : Node {
    // Gets a computed value that was calculated in a ControlNode's tick function.
    virtual float get_value() = 0;
};