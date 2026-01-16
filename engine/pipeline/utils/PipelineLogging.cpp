//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

#include "PipelineLogging.h"
#include <Arduino.h>
#include <cstdio>

namespace LEDSegments {

    void pipelineLog(const char *message) {
        if (!message) return;
#ifdef Serial
        Serial.println(message);
#else
        std::printf("%s\n", message);
#endif
    }

    void pipelineLog(const char *message, const char *detail) {
        pipelineLog(message);
        if (detail) pipelineLog(detail);
    }
}

