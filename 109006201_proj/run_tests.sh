#!/usr/bin/env bash
# Build, run both versions on every test case, and report:
#   * basic:    output must match sorted_ans/ (regression gate)
#   * advanced: must be accepted by bin/verifier AND out-earn basic
#
# The bundled verifier is an x86_64 macOS binary; on Apple Silicon it runs under
# Rosetta (arch -x86_64). On other platforms, set VERIFIER to your own binary or
# the script skips verifier checks and only diffs basic vs sorted_ans.
set -u
cd "$(dirname "$0")"

CASES=${CASES:-"case1 case2 case3"}
PASS=1

echo "==> building"
make -s || { echo "build failed"; exit 1; }

run_verifier() {  # $1 = case  -> echoes revenue, returns verifier exit
  if command -v arch >/dev/null 2>&1 && [ -x bin/verifier ]; then
    arch -x86_64 ./bin/verifier "$1" 2>&1
  else
    echo "SKIP"
  fi
}

revenue_col() { awk '{s+=$6} END{print s+0}' "$1"; }
accepted()    { awk '$2==1{n++} END{print n+0}' "$1"; }

printf "\n%-7s | %-18s | %-28s\n" "case" "basic" "advanced"
printf -- "--------+--------------------+------------------------------\n"

for c in $CASES; do
  mkdir -p "result/$c"

  # ---- basic: regression against sorted_ans ----
  ./bin/main "$c" basic >/dev/null 2>&1
  b_rev=$(revenue_col "result/$c/user_result.txt")
  b_acc=$(accepted "result/$c/user_result.txt")
  b_ok="MATCH"
  for f in user_result transfer_log station_status; do
    if [ -f "sorted_ans/$c/$f.txt" ]; then
      if ! diff -q <(sort "result/$c/$f.txt") <(sort "sorted_ans/$c/$f.txt") >/dev/null; then
        b_ok="DIFF($f)"; PASS=0
      fi
    fi
  done

  # ---- advanced: verifier + must beat basic ----
  ./bin/main "$c" advanced >/dev/null 2>&1
  a_rev=$(revenue_col "result/$c/user_result.txt")
  a_acc=$(accepted "result/$c/user_result.txt")
  vout=$(run_verifier "$c")
  if [ "$vout" = "SKIP" ]; then
    a_ok="verifier-skipped"
  elif echo "$vout" | grep -qiE "error|wrong|fail|not exist|invalid"; then
    a_ok="VERIFIER-ERROR"; PASS=0
  elif [ "$a_rev" -le "$b_rev" ]; then
    a_ok="NOT>basic"; PASS=0
  else
    a_ok="OK"
  fi

  printf "%-7s | %6s acc  %-8s | %6s acc  %-9s %-14s\n" \
         "$c" "$b_acc" "$b_ok" "$a_acc" "$a_rev" "$a_ok"
done

echo
[ "$PASS" -eq 1 ] && echo "ALL CHECKS PASSED" || { echo "SOME CHECKS FAILED"; exit 1; }
