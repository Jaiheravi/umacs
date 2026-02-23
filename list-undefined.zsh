#!/usr/bin/env zsh
# list-undefined.zsh — compile and extract undefined symbols
# Disclaimer: This script was AI generated so all the effort goes to the actual
# work, which is the µmacs editor.

if [[ -z "$1" ]]; then
    echo "Usage: list-undefined.zsh <file.c | make target>"
    echo "  list-undefined.zsh src/emacs.c"
    echo "  list-undefined.zsh make"
    exit 1
fi

if [[ "$1" == "make" ]]; then
    output=$(make 2>&1)
else
    # Adjust include paths as you discover them
    output=$(cc -c -I src -I lib "$1" 2>&1)
fi

echo "=== Undefined symbols ==="
echo "$output" | grep -oE "Undefined symbol[s]?: .*|undefined reference to \`[^']+\`" | sort -u

echo "\n=== Missing headers ==="
echo "$output" | grep -oE "'[^']+\.h' file not found|No such file or directory.*\.h" | sort -u

echo "\n=== Missing macros/defines ==="
echo "$output" | grep -oE "use of undeclared identifier '[^']+'" | sort -u

echo "\n=== Raw errors (last 30) ==="
echo "$output" | grep -i "error:" | tail -30
