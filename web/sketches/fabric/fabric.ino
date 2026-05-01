#include "FabricDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "display/WebFastLedDisplay.h"

using namespace PolarShader;

namespace {
    constexpr uint8_t BRIGHTNESS = 255;
    constexpr uint8_t REFRESH_MS = 30;

    WebFastLedDisplay<FabricDisplaySpec> *display = nullptr;
}

void setup() {
    static FabricDisplaySpec spec;
    static WebDisplayGeometry geometry = buildWebGeometry(spec);
    display = new WebFastLedDisplay<FabricDisplaySpec>(spec, geometry, BRIGHTNESS, REFRESH_MS);
}

void loop() {
    if (display) {
        display->loop();
    }
}
