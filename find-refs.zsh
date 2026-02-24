#!/usr/bin/env zsh
# find-refs.zsh — find references to/from a feature/module
# Disclaimer: This script was AI generated so all the effort goes to the actual
# work, which is the emacs-zero editor.

if [[ -z "$1" ]]; then
    echo "Usage: find-refs.zsh <feature-name>"
    exit 1
fi

feat="$1"

echo "=== Elisp require/load references ==="
git grep -n "require.*['\"]$feat" -- '*.el'

echo "\n=== Autoload cookies ==="
git grep -n "autoload.*$feat" -- '*.el'

echo "\n=== C source references ==="
git grep -n "$feat" -- '*.c' '*.h'

echo "\n=== Makefile/build references ==="
git grep -n "$feat" -- 'Makefile*' '*.mk' 'configure.ac' 'configure'

echo "\n=== declare-function references ==="
git grep -n "declare-function.*['\"]$feat" -- '*.el'

echo "\n=== config.h references ==="
git grep -n "$feat" -- 'config.h.in' 'config.h'

echo "\n=== Dependencies (what $feat requires) ==="
git grep -n "^#include.*$feat\|require.*['\"]$feat" -- '*.c' '*.h' '*.el'
