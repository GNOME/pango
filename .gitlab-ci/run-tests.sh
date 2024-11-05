#!/bin/bash

set +x
set +e

srcdir=$( pwd )
builddir=$1

# Ignore memory leaks lower in dependencies
export LSAN_OPTIONS=suppressions=$srcdir/lsan.supp:print_suppressions=0:symbolize=1
export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer

meson test -C ${builddir} \
        --timeout-multiplier "${MESON_TEST_TIMEOUT_MULTIPLIER}" \
        --print-errorlogs \
        --suite=pango

# Store the exit code for the CI run, but always
# generate the reports
exit_code=$?

cd ${builddir}

./utils/pango-list --verbose > fontlist.txt
./tests/test-font -p /pango/font/metrics --verbose
./utils/pango-view --no-display --output hello.png ${srcdir}/utils/HELLO.txt

exit $exit_code
