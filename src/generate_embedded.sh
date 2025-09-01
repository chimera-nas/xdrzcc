#!/bin/sh

# SPDX-FileCopyrightText: 2025 Ben Jarvis
#
# SPDX-License-Identifier: LGPL-2.1-only

echo "const char* $3 = \n" > $2

sed -e '/SPDX[-:]/d' -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/' $1 >> $2
echo ";\n" >> $2
