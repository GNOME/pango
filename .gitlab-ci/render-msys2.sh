#!/bin/bash

set -e

PATH=_build/pango:$PATH _build/utils/pango-view --line-spacing 0 --no-display --output _build/hello.png utils/HELLO.txt
