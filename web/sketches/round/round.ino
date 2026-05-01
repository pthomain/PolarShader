#include "RoundDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "display/WebFastLedDisplay.h"

using namespace PolarShader;

namespace {
    constexpr uint8_t BRIGHTNESS = 30;
    constexpr uint8_t REFRESH_MS = 30;

    WebFastLedDisplay<RoundDisplaySpec> *display = nullptr;
}

void setup() {
    static RoundDisplaySpec spec;
    static WebDisplayGeometry geometry = buildWebGeometry(spec);
    display = new WebFastLedDisplay<RoundDisplaySpec>(spec, geometry, BRIGHTNESS, REFRESH_MS);
}

void loop() {
    if (display) {
        display->loop();
    }
}
