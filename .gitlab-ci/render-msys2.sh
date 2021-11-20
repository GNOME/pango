#!/bin/bash

set -e

PATH=_build/pango:$PATH _build/utils/pango-view --no-display --output _build/hello.png utils/HELLO.txt || true
