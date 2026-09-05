#!/usr/bin/env bash
#
# Generate doc/refman.pdf from the sources.
#
# Doxygen emits LaTeX; tectonic turns it into a PDF. Two LaTeX passes are needed
# because refman.idx -- the raw index data -- is written by the first pass, and
# make_index.py turns it into the refman.ind that the second pass embeds. That
# step normally belongs to makeindex, which tectonic does not ship; without it
# the PDF still builds and silently has no index.
#
# Usage: tools/make_doc.sh [-v]
#   -v   show the full doxygen and tectonic output instead of only failures
set -euo pipefail

verbose=0
[ "${1:-}" = "-v" ] && verbose=1

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
latex_dir="$repo_root/test/build/doxygen/latex"
output="$repo_root/doc/refman.pdf"
log="$(mktemp -t make_doc)"
trap 'rm -f "$log"' EXIT

for tool in doxygen tectonic python3; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found (brew install $tool)" >&2; exit 1; }
done

# Run a step quietly; on failure (or with -v) show what it printed.
run() {
  local label="$1"; shift
  echo "==> $label"
  if ! "$@" >"$log" 2>&1; then
    sed 's/^/    /' "$log" >&2
    echo "error: $label failed" >&2
    exit 1
  fi
  [ "$verbose" = 1 ] && sed 's/^/    /' "$log" || true
}

cd "$repo_root"
run "doxygen" doxygen Doxyfile

# Obsolete-tag notices are expected -- the config predates doxygen 1.18.
# Anything else is a real documentation defect and should fail the build.
if grep -viE 'obsolete|To avoid this warning' "$log" | grep -iE 'warning|error' >/dev/null; then
  grep -viE 'obsolete|To avoid this warning' "$log" | grep -iE 'warning|error' | sed 's/^/    /' >&2
  echo "error: doxygen reported documentation warnings" >&2
  exit 1
fi

[ -f "$latex_dir/refman.tex" ] || { echo "error: $latex_dir/refman.tex was not generated" >&2; exit 1; }
cd "$latex_dir"

run "latex pass 1 (writes refman.idx)" tectonic -X compile refman.tex --outdir .
run "index" python3 "$repo_root/tools/make_index.py"
sed 's/^/    /' "$log"
run "latex pass 2 (embeds the index)" tectonic -X compile refman.tex --outdir .

cp refman.pdf "$output"
echo "==> wrote $output"
command -v pdfinfo >/dev/null 2>&1 && pdfinfo "$output" | grep -E '^(Title|Pages)' | sed 's/^/    /' || true
