#
# Confidential Information of Telekinesys Research Limited (t/a Havok). Not for
# disclosure or distribution without Havok's prior written consent. This
# software contains code, techniques and know-how which is confidential and
# proprietary to Havok. Product and Trade Secret source code contains trade
# secrets of Havok. Havok Software (C) Copyright 1999-2013 Telekinesys Research
# Limited t/a Havok. All Rights Reserved. Use of this software is subject to
# the terms of an end user license agreement.
#

import os

import utilities


def _getRegistryValue(root_key, key):
    """
    Try to get a registry value from the current user. Handles both
    Python 2.X and Python 3.X
    """

    value = ""

    try:
        from winreg import ConnectRegistry, OpenKey, QueryValueEx, HKEY_CURRENT_USER
    except ImportError:
        try:
            from _winreg import ConnectRegistry, OpenKey, QueryValueEx, HKEY_CURRENT_USER
        except ImportError:
            value = ""

    try:
        aReg = ConnectRegistry(None, HKEY_CURRENT_USER)
        aKey = OpenKey(aReg, root_key)
        value = QueryValueEx(aKey, key)[0]
    except WindowsError:
        value = ""

    return value


def _getHavokContentToolsPath():
    """
    Try to find the Havok Content Tools path through the registry, environment
    variable, and by default install location
    """

    havok_tools_root = _getRegistryValue("Software\\Havok\\hkFilters", "FilterPath")
    if not os.path.exists(havok_tools_root):
        havok_tools_root = _getRegistryValue("Software\\Havok\\hkFilters_x64", "FilterPath")

    if not os.path.exists(havok_tools_root):
        havok_tools_root = os.environ.get('HAVOK_TOOLS_ROOT')

    if not os.path.exists(str(havok_tools_root)):
        havok_tools_root = "C:\\Program Files\\Havok\\HavokContentTools"
    if not os.path.exists(havok_tools_root):
        havok_tools_root = "C:\\Program Files (x86)\\Havok\\HavokContentTools"

    if not os.path.exists(havok_tools_root):
        havok_tools_root = ""
        print("Could not find path to Havok Content Tools!")

    return havok_tools_root


class HCT():
    def __init__(self):
        self.havok_tools_root = _getHavokContentToolsPath()

        self.havok_filter_manager = os.path.join(
            self.havok_tools_root,
            'hctStandAloneFilterManager.exe')

        if not os.path.exists(self.havok_filter_manager):
            print("Failed to find filter manager!")

        return

    def run(self, filename, filter_set,
            asset_path, output_path,
            interactive=False, verbose=False):
        arguments = [
           "-p", asset_path + "\\",
           "-o", output_path + "\\",
           "-s", filter_set]

        if interactive:
            arguments.append("-i")

        arguments.append(filename)

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

    def run(self, filename):
        arguments = []
        arguments.append(filename)

        utilities.print_line()
        print("Starting Havok Filter Manager...")

        command = [self.havok_tool_standalone] + arguments
        utilities.print_line(True)
        print(' '.join(command))

        cd = os.path.dirname(filename)

        # It's important that the current directory is set to the output path
        # or it won't output to the expected directory
        utilities.run(command, False, cd)

        return