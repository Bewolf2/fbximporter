// TKBMS v1.0 -----------------------------------------------------
//
// PLATFORM		: WIN32 X64
// PRODUCT		: ALL
// VISIBILITY	: PUBLIC
//
// ------------------------------------------------------TKBMS v1.0

#define HK_CLASSES_FILE <Common/Serialize/Classlist/hkClasses.h>

#include <Common/Base/Config/hkProductFeaturesNoPatchesOrCompat.h>
#include <Common/Base/Config/hkProductFeatures.cxx>
#include <Common/Compat/Deprecated/Compat/hkCompat_None.cxx>

// Havok base infrastructure
#include <Common/Base/hkBase.h>
#include <Common/Base/Math/hkMath.h>

// Keycode
#include <Common/Base/keycode.cxx>

#include <Common/Base/Ext/hkBaseExt.h>
#include <Common/Base/System/hkBaseSystem.h>
#include <Common/Base/Memory/System/hkMemorySystem.h>
#include <Common/Base/Memory/System/Util/hkMemoryInitUtil.h>
#include <Common/Base/Memory/Allocator/Malloc/hkMallocAllocator.h>
#include <Common/Base/System/Error/hkError.h>
#include <Common/SceneData/Mesh/hkxMesh.h>

#include "FbxToHkxConverter.h"

static void HK_CALL havokErrorReport(const char* msg, void*)
{
	// Output to console
	printf("%s\n", msg);
}

int main(int argc, char* argv[])
{
	// initialize Havok internals
	{
		hkMemorySystem::FrameInfo frameInfo(0);

#ifdef _DEBUG
		// (Use debug mem manager to detect mem leaks in Havok code)
		hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initChecking(hkMallocAllocator::m_defaultMallocAllocator, frameInfo);
#else
		hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initFreeListLargeBlock(hkMallocAllocator::m_defaultMallocAllocator, frameInfo);
#endif

		hkBaseSystem::init( memoryRouter, havokErrorReport );

		hkError& errorhandler = hkError::getInstance();
		errorhandler.enableAll();
	}

	if (argc != 2)
	{
		printf("Invalid number of input arguments\n");
		printf("Usage: FBXImport <input_filename>\n");
		return -1;
	}

	// Load FBX and save as HKX
	{
		const char* filename = argv[1];

		FbxManager* fbxSdkManager = FbxManager::Create();
		if( !fbxSdkManager )
		{
			HK_ERROR(0x5213afed, "Unable to create FBX Manager!\n");
			return -1;
		}

		FbxIOSettings* fbxIoSettings = FbxIOSettings::Create(fbxSdkManager, IOSROOT);
		fbxSdkManager->SetIOSettings(fbxIoSettings);

		FbxImporter* fbxImporter = FbxImporter::Create(fbxSdkManager,"");

		if (!fbxImporter->Initialize(filename, -1, fbxSdkManager->GetIOSettings()))
		{
			HK_WARN(0x5216afed, "Failed to initialize the importer! Please ensure file " << filename << " exists\n");
			fbxSdkManager->Destroy();
			return -1;
		}

		FbxScene* fbxScene = FbxScene::Create(fbxSdkManager,"tempScene");
		if (!fbxScene)
		{
			HK_ERROR(0x5216afed, "Failed to create the scene!\n");
			fbxImporter->Destroy();
			fbxSdkManager->Destroy();
			return -1;
		}

		fbxImporter->Import(fbxScene);
		fbxImporter->Destroy();

		// Currently assume that the file is loaded from 3dsmax
		FbxAxisSystem::Max.ConvertScene(fbxScene);

		FbxToHkxConverter::Options options(fbxSdkManager);
		FbxToHkxConverter converter(options);

		if(converter.createScenes(fbxScene))
		{
			int lastSlashIndex = hkString::lastIndexOf(filename,'\\') + 1;
			int extensionIndex = hkString::lastIndexOf(filename,'.');

			hkStringBuf path;
			path.set(filename, lastSlashIndex);

			hkStringBuf name;
			name.set(filename + lastSlashIndex, extensionIndex - lastSlashIndex);

			converter.saveScenes(path, name);
		}
		else
		{
			HK_ERROR(0x0, "Failed to convert the scene!\n");
			fbxSdkManager->Destroy();
			return -1;
		}

		fbxSdkManager->Destroy();
	}

	// quit Havok
	{
		hkBaseSystem::quit();
		hkMemoryInitUtil::quit();
	}
	
	return 0;
}
