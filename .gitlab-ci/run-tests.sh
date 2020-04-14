#!/bin/bash

set +x
set +e

srcdir=$( pwd )
builddir=$1

meson test -C ${builddir} \
        --print-errorlogs \
        --suite=pango 

# Store the exit code for the CI run, but always
# generate the reports
exit_code=$?

cd ${builddir}

./utils/pango-list --verbose > fontlist.txt
./tests/test-font -p /pango/font/metrics --verbose
./utils/pango-view --no-display --output hello.png ${srcdir}/utils/HELLO.txt

$srcdir/.gitlab-ci/meson-junit-report.py \
        --project-name=pango \
        --job-id="${CI_JOB_NAME}" \
        --output=report.xml \
        meson-logs/testlog.json

exit $exit_code
