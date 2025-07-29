#!/bin/sh
#
# build.sh - Copyright (c) 2024-2025 - Olivier Poncet
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# ----------------------------------------------------------------------------
# global variables
# ----------------------------------------------------------------------------

arg_scripts="${0:-not-set}"
arg_target="${1:-not-set}"
arg_jobs="$(nproc)"

# ----------------------------------------------------------------------------
# consume the first argument if needed
# ----------------------------------------------------------------------------

if [ "${#}" -gt '0' ]
then
    shift
fi

# ----------------------------------------------------------------------------
# check the rest of command line
# ----------------------------------------------------------------------------

if [ "${#}" -gt '0' ]
then
    echo "Error: too many arguments"
    exit 1
fi

# ----------------------------------------------------------------------------
# special clean target
# ----------------------------------------------------------------------------

if [ "target:${arg_target}" = 'target:clean' ]
then
    targets="linux wasm"
    for target in ${targets}
    do
        make -f "Makefile.${target}" clean                           || exit 1
    done
    exit 0
fi

# ----------------------------------------------------------------------------
# linux and wasm target
# ----------------------------------------------------------------------------

if [ "target:${arg_target}" = 'target:linux' ] \
|| [ "target:${arg_target}" = 'target:wasm'  ]
then
    make -f "Makefile.${arg_target}" clean                           || exit 1
    make -f "Makefile.${arg_target}" -j ${arg_jobs} all              || exit 1
    exit 0
fi

# ----------------------------------------------------------------------------
# help target
# ----------------------------------------------------------------------------

if [ "target:${arg_target}" = 'target:help'    ] \
|| [ "target:${arg_target}" = 'target:not-set' ]
then
    echo "Usage: build.sh [TARGET]"                                  || exit 1
    echo ""                                                          || exit 1
    echo "Supported targets:"                                        || exit 1
    echo "      - clean           full cleanup"                      || exit 1
    echo "      - linux           clean and build the linux target"  || exit 1
    echo "      - wasm            clean and build the wasm target"   || exit 1
    echo "      - help            display this help"                 || exit 1
    echo ""                                                          || exit 1
    exit 0
else
    echo "Error: invalid target '${arg_target}'"                     || exit 1
    exit 1
fi

# ----------------------------------------------------------------------------
# End-Of-File
# ----------------------------------------------------------------------------
