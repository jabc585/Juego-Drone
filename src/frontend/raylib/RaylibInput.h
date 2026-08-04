#pragma once

#include "frontend/IInputSource.h"

namespace drone {

class RaylibInput : public IInputSource {
public:
    Command poll() override;
};

}  // namespace drone
