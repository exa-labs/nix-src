#!/usr/bin/env bash

source common.sh

# shellcheck disable=SC1111
needLocalStore "“min-free” and “max-free” are daemon options"

TODO_NixOS

clearStore

garbage1=$(nix store add-path --name garbage1 ./nar-access.sh)
garbage2=$(nix store add-path --name garbage2 ./nar-access.sh)
garbage3=$(nix store add-path --name garbage3 ./nar-access.sh)

fake_free=$TEST_ROOT/fake-free
export _NIX_TEST_FREE_SPACE_FILE=$fake_free
echo 100 > "$fake_free"

sync2=$TEST_ROOT/sync2
mkfifo "$sync2"

# Process A: acquire the GC write lock, then block on the sync fifo
# without deleting anything (--print-dead only reports). SYNC_2 sits
# after the roots socket is created, so addTempRoot in other processes
# can still register roots and proceed while A holds the lock.
_NIX_TEST_GC_SYNC_2=$sync2 nix-store --gc --print-dead &
pidA=$!

# Wait until A holds the GC lock: a probe GC can only time out while
# the lock is taken.
for _ in {1..60}; do
    if ! timeout 2 nix-store --gc --print-dead >/dev/null 2>&1; then
        break
    fi
    sleep 0.5
done

# Process B: a build whose auto-GC queues behind A's lock.
expr=$(cat <<EOF
with import ${config_nix}; mkDerivation {
  name = "gc-skip";
  buildCommand = "mkdir \$out";
}
EOF
)

nix build --impure -v -o "$TEST_ROOT"/result-B -L --expr "$expr" \
    --min-free 1K --max-free 2K --min-free-check-interval 1 \
    2>"$TEST_ROOT/B.log" &
pidB=$!

# Wait until B's auto-GC is queued behind A's lock.
for _ in {1..60}; do
    if grep -q "waiting for the big garbage collector lock" "$TEST_ROOT/B.log" 2>/dev/null; then
        break
    fi
    sleep 0.5
done
grep -q "waiting for the big garbage collector lock" "$TEST_ROOT/B.log"

# Another collector reclaimed the space while B was queued: free space
# is back above min-free before B acquires the lock.
echo 5000 > "$fake_free.tmp" && mv "$fake_free.tmp" "$fake_free"

# Release A; B's queued pass should re-check free space and skip.
echo > "$sync2"

wait "$pidA"
wait "$pidB"

grep -q "skipping auto-GC" "$TEST_ROOT/B.log"

# The skipped pass must not have collected anything.
[[ -e $garbage1 ]]
[[ -e $garbage2 ]]
[[ -e $garbage3 ]]
