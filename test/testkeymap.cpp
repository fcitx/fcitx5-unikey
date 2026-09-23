/*
 * SPDX-FileCopyrightText: 2026-2026 ndlong78 <ndlong78@icloud.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "inputproc.h"
#include "usrkeymap.h"
#include <cstdio>
#include <fcitx-utils/log.h>
#include <string_view>
#include <vector>

namespace {

std::vector<UkKeyMapping> loadFromString(std::string_view content) {
    FILE *f = std::tmpfile();
    FCITX_ASSERT(f);
    FCITX_ASSERT(std::fwrite(content.data(), 1, content.size(), f) ==
                 content.size());
    std::fflush(f);
    std::rewind(f);
    auto result = UkLoadKeyOrderMap(fileno(f));
    std::fclose(f);
    return result;
}

void testDuplicateKeyIsSkipped() {
    // "S" is the same key as "s" for tone events, it should be skipped, but
    // the following lines must still be loaded.
    auto map = loadFromString("s = Tone1\n"
                              "S = Tone2\n"
                              "f = Tone2\n"
                              "; comment\n"
                              "d = D-Mark\n");
    FCITX_ASSERT(map.size() == 3) << map.size();
    FCITX_ASSERT(map[0].key == 'S' && map[0].action == vneTone1);
    FCITX_ASSERT(map[1].key == 'F' && map[1].action == vneTone2);
    FCITX_ASSERT(map[2].key == 'D' && map[2].action == vneDd);
}

void testInvalidLines() {
    auto map = loadFromString("ab = Tone1\n"
                              "x = NotACommand\n"
                              "z =\n"
                              "j = Tone5\n");
    FCITX_ASSERT(map.size() == 1) << map.size();
    FCITX_ASSERT(map[0].key == 'J' && map[0].action == vneTone5);
}

} // namespace

int main() {
    testDuplicateKeyIsSkipped();
    testInvalidLines();
    return 0;
}
