#!/bin/bash

set -e

PATH=_build/pango:$PATH _build/tests/test-font -p /pango/font/metrics --verbose
PATH=_build/pango:$PATH _build/utils/pango-list --verbose > _build/fontlist.txt
