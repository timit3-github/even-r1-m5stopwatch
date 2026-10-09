#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d)
# Keep test binaries outside the deliverable project.
cc -std=c11 -Wall -Wextra -Werror -I include src/r1_wire.c tests/test_wire.c -o "$test_dir/wire"
cc -std=c11 -Wall -Wextra -Werror -I include src/r1_legacy.c tests/test_legacy.c -o "$test_dir/legacy"
cc -std=c11 -Wall -Wextra -Werror -I include src/r1_inputs.c tests/test_inputs.c -o "$test_dir/inputs"
"$test_dir/wire"
"$test_dir/legacy"
"$test_dir/inputs"
