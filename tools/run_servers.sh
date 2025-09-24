#!/usr/bin/env bash
set -euo pipefail
# Launch 3 servers with different delays to emulate heterogeneous backends.
# Usage: tools/run_servers.sh

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${ROOT}/server_sim" 9001 0.2 >/dev/null 2>&1 &
echo "server 9001 delay=0.2s pid=$!"
"${ROOT}/server_sim" 9002 0.6 >/dev/null 2>&1 &
echo "server 9002 delay=0.6s pid=$!"
"${ROOT}/server_sim" 9003 1.0 >/dev/null 2>&1 &
echo "server 9003 delay=1.0s pid=$!"
