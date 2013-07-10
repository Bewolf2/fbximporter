FBX Importer
============

Handles calling the FBX Importer executable and then processing the result of that with Havok Content Tools filter manager. Given an FBX file it will:

- Convert animated FBX files to rig and animation files
- Convert static FBX files to Vision static mesh files (.vmesh)

There are two primary pieces to this:

1. **FBXImporter.exe** - This is a C++ project that generates an executable that takes an FBX file as input and converts it to a Havok scene file (e.g. a tag
file with extension .hkx or .hkt).
2. **convert.py** - This takes an FBX file as input, calls FBXImporter.exe to generate the HKX files, and then it takes the outputted files and generates
corresponding Vision or Animation Studio compatible files by calling the Havok Content Tools standalone filter manager (**hctStandAloneFilterManager.exe**).

Dependencies
------------

* [Visual C++ 2010][1]
* [Python 2.7.X][2]
* [FBX SDK 2013.3][4] - **We will be upgrading to 2014.1 soon!**
* [Python Tools for Visual Studio][5] - Useful for debugging your projects and we include a .pyproj/.sln so you'll need to have it installed to successfully open the solution.

Setup
-----

1. Install the Project Anarchy SDK from the [homepage][3]
    * Make sure the content tools and SDK are checked for install
2. Git clone the repository into **$(AnarchySDK)\Tools\FBXImporter**
    * ```git clone https://github.com/projectanarchy/fbximporter.git FBXImporter```
3. Open **$(AnarchySDK)\Tools\FBXImporter\Workspace\FBXImporter.sln**
    * Build the project. This will output an executable to: **$(AnarchySDK)\Bin\Tools\FBXImporter.exe**
4. Now you can drag/drop an FBX file onto the Python convert script:
    * **$(AnarchySDK)\Tools\FBXImporter\Scripts\convert.py**

The following sections describe what files get generated for the particular kind of FBX asset. For all generated assets, there will be a cooresponding HKO file that contains the filter set used to generate the file. These are passed in to the Havok Content Tools standalone filter manager (**hctStandAloneFilterManager.exe**). For example, if you would like to re-generate the StaticBox example below:

```hctStandAloneFilterManager.exe -s StaticBox.hko StaticBox.hkt```

If you want to edit the filters manually, you can add the **interactive mode** with '-i' like so:

```hctStandAloneFilterManager.exe -i -s StaticBox.hko StaticBox.hkt```

***
### Static Mesh

If you have an FBX file named **StaticBox.fbx** that has no animations, passing it to **convert.py** will generate the following files:

- ```StaticBox.vmesh```
- ```StaticBox.hkt```
- ```StaticBox.hko``` - The configuration (filter set) that's passed to the filter tools.

***
### Animated Model

If you have an FBX file named **AnimatedBox.fbx** that has one animation named *Anim1*, passing this to **convert.py** will generate the following files:

- ```AnimatedBox.hkt```
- ```AnimatedBox.hko``` - Used to generate ```AnimatedBox__out_rig.hkx```.
- ```AnimatedBox__out_rig.hkx``` - Rig file used for Animation Studio. Put this in the **CharacterAssets** folder.
- ```AnimatedBox__out_anim_Anim1.hkx``` - Contains animation data that is compressed and includes extracted motion. Put this in the **Animations** folder.
- ```AnimatedBox_Anim1.hkt```
- ```AnimatedBox_Anim1.hko``` - Used to generate ```AnimatedBox__out_anim_Anim1.hkx```.

This is to be used with Animation Studio. **Vision Model files generation is currently not supported.**

Licence
-------

Confidential Information of Havok.  (C) Copyright 1999-2013 Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok Logo, and the Havok buzzsaw
logo are trademarks of Havok.  Title, ownership rights, and intellectual property rights in the Havok software remain in Havok and/or its suppliers.

Use of this software for evaluation purposes is subject to and indicates acceptance of the End User licence Agreement for this product. A copy of the
license is included with this software and is also available from salesteam@havok.com.

[1]: http://www.microsoft.com/visualstudio/eng/downloads#d-2010-express
[2]: http://www.python.org/download/releases/2.7.5/
[3]: http://www.projectanarchy.com/download
[4]: http://usa.autodesk.com/adsk/servlet/pc/item?siteID=123112&id=10775892
[5]: http://pytools.codeplex.com/