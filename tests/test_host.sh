#!/usr/bin/env bash
# Requires only a C++11 compiler, never connects to a StopWatch.
set -euo pipefail
cd "$(dirname "$0")/.."
compiler="${CXX:-g++}"
"$compiler" -std=c++11 -Wall -Wextra -fsyntax-only -Itests/host_mock -Ifirmware/include firmware/src/main.cpp
for t in firmware/test/*.cpp; do
  binary="$(mktemp /tmp/starvoyager-XXXXXX)"
  "$compiler" -std=c++11 -O2 -Ifirmware/include "$t" -o "$binary"
  "$binary"
  rm -f "$binary"
done
for t in tests/button_test.cpp tests/button_gate_test.cpp tests/ring_test.cpp tests/test_release_tail.cpp tests/test_warp_fx_timing.cpp; do
  binary="$(mktemp /tmp/starvoyager-ui-XXXXXX)"
  "$compiler" -std=c++11 -Wall -Wextra -O2 -Ifirmware/include "$t" -o "$binary"
  "$binary"; rm -f "$binary"
done
printf 'PASS: R16F7 syntax, warp arrival camera/velocity, 2x thrust and 1.10s tail, hardware gates, physics\n' 
