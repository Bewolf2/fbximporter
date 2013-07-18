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
Package FBX Convertor for distribution
"""


import os
import sys
import logging
import inspect
import zipfile
import time
import subprocess

# We define this first as some global constants use it
def getDatestamp():
    """Get local datestamp"""
    return "%02i%02i%02i" % time.localtime()[0:3]

LOGGER = logging.getLogger('fbximporter.package')

SCRIPT_DIR = os.path.abspath(os.path.dirname(inspect.getfile(inspect.currentframe())))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))

TOOL_NAME = "FBXImporter"

CONVERT_EXECUTABLE = "FBXConverter.exe"
PREVIEW_EXECUTABLE = "PreviewTool.exe"

CONVERT_EXE_PATH = "../../Bin/Tools/%s" % (CONVERT_EXECUTABLE)
PREVIEW_EXE_PATH = "../../Bin/Tools/%s" % (PREVIEW_EXECUTABLE)

# These paths are relative to the project root
PY2EXE_PATHS = {"Scripts/convert.py": CONVERT_EXE_PATH,
                "Scripts/preview.py": PREVIEW_EXE_PATH}

IGNORE_LIST = ["__pycache__", ".pyc", ".spyderproject", ".suo", ".user"]

# (src, dest) pairs for packaging
# src is relative to package root, dest is the path in the zip to place the file at.
# if src is a directory it will be added recursively
PACKAGE_PATHS = {"README.md": ("Tools/%s/README.md" % TOOL_NAME),
                 "Scripts": ("Tools/%s/Scripts" % TOOL_NAME),
                 "Source": ("Tools/%s/Source" % TOOL_NAME),
                 CONVERT_EXE_PATH: ("Bin/Tools/%s" % CONVERT_EXECUTABLE),
                 PREVIEW_EXE_PATH: ("Bin/Tools/%s" % PREVIEW_EXECUTABLE),
                 ("Bin/%s.exe" % TOOL_NAME): ("Tools/%s/Bin/%s.exe" % (TOOL_NAME, TOOL_NAME)),
                 "Workspace": ("Tools/%s/Workspace" % TOOL_NAME)}

# Define the command line options. Need to put this after getDatestamp
# is defined.
PACKAGE = "Package/Output/ProjectAnarchy_FBXImporter_%s.zip" % getDatestamp()
COMMAND_LINE_OPTIONS = (
    (('-p', '--pkg-path'), {'action': 'store',
                            'dest': 'packagePath',
                            'default': os.path.join(PROJECT_ROOT, PACKAGE),
                            'help': 'Output package path and name. [default: %default]'}),
    (('-v', '--verbose'), {'action': 'store_true',
                           'dest': 'verbose',
                           'default': False,
                           'help': 'Enable verbose output. [default: %default]'}),
    )


def setupLogging():
    """Setup our logging support"""
    LOGGER.propagate = False
    LOGGER.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s (%(levelname)s) %(message)s')
    consoleHandler = logging.StreamHandler()
    consoleHandler.setLevel(logging.INFO)
    consoleHandler.setFormatter(formatter)
    LOGGER.addHandler(consoleHandler)


def makeExes(verbose):
    """Iterate over the src,dest pairs in PY2EXE_PATHS generating exes.
    Note: exe.py is deliberately called in a seperate process, importing
    the script will cause errors (due to how py2exe works)."""
    for (src, dest) in PY2EXE_PATHS.items():
        cmdTemplate = "python %(exeGenScript)s -s %(src)s -o %(dest)s"
        if verbose:
            cmdTemplate += " -v"
        cmd = cmdTemplate % {"exeGenScript": os.path.join(SCRIPT_DIR, "exe.py"),
                             "src": os.path.join(PROJECT_ROOT, src),
                             "dest": os.path.join(PROJECT_ROOT, dest)}
        LOGGER.info(cmd)
        result = subprocess.call(cmd)
        if result != 0:
            LOGGER.error("Exe generation failed for %s", src)


def makePackage(packagePath):
    """
    Iterate over the src,dest pairs in PACKAGE_PATHS adding to zip.
    Note: exe.py is deliberately called in a seperate process, importing
    the script will cause errors (due to how py2exe works).
    """

    packageZip = zipfile.ZipFile(packagePath, 'w')

    def addFileToZip(src, dest):
        """Adds 'src' file to zip file at 'dest'"""
        LOGGER.info("Add to zip: %s (%s)", src, dest)
        try:
            packageZip.write(src, dest)
        except:
            LOGGER.error("Failed to add [%s] to zip file." % src)

    def ignoreFile(filename):
        """
        Go through the global ignore list and return True if the filename
        includes text from any of them
        """
        for ignore in IGNORE_LIST:
            if ignore in filename:
                return True
        return False

    for (src, dest) in PACKAGE_PATHS.items():
        absSrc = os.path.abspath(os.path.join(PROJECT_ROOT, src))
        if os.path.isfile(absSrc):
            addFileToZip(os.path.join(PROJECT_ROOT, src), dest)
        elif os.path.isdir(absSrc):
            for (dirpath, __, filenames) in os.walk(absSrc):
                for filename in filenames:
                    if not ignoreFile(filename):
                        relDest = "%s/%s/%s" % (dest, dirpath.replace(absSrc, ''), filename)
                        relDest = os.path.normpath(relDest)
                        addFileToZip(os.path.join(dirpath, filename), relDest)
        else:
            LOGGER.error("Package path %s cannot be found." % src)

    packageZip.close()
    LOGGER.info("Generated: %s", packagePath)


def main(packagePath, verbose=False):
    """
    Generate exes, the package according to rules in PACKAGE_PATHS.
    Final package is placed at 'packagePath'
    """
    LOGGER.info("FBX Importer Packaging")

    try:
        makeExes(verbose)
        makePackage(packagePath)
    except:
        LOGGER.exception("Packaging failed")
        return 1

    return 0


if __name__ == '__main__':
    from optparse import OptionParser

    PARSER = OptionParser("Usage: package.py [-p packagepath] [-v]")
    for options in COMMAND_LINE_OPTIONS:
        PARSER.add_option(*options[0], **options[1])
    (OPTIONS, _) = PARSER.parse_args()

    setupLogging()

    RESULT = main(OPTIONS.packagePath, OPTIONS.verbose)
    LOGGER.info("Done")

    sys.exit(RESULT)