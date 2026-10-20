#!/bin/sh
# Usage: tests/run.sh ./netdiag
set -u
bin=${1:-./netdiag}
fail=0
check() {  # check <name> <input> <expected substring>
    out=$(printf '%s\n' "$2" | "$bin")
    case $out in
        *"$3"*) echo "PASS $1" ;;
        *) echo "FAIL $1: expected '$3' in: $out"; fail=1 ;;
    esac
}
check help     "HELP"                    "OK PING"
check lookup   "LOOKUP localhost"        "OK localhost"
check bad-host "LOOKUP a;b"              "ERR invalid host"
check bad-port "CHECK localhost 99999"   "ERR usage"
check closed   "CHECK 127.0.0.1 1"       "closed"
check unknown  "FROB"                    "ERR unknown command"
check quit     "QUIT"                    "OK bye"
exit $fail
