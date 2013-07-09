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

import projectanarchy.fbx

def main():
    """
    Exports/converts the FBX file passed in as an argument.
    """

    # TODO
    #   * Add parameter for specifying output filename
    #   * Add parameter for outputting tag or binary file

    projectanarchy.fbx.export_all()

    return

if __name__ == "__main__":
    main()