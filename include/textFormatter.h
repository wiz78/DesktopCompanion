//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_TEXT_FORMATTER_H
#define FRIENDLYBOT_TEXT_FORMATTER_H

#include <cstddef>
#include <string>
#include <vector>

namespace TextFormatter
{
    static constexpr size_t MAX_WRAPPED_CHARS = 40;
    static constexpr size_t MAX_CHARS_PER_LINE = 10;
    static constexpr size_t MAX_WRAPPED_LINES = 3;

    [[nodiscard]] std::vector<std::string> wrapText(
        const std::string& text,
        size_t maxCharsPerLine = MAX_CHARS_PER_LINE,
        size_t maxLines = MAX_WRAPPED_LINES
    );

    [[nodiscard]] bool canFitInLines(
        const std::string& text,
        size_t maxCharsPerLine = MAX_CHARS_PER_LINE,
        size_t maxLines = MAX_WRAPPED_LINES,
        size_t maxTotalChars = MAX_WRAPPED_CHARS
    );
}

#endif // FRIENDLYBOT_TEXT_FORMATTER_H
