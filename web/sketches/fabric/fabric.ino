#include "FabricDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "display/WebFastLedDisplay.h"

using namespace PolarShader;

namespace {
    constexpr uint8_t BRIGHTNESS = 255;
    constexpr uint8_t REFRESH_MS = 30;

    WebFastLedDisplay<FabricDisplaySpec>& display() {
        static FabricDisplaySpec spec;
        static WebDisplayGeometry geometry = buildWebGeometry(spec);
        static WebFastLedDisplay<FabricDisplaySpec> instance(spec, geometry, BRIGHTNESS, REFRESH_MS);
        return instance;
    }
}

void setup() {
    (void) display();
}

void loop() {
    display().loop();
}
