#include "view.hpp"
#include "still_image.hpp"

#include <cassert>

namespace MonkVG {

void View::initialize() {

}

std::unique_ptr<StillImage> View::readStillImage() {
    return nullptr;
}

void View::notifyMapChange(MapChange) const{
    // no-op
}

std::shared_ptr<Transform> getTransform() {

    return nullptr;
}

}
