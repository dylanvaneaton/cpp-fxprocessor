#pragma once

#include "ControlNode.hpp"

struct ConstantNode : ControlNode {
    float constant = 0.0f;

    float get_value() override { return constant; }
};