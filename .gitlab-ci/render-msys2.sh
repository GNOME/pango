#!/bin/bash

set -e

PATH=_build/pango:$PATH gdb --command=./.gitlab-ci/gdb.txt --args _build/utils/pango-view --no-display --output _build/hello.png utils/HELLO.txt
