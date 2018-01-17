#!/bin/bash
# Tool for checking for clang-format compliance
# Heavily inspired by: https://github.com/dolphin-emu/dolphin/blob/master/Tools/lint.sh

COMMIT=${1:---cached}
FILES=$(git diff --name-only --diff-filter=ACMRTUXB $COMMIT)

EXIT_CODE=0

for f in ${FILES}; do
	# Ignore anything that isn't a source file
	if ! echo "$f" | egrep -q "^src.*\.(cpp|h)$"; then
		continue
	fi
	# Check if clang-format and the original differ
	if ! clang-format "$f" | diff - "$f"; then
		echo "$f violates the coding style, fix listed above"
		EXIT_CODE=1
	fi
done

exit $EXIT_CODE
