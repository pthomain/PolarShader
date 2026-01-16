//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

#ifndef LED_SEGMENTS_PIPELINE_UTILS_PIPELINELOGGING_H
#define LED_SEGMENTS_PIPELINE_UTILS_PIPELINELOGGING_H

namespace LEDSegments {
    /**
     * @brief Lightweight logging helpers for pipeline construction/runtime.
     *
     * Implemented in a .cpp file to avoid dragging Serial/Arduino into headers.
     * On embedded targets this writes to Serial; on host targets it can fall
     * back to stdio.
     */
    void pipelineLog(const char *message);
    void pipelineLog(const char *message, const char *detail);
}

#endif //LED_SEGMENTS_PIPELINE_UTILS_PIPELINELOGGING_H
