"""
An interface to py2exe allowing us to package and distribute scripts
as windows executables.
"""

import os
import sys
import optparse
import shutil


def makeExe(script, outputExePath=None, keepIntermediates=False, verbose=False):
    """Given a path to a script bundle into an exe with a minimum of options and fuss.
    """
    importErrStr = "exe.py must be called standalone, importing can pollute python environment."
    assert __name__ == "__main__", importErrStr
    assert os.path.exists(script), "Error: Script %s not found." % script
    scriptDir = os.path.dirname(os.path.abspath(script))
    if outputExePath == None:
        outputExePath = os.path.splitext(script)[0] + ".exe"
    os.chdir(scriptDir)
    sys.argv = ['', 'py2exe']
    sys.path.append('.')
    from distutils.core import setup
    try:
        import py2exe
    except ImportError:
        print "ERROR: py2exe not found, please install it."
        return None
    savedStdout = sys.stdout
    if not verbose:
        sys.stdout = open(os.devnull, 'w')
    result = setup(console=[os.path.basename(script)],
                   options={'py2exe':{'bundle_files': '1'}},
                   zipfile=None)
    if not verbose:
        sys.stdout = savedStdout
    sys.path.pop()
    exeName = os.path.splitext(os.path.basename(script))[0] + '.exe'
    expectedExe = os.path.join(scriptDir, 'dist', exeName)
    if not os.path.exists(os.path.dirname(outputExePath)):
        os.makedirs(os.path.dirname(outputExePath))
    shutil.copyfile(expectedExe, outputExePath)
    if not keepIntermediates:
        shutil.rmtree(os.path.join(scriptDir, 'build'))
        shutil.rmtree(os.path.join(scriptDir, 'dist'))
    return outputExePath


COMMAND_LINE_OPTIONS = (
    (('-v', '--verbose',),
         {'action': 'store_true',
          'default': False,
          'help': r'Verbose mode.'}),
    (('-s', '--script',),
         {'action': 'store',
          'help': r'Path to script to be convered to an exe.'}),
    (('-o', '--output-exe'),
         {'action': 'store',
          'dest': 'outputExePath',
          'help': r"Output exe path. Default: scriptName.exe alongside the input script."}),
    (('-k', '--keep-intermediates',),
         {'action': 'store_true',
          'default': False,
          'dest': 'keepIntermediates',
          'help': r'Dont delete intermediate files.'}),
    )


HELP_STRING = """Compiles a python script to a windows executable (requires Py2exe - http://www.py2exe.org).
    Usage: exe.py -s <script>"""


if __name__=="__main__":
    if os.name=="posix":
        print "Script only supported on Windows."
        sys.exit(1)
    parser = optparse.OptionParser(usage=HELP_STRING)
    for options in COMMAND_LINE_OPTIONS:
        parser.add_option(*options[0], **options[1])
    options, _ = parser.parse_args()

    result = 0
    if not options.script:
        parser.print_help()
    else:
        outputExePath = os.path.abspath(options.outputExePath) if options.outputExePath else None
        scriptPath = os.path.abspath(options.script)
        exe = makeExe(scriptPath, outputExePath, options.keepIntermediates, options.verbose)
        if not os.path.exists(exe):
            result = 1
        else:
            print 'Created %s' % exe

    sys.exit(result)
