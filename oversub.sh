#!/bin/bash
#
# Copyright (C) 2014, 2015, 2016 Solra Bizna
#
# This file is part of overbuild.
#
# overbuild is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# overbuild is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with overbuild.  If not, see <http://www.gnu.org/licenses/>.
#

echo -ne "\\r\\e[0;0H\\e[33m*** Build pending... ***\\e[0m"
if ! make -O -k -j2 2>/tmp/oversub_result.txt >/dev/null; then
    SIZE=$(stty size)
    ROWS=$(echo $SIZE | cut -f 1 -d " ")
    COLS=$(echo $SIZE | cut -f 2 -d " ")
    clear
    echo -e "\\e[31m*** Build failed ***\\e[0m"
    head -n $(( $ROWS - 2 )) /tmp/oversub_result.txt | \
        awk "{print substr(\$0,0,$(($COLS)))}"
    echo -ne \\e[31m
else
    clear
    echo -e "\\e[32m*** Build succeeded ***"
    head -n $(( $ROWS - 2 )) /tmp/oversub_result.txt | \
        awk "{print substr(\$0,0,$(($COLS)))}"
fi
echo -n "---" `date` "---"
echo -ne \\e[0m
