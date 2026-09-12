# SPDX-FileCopyrightText: 2026 Ben Jarvis
# SPDX-License-Identifier: LGPL-2.1-only

# Byte arrays avoid MSVC's string literal size limits. Normalize checkout EOLs.
file(READ "${INPUT}" content)
string(REPLACE "\r\n" "\n" content "${content}")
string(REGEX REPLACE "[^\n]*SPDX[-:][^\n]*\n" "" content "${content}")
string(HEX "${content}" bytes)
string(REGEX REPLACE "(................................)" "\\1\n" bytes "${bytes}")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${bytes}")
file(WRITE "${OUTPUT}" "const char ${SYMBOL}[] = {\n${bytes}0\n};\n")
