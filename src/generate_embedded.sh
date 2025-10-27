#!/bin/sh

# SPDX-FileCopyrightText: 2025 Ben Jarvis
#
# SPDX-License-Identifier: LGPL-2.1-only

printf "const char* $3 = " > $2

sed -e '/SPDX[-:]/d' -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/' $1 >> $2
printf ";" >> $2
