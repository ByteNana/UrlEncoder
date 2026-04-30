#!/usr/bin/env bash
BRANCH=$(git rev-parse --abbrev-ref HEAD)
[[ "$BRANCH" != rc-* ]] && exit 0
EXPECTED="${BRANCH#rc-}"
ACTUAL=$(grep -oE 'project\s*\([^)]*VERSION\s+[0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
if [ -z "$ACTUAL" ]; then
  echo "  ERROR: Could not extract VERSION from CMakeLists.txt"
  exit 1
fi
if [ "$ACTUAL" != "$EXPECTED" ]; then
  echo "  ERROR: Version mismatch — branch expects ${EXPECTED}, CMakeLists.txt has ${ACTUAL}"
  exit 1
fi
