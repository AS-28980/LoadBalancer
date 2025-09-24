#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: > "${ROOT}/times.txt"
# start N clients (each does 20 requests with ~5-6s jitter)
N="${1:-5}"
for i in $(seq 1 "$N"); do
  "${ROOT}/client_sim" 127.0.0.1 8080 &
  echo "client $i started pid=$!"
done
wait
echo "All clients finished. See times.txt"
