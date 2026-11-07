#!/usr/bin/env bash
# Usage: changelog-section.sh <tag> <notes-file>
# Finds the "## [<tag>] Title" section in CHANGELOG.md, writes the lines under it
# (up to the next "## " heading) to <notes-file>, and prints the title.
set -euo pipefail

tag="$1"
notes_file="$2"
changelog="${CHANGELOG_PATH:-CHANGELOG.md}"

# A Windows checkout may carry CRLF endings, which would leak "\r" into the title.
lines=$(tr -d '\r' < "$changelog")
heading=$(grep -m1 -F "## [${tag}]" <<< "$lines" || true)
if [ -z "$heading" ]; then
  echo "::error::${changelog} has no '## [${tag}]' section" >&2
  exit 1
fi

awk -v h="$heading" '$0 == h {found=1; next} found && /^## / {exit} found {print}' <<< "$lines" > "$notes_file"

title="${heading#"## [${tag}]"}"
title="${title#"${title%%[![:space:]]*}"}"
echo "${title:-$tag}"
