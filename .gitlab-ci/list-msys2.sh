#!/bin/bash

set -e

PATH=_build/pango:$PATH _build/utils/pango-list > _build/fontlist
