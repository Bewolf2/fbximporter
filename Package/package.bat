@echo off

::
:: Helper script that builds the FBXImporter solution and then calls
:: the packaging script. Assumes you have VS2010 and Python installed.
::

set VS_BIN=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin
set SOLUTION=%~dp0\..\Workspace\FBXImporter.sln
set PACKAGE=%~dp0\package.py

:: Setup the Visual Studio environment variables
call "%VS_BIN%\vcvars32.bat"

:: Build the 'Dev DLL' solution which creates 'FBXImporter.exe'
msbuild %SOLUTION% /t:Build /p:Configuration="Dev DLL"

:: Run the packaging script
python %PACKAGE%