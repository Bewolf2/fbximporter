#! /usr/bin/python
#
# Confidential Information of Telekinesys Research Limited (t/a Havok). Not for
# disclosure or distribution without Havok's prior written consent. This
# software contains code, techniques and know-how which is confidential and
# proprietary to Havok. Product and Trade Secret source code contains trade
# secrets of Havok. Havok Software (C) Copyright 1999-2013 Telekinesys Research
# Limited t/a Havok. All Rights Reserved. Use of this software is subject to
# the terms of an end user license agreement.
#

"""
convert.py - The main script of Havok FBX conversion suite.
"""

import sys

from optparse import OptionParser

import projectanarchy.fbx
import projectanarchy.utilities

COMMAND_LINE_OPTIONS = (
    (('-f', '--filename',),
     {'action': 'store',
      'dest': 'filename',
      'help': "FBX file to convert/import"}),
    (('-i', '--interactive',),
     {'action': 'store_true',
      'dest': 'interactive',
      'default': False,
      'help': "Use interactive mode which will bring up the standalone filter manager"}),
    (('-q', '--quiet',),
     {'action': 'store_false',
      'dest': 'verbose',
      'default': True,
      'help': "Don't print out status updates"}),
    (('-m', '--model'),
     {'action': 'store_true',
      'dest': 'outputVisionModel',
      'default': False,
      'help': 'Output a Vision Model file (does NOT include animations!)'}),
    (('-s', '--static-mesh'),
     {'action': 'store_true',
      'dest': 'outputStaticMesh',
      'default': False,
      'help': 'Forces it to output a static mesh and not a model with animation'}))


def main():
    """
    Exports/converts the FBX file passed in as an argument.
    """

    parser = OptionParser('')
    for options in COMMAND_LINE_OPTIONS:
        parser.add_option(*options[0], **options[1])
    (options, _) = parser.parse_args()

    # Print the header
    if options.verbose:
        projectanarchy.utilities.print_line()
        print("Havok FBX Importer")
        projectanarchy.utilities.print_line()

    fbx_file = options.filename
    if not fbx_file and len(sys.argv) > 1:
        fbx_file = sys.argv[len(sys.argv) - 1]

    success = False

    if not fbx_file:
        print("Missing FBX input filename!")
    else:
        success = projectanarchy.fbx.convert(
            fbx_file=fbx_file,
            static_mesh=options.outputStaticMesh,
            vision_model=options.outputVisionModel,
            interactive=options.interactive,
            verbose=options.verbose)

    return success

if __name__ == "__main__":
    SUCCESS = main()
    sys.exit(0 if SUCCESS else 1)