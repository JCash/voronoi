#!/usr/bin/env bash
set -euo pipefail

REPOSITORY="${GITHUB_REPOSITORY:-JCash/voronoi}"
OUTPUT_FILE="${1:-CHANGELOG.md}"
TEMP_FILE="$(mktemp)"
trap 'rm -f "${TEMP_FILE}"' EXIT

RELEASES="$(gh api \
  --paginate \
  "repos/${REPOSITORY}/releases?per_page=100" \
  --jq '
    .[]
    | select(.draft == false and .published_at != null)
    | .tag_name as $tag
    | (.body // ""
       | gsub("\\r"; "")
       | gsub(" "; " ")
       | gsub("^\\s+|\\s+$"; "")) as $body
    | ($body | split("\n")) as $lines
    | (if $lines[0] == ("## " + ($tag | ltrimstr("v")))
       then ($lines[1:] | join("\n") | gsub("^\\s+|\\s+$"; ""))
       else $body
       end) as $notes
    | ($notes
       | split("\n")
       | map(
           if startswith("## ") then "#" + .
           elif startswith("* ") then "- " + .[2:]
           else .
           end
         )
       | join("\n")) as $notes
    | "## [" + ($tag | ltrimstr("v")) + "](" + .html_url + ") - "
      + (.published_at[0:10]) + "\n\n"
      + (if ($notes | length) > 0 then $notes else (.name // $tag) end)
      + "\n"
  '
)"

if [[ -z "${RELEASES}" ]]; then
  echo "No published releases found for ${REPOSITORY}" >&2
  exit 1
fi

{
  echo "# Changelog"
  echo
  echo "All notable changes are collected from the project's GitHub releases."
  echo
  printf '%s\n' "${RELEASES}"
} > "${TEMP_FILE}"

mv "${TEMP_FILE}" "${OUTPUT_FILE}"
trap - EXIT
