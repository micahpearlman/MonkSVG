#include "constants.hpp"

const float MonkVG::util::tileSize = 512.0f;

const double MonkVG::util::DEG2RAD = M_PI / 180.0;
const double MonkVG::util::RAD2DEG = 180.0 / M_PI;
const double MonkVG::util::M2PI = 2 * M_PI;
const double MonkVG::util::EARTH_RADIUS_M = 6378137;
const double MonkVG::util::LATITUDE_MAX = 85.05112878;

#if defined(DEBUG)
const bool MonkVG::debug::tileParseWarnings = false;
const bool MonkVG::debug::styleParseWarnings = false;
const bool MonkVG::debug::spriteWarnings = false;
const bool MonkVG::debug::renderWarnings = false;
const bool MonkVG::debug::renderTree = false;
const bool MonkVG::debug::labelTextMissingWarning = true;
const bool MonkVG::debug::missingFontStackWarning = true;
const bool MonkVG::debug::missingFontFaceWarning = true;
const bool MonkVG::debug::glyphWarning = true;
const bool MonkVG::debug::shapingWarning = true;
#else
const bool MonkVG::debug::tileParseWarnings = false;
const bool MonkVG::debug::styleParseWarnings = false;
const bool MonkVG::debug::spriteWarnings = false;
const bool MonkVG::debug::renderWarnings = false;
const bool MonkVG::debug::renderTree = false;
const bool MonkVG::debug::labelTextMissingWarning = false;
const bool MonkVG::debug::missingFontStackWarning = false;
const bool MonkVG::debug::missingFontFaceWarning = false;
const bool MonkVG::debug::glyphWarning = false;
const bool MonkVG::debug::shapingWarning = false;
#endif