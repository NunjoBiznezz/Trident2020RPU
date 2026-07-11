#!/bin/bash

set -e

# Check arguments
if [ $# -ne 2 ]; then
  echo "Usage: $0 <source_file> <destination_file>"
  echo "  source_file: Input audio file (any format supported by ffmpeg)"
  echo "  destination_file: Output WAV file"
  exit 1
fi

source_file="$1"
dest_file="$2"

# Check if source file exists
if [ ! -f "$source_file" ]; then
  echo "ERROR: Source file does not exist: $source_file"
  exit 1
fi

# Check if destination directory exists, create if needed
dest_dir="$(dirname "$dest_file")"
if [ ! -d "$dest_dir" ]; then
  echo "Creating destination directory: $dest_dir"
  mkdir -p "$dest_dir"
fi

echo "Converting: $source_file -> $dest_file"

# Convert to WAV with same parameters as batch script
ffmpeg -y -i "$source_file" \
  -map_metadata -1 \
  -bitexact \
  -af "aresample=44100,pan=stereo|c0=c0|c1=c0" \
  -c:a pcm_s16le \
  "$dest_file"

# Verify output format
ffmpeg -i "$dest_file" 2>&1 | grep -q "44100 Hz, stereo" || {
  echo "ERROR: Format verification failed for $dest_file"
  rm -f "$dest_file"
  exit 1
}

echo "Conversion complete: $dest_file"
