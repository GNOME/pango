#!/bin/bash

set -e

PATH=_build/pango2:$PATH _build/utils/pango-list --verbose --metrics > _build/fontlist.txt
