#!/usr/bin/python
#
# Utility script to generate .pc files for Pango
# for Visual Studio builds, to be used for
# building introspection files

# Author: Fan, Chun-wei
# Date: April 21, 2016

import os
import sys
import argparse

from replace import replace_multi
from pc_base import BasePCItems

def main(argv):
    base_pc = BasePCItems()

    pango_parser = argparse.ArgumentParser(description='Setup basic .pc file info')
    pango_parser.add_argument('--pangoft2',
                              action='store_const',
                              const=1,
                              help='Create .pc for PangoFT2')
    base_pc.setup(argv, pango_parser)
    base_pkg_replace_items = {'@PANGO_API_VERSION@': '1.0'}

    base_pkg_replace_items.update(base_pc.base_replace_items)

    # Generate pango.pc
    replace_multi(base_pc.top_srcdir + '/pango.pc.in',
                  base_pc.srcdir + '/pango.pc',
                  base_pkg_replace_items)

    # Generate pangowin32.pc
    replace_multi(base_pc.top_srcdir + '/pangowin32.pc.in',
                  base_pc.srcdir + '/pangowin32.pc',
                  base_pkg_replace_items)

    # Generate pangoft2.pc, if requested
    pango_args = pango_parser.parse_args()
    if getattr(pango_args, 'pangoft2', None) is 1:
        replace_multi(base_pc.top_srcdir + '/pangoft2.pc.in',
                      base_pc.srcdir + '/pangoft2.pc',
                      base_pkg_replace_items)

    # Generate pangocairo.pc
    pangocairo_replace_items = {'@PKGCONFIG_CAIRO_REQUIRES@': 'cairo'}
    pangocairo_replace_items.update(base_pkg_replace_items)
    replace_multi(base_pc.top_srcdir + '/pangocairo.pc.in',
                  base_pc.srcdir + '/pangocairo.pc',
                  pangocairo_replace_items)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
