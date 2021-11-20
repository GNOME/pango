#!/bin/bash

set -e

PATH=_build/pango:$PATH _build/utils/pango-list --verbose --metrics > _build/fontlist.txt || true
