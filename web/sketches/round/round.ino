#include "RoundDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "display/WebFastLedDisplay.h"

using namespace PolarShader;

namespace {
    constexpr uint8_t BRIGHTNESS = 30;
    constexpr uint8_t REFRESH_MS = 30;

    WebFastLedDisplay<RoundDisplaySpec>& display() {
        static RoundDisplaySpec spec;
        static WebDisplayGeometry geometry = buildWebGeometry(spec);
        static WebFastLedDisplay<RoundDisplaySpec> instance(spec, geometry, BRIGHTNESS, REFRESH_MS);
        return instance;
    }
}

void setup() {
    (void) display();
}

void loop() {
    display().loop();
}
