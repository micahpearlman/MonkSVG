#ifndef MBGL_UTIL_BOX
#define MBGL_UTIL_BOX

#include "vec.hpp"

namespace MonkVG {

struct box {
    vec2<double> tl, tr, bl, br;
    vec2<double> center;
};

}

#endif
