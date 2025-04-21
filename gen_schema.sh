#!/bin/sh

# SPDX-License-Identifier: GPL-2.0-only
#
# pisound-micro-mapper - a daemon to facilitate Pisound Micro control mappings.
#  Copyright (C) 2023-2025  Vilniaus Blokas UAB, https://blokas.io/
#
# This file is part of pisound-micro-mapper.
#
# pisound-micro-mapper is free software: you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation, version 2.0 of the License.
#
# pisound-micro-mapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along with pisound-micro-mapper.
# If not, see <https://www.gnu.org/licenses/>.

NAME=$(basename ${1%.*})
PIPE=$$.pipe
TYPE=${NAME}
echo "#include <stddef.h>" > $2
echo "static const char ${NAME}[] = {" >> $2
mkfifo ${PIPE}
if [ $? -ne 0 ]; then
	exit 1
fi
xxd -i < ${PIPE} >> $2 &
npx -y ts-json-schema-generator --minify --strict-tuples --path $1 --no-top-ref --type ${TYPE} > ${PIPE}
if [ $? -ne 0 ]; then
	rm ${PIPE}
	rm $2
	exit 1
fi
wait
rm ${PIPE}
echo "};" >> $2
echo "const char *get_${NAME}() { return ${NAME}; }" >> $2
echo "size_t get_${NAME}_length() { return sizeof(${NAME}); }" >> $2
