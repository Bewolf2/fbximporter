#
# Confidential Information of Telekinesys Research Limited (t/a Havok). Not for
# disclosure or distribution without Havok's prior written consent. This
# software contains code, techniques and know-how which is confidential and
# proprietary to Havok. Product and Trade Secret source code contains trade
# secrets of Havok. Havok Software (C) Copyright 1999-2013 Telekinesys Research
# Limited t/a Havok. All Rights Reserved. Use of this software is subject to
# the terms of an end user license agreement.
#

import subprocess
import sys
import os

import msvcrt as m

def line(thin=False):    
    if thin:
        output = '-' * 79
    else:
        output = '=' * 79
    return output

def print_line(thin = False):
    print(line(thin))
    return

def clear():
    os.system('cls')

def wait():
    print("Press any key to continue...")
    sys.stdout.flush()
    m.getch()

def is_drag_drop_start():
    """
    Returns true if the script was started by dragging a file onto it
    """

    cwd = os.getcwd()
    drag_drop = False

    if 'WINDIR' in os.environ:
        winpath = os.environ['WINDIR']
        drag_drop = (winpath == cwd)

    return drag_drop

def parse_text(source, label, parse_index=0):
    index = source.find(label, parse_index)
    if index == -1:
        return False

    end = source.find("\n", index)
    return source[index + len(label) + 1 : end]

def run(arguments, verbose=False, current_directory=""):
    if current_directory == "":
        current_directory = os.path.dirname(arguments[0])

    disable_capture = False
    
    if disable_capture:
        child = subprocess.Popen(arguments, cwd=current_directory)
    else:
        child = subprocess.Popen(arguments, shell=True, stdout=subprocess.PIPE, cwd=current_directory)

    output = ""

    if disable_capture:
        child.communicate()
    else:
        while True:
            try:
                output_character = child.stdout.read(1)

                # handle the Python 3.0 case where it's returned as a series of bytes
                if isinstance(output_character, bytes):
                    output_character = output_character.decode("utf-8")

                if output_character == '' and child.poll() != None:
                    break

                if verbose:
                    sys.stdout.write(output_character)
                    sys.stdout.flush()

                output += output_character;
            except:
                # just catch everything and break out of the loop
                break

    return output