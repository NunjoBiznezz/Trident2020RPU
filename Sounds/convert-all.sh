#!/bin/bash

set -e

for f in *.wav; do
  echo "Processing: $f"

  dir="$(dirname "$f")"
  base="$(basename "$f")"
  tmp="$dir/.tmp_${base%.*}_$$.wav"

  ffmpeg -y -i "$f" \
    -map_metadata -1 \
    -bitexact \
    -af "aresample=44100,pan=stereo|c0=c0|c1=c0" \
    -c:a pcm_s16le \
    "$tmp"

  # Verify before replace
  ffmpeg -i "$tmp" 2>&1 | grep -q "44100 Hz, stereo" || {
    echo "ERROR: format verification failed for $f"
    rm -f "$tmp"
    exit 1
  }

  mv -f "$tmp" "$f"
  echo "Replaced: $f"
done

