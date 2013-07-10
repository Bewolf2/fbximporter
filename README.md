FBX Importer
============

Handles calling the FBX Importer executable and then processing the result of
that with Havok Content Tools filter manager. Given an FBX file it will:

- Convert animated FBX files to rig and animation files
- Convert static FBX files to Vision static mesh files (**.vmesh**)

There are two primary pieces to this:

1. **FBXImporter.exe** - This is a C++ project that generates an executable that
takes an FBX file as input and converts it to a Havok scene file (e.g. a tag
file with extension .hkx or .hkt).
2. **convert.py** - This takes an FBX file as input, calls FBXImporter.exe to
generate the HKX files, and then it takes the outputted files and generates
corresponding Vision or Animation Studio compatible files by calling the
Havok Content Tools filter manager.

Dependencies
------------

* [Visual C++ 2010][1]
* [Python 2.7.X][2]

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

Licence
-------

Confidential Information of Havok.  (C) Copyright 1999-2013 Telekinesys Research
Limited t/a Havok. All Rights Reserved. The Havok Logo, and the Havok buzzsaw
logo are trademarks of Havok.  Title, ownership rights, and intellectual
property rights in the Havok software remain in Havok and/or its suppliers.

Use of this software for evaluation purposes is subject to and indicates
acceptance of the End User licence Agreement for this product. A copy of the
license is included with this software and is also available
from salesteam@havok.com.

[1]: http://www.microsoft.com/visualstudio/eng/downloads#d-2010-express
[2]: http://www.python.org/download/releases/2.7.5/
[3]: http://www.projectanarchy.com/download
