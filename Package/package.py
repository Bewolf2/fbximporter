"""
Package FBX Convertor for distribution
TODO
 - Bin\Tools\FBXImporter.exe
 - Tools\FBXImporter\README.md
 - Tools\FBXImporter\Scripts\FBXConverter.exe
 - Tools\FBXImporter\Scripts\configurations\*

"""


import os
import sys
import logging
import inspect
import zipfile
import time
import subprocess

LOGGER = logging.getLogger('fbximporter.package')

SCRIPT_DIR = os.path.abspath(os.path.dirname(inspect.getfile(inspect.currentframe())))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))


# These paths are relative to the project root
PY2EXE_PATHS = {"Scripts/convert.py": "Scripts/FBXConverter.exe",
                "Scripts/preview.py": "Scripts/FBXPreview.exe",}

# (src, dest) pairs for packaging
# src is relative to package root, dest is the path in the zip to place the file at.
# if src is a directory it will be added recursively
PACKAGE_PATHS = {"README.md": "Tools/FBXImporter/README.md",
                 "Scripts/configurations": "Tools/FBXImporter/Scripts/configurations",
                 "Scripts/FBXConverter.exe": "Bin/Tools/FBXConverter.exe",
                 "Scripts/FBXPreview.exe": "Bin/Tools/FBXPreview.exe",
                 "Workspace": "Workspace"}


def getDatestamp():
    """Get local datestamp"""
    return "%02i%02i%02i" % time.localtime()[0:3]


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
    """Iterate over the src,dest pairs in PACKAGE_PATHS adding to zip.
    Note: exe.py is deliberately called in a seperate process, importing
    the script will cause errors (due to how py2exe works)."""

    packageZip = zipfile.ZipFile(packagePath, 'w')

    def addFileToZip(src, dest):
        LOGGER.info("Add to zip: %s (%s)", src, dest)
        packageZip.write(src, dest)

    for (src, dest) in PACKAGE_PATHS.items():
        absSrc = os.path.join(PROJECT_ROOT, src)
        if os.path.isfile(absSrc):
            addFileToZip(os.path.join(PROJECT_ROOT, src), dest)
        elif os.path.isdir(absSrc):
            for (dirpath, dirnames, filenames) in os.walk(absSrc):
                for f in filenames:
                    relDest = os.path.normpath(os.path.join(dest, dirpath.replace(absSrc, ''), f))
                    addFileToZip(os.path.join(dirpath, f), relDest)
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


COMMAND_LINE_OPTIONS = (
    (('-p', '--pkg-path'), {'action': 'store',
                            'dest': 'packagePath',
                            'default': os.path.join(PROJECT_ROOT, 'ProjectAnarchy_FbxImporter_%s.zip' % getDatestamp()),
                            'help': 'Output package path and name. [default: %default]'}),
    (('-v', '--verbose'), {'action': 'store_true',
                           'dest': 'verbose',
                           'default': False,
                           'help': 'Enable verbose output. [default: %default]'}),
    )

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser("Usage: package.py [-p packagepath] [-v]")
    for options in COMMAND_LINE_OPTIONS:
        parser.add_option(*options[0], **options[1])
    (options, argv) = parser.parse_args()
    setupLogging()
    result = main(options.packagePath, options.verbose)
    LOGGER.info("Done")
    sys.exit(result)
