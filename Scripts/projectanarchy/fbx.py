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
Handles calling the FBX Importer executable and then processing the result
of that with Havok Content Tools filter manager. Given an FBX file it will:
    - Convert animated FBX files to rig and animation files
    - Convert static FBX files to Vision static mesh files (.vmesh)
"""

import sys
import os
import traceback

import utilities
from hct import HCT

class HavokScene():
    def __init__(self):
        self.initialized = False

def export():
    # Convert FBX to HKT first
    current_directory = os.path.dirname(os.path.realpath(__file__))
    
    root = current_directory
    while os.path.dirname(root) != root:
        fbx_importer = os.path.join(root, "Bin\\Tools\\FBXImporter.exe")
        if os.path.isfile(fbx_importer):
            break
        root = os.path.abspath(os.path.join(root, os.pardir) + '\\')

    fbx_importer = os.path.abspath(fbx_importer)
    if not os.path.exists(fbx_importer):
        print(fbx_importer)
        print("Failed to find FBX importer!")
        return

    input_file = os.path.abspath(str(sys.argv[1]))
    intput_directory = os.path.dirname(input_file)

    print("Converting FBX to Havok Scene Format...")
    fbx_importer_output = utilities.run([fbx_importer, input_file], True)

    label_animation_stacks = "Animation stacks:"
    label_tag_file = "Saved tag file:"
    label_scene_length = "Scene length:"

    parse_index = fbx_importer_output.find(label_tag_file)
    if parse_index == -1:
        print("Conversion to FBX failed!")
        utilities.print_line()
        print(fbx_importer_output)
        return

    havok_content_tools = HCT()

    animationStacks = int(utilities.parse_text(
        fbx_importer_output,
        label_animation_stacks))
    havokScenes = []
    isRootNode = True

    utilities.print_line()
    print("Generating Vision / Animation Studio files")
    utilities.print_line()

    while parse_index >= 0:
        scene_file = utilities.parse_text(fbx_importer_output, label_tag_file, parse_index)
        scene_file = os.path.join(intput_directory, scene_file)

        print("Source Havok tag file: %s" % os.path.basename(scene_file))

        (input_file_path, _) = os.path.splitext(scene_file)
        input_directory = os.path.dirname(scene_file)

        target_filename = os.path.basename(input_file_path)
        if isRootNode:
            rootName = target_filename
        else:
            animName = target_filename[len(rootName) + 1:]

        if isRootNode and animationStacks > 0:
            configuration_file = "..\configurations\AnimationRig.hko"
            target_filename = "%s__out_rig.hkx" % (rootName)
        elif animationStacks > 0:
            configuration_file = "..\configurations\Animation.hko"
            target_filename = "%s__out_anim_%s.hkx" % (rootName, animName)
        else:
            configuration_file = "..\configurations\VisionStaticMesh.hko"
            target_filename = "%s.vmesh" % (rootName)

        configuration_file = os.path.abspath(os.path.join(
            current_directory,
            configuration_file))
        output_configuration_file = os.path.abspath(input_file_path + ".hko")

        with open(output_configuration_file, 'wt') as out:
            for line in open(configuration_file):
                out.write(line.replace('$(output)', target_filename))
        print("Generated Havok configuration file: %s" % os.path.basename(output_configuration_file))
        print("Target filename: %s" % target_filename)
        
        havokScene = HavokScene()
        havokScene.file = scene_file
        havokScene.filterSet = output_configuration_file
        havokScene.assetPath = input_directory
        havokScene.outputPath = input_directory
        havokScene.sceneLength = float(utilities.parse_text(
            fbx_importer_output,
            label_scene_length,
            parse_index))
        havokScenes.append(havokScene)

        havok_content_tools.run(
            havokScene.file,
            havokScene.filterSet,
            havokScene.assetPath,
            havokScene.outputPath,
            False
            )

        isRootNode = False

        # Get the next file
        parse_index = fbx_importer_output.find(label_tag_file, parse_index + 1)

        utilities.print_line(True)

    print("Conversion complete!")

    return

def export_all():
    try:
        # Print the header
        utilities.print_line()
        print("Havok FBX Importer")
        utilities.print_line()

        # Do the export
        export()

        return 0
    except IOError as error:
        print("I/O error({0}): {1}".format(error[0], error[1]))
        utilities.wait()
    except ValueError:
        print("Could not convert data to an integer.")
        utilities.wait()
    except:
        print("Unexpected error:", sys.exc_info()[0])
        traceback.print_exc(file=sys.stdout)
        utilities.wait()

    return