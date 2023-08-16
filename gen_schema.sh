#!/bin/sh
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
npx ts-json-schema-generator --minify --strict-tuples --path $1 --no-top-ref --type ${TYPE} > ${PIPE}
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
