#
# Confidential Information of Telekinesys Research Limited (t/a Havok). Not for disclosure or distribution without Havok's
# prior written consent. This software contains code, techniques and know-how which is confidential and proprietary to Havok.
# Product and Trade Secret source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2013 Telekinesys
# Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
#

import sys
import os
import traceback
import types

import utilities

def _getHavokContentToolsPath():
	havok_tools_root = os.environ['HAVOK_TOOLS_ROOT']
	if not os.path.exists(havok_tools_root):
		havok_tools_root = "C:\\Program Files\\Havok\\HavokContentTools"
	if not os.path.exists(havok_tools_root):
		havok_tools_root = "C:\\Program Files (x86)\\Havok\\HavokContentTools"
	return havok_tools_root

class HCT():
	def __init__(self):
		self.havok_tools_root = _getHavokContentToolsPath()

		self.havok_filter_manager = os.path.join(self.havok_tools_root, 'hctStandAloneFilterManager.exe')
		if not os.path.exists(self.havok_filter_manager):
			print("Failed to find filter manager!")

		return

	def run(self, file, filter_set, asset_path, output_path, interactive=False, verbose=False):
		arguments = [
		   "-p", asset_path + "\\",
		   "-o", output_path + "\\",
		   "-s", filter_set ]
		if interactive:
			arguments.append("-i")

		arguments.append(file);

		command = [self.havok_filter_manager] + arguments

		if verbose:
			utilities.print_line()
			print("Starting Havok Filter Manager...")
			utilities.print_line(True)
			print(' '.join(command))

		# It's important that the current directory is set to the output path
		# or it won't output to the expected directory
		utilities.run(command, False, output_path)

		return

class PreviewTool():
	def __init__(self):
		self.havok_tools_root = _getHavokContentToolsPath()

		self.havok_tool_standalone = os.path.join(self.havok_tools_root, 'ToolStandAlone.exe')
		if not os.path.exists(self.havok_tool_standalone):
			print("Failed to find standalone tool!")

		return

	def run(self, file):
		arguments = []
		arguments.append(file);

		utilities.print_line()
		print("Starting Havok Filter Manager...")

		command = [self.havok_tool_standalone] + arguments
		utilities.print_line(True)
		print(' '.join(command))

		cd = os.path.dirname(file)

		# It's important that the current directory is set to the output path
		# or it won't output to the expected directory
		utilities.run(command, False, cd)

		return
