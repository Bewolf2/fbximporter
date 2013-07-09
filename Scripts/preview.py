#
# Confidential Information of Telekinesys Research Limited (t/a Havok). Not for disclosure or distribution without Havok's
# prior written consent. This software contains code, techniques and know-how which is confidential and proprietary to Havok.
# Product and Trade Secret source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2013 Telekinesys
# Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
#

import sys
import os

import projectanarchy.utilities
import projectanarchy.hct

def main(): 
	preview_tool = projectanarchy.hct.PreviewTool()

	file = ""
	if len(sys.argv) > 1:
		file = sys.argv[1]
	preview_tool.run(file)

	return

if __name__ == "__main__":
	main()
