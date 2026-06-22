#!/usr/bin/env bash

set -euo pipefail

SHADER_DIR="${1:-.}"

find "$SHADER_DIR" -type f \( \
    -name "*.vert" -o \
    -name "*.frag" -o \
    -name "*.geom" \
\) | while read -r shader
do
    dir="$(dirname "$shader")"
    filename="$(basename "$shader")"

    stem="${filename%.*}"
    ext="${filename##*.}"

    case "$ext" in
        vert) suffix="Vert" ;;
        frag) suffix="Frag" ;;
        geom) suffix="Geom" ;;
        *) continue ;;
    esac

    output="$dir/${stem}${suffix}.spv"

    echo "Compiling $shader -> $output"
    glslc "$shader" -o "$output"
done
