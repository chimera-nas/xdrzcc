#!/bin/sh

echo "const char* $3 = \n" > $2

sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/' $1 >> $2
echo ";\n" >> $2
