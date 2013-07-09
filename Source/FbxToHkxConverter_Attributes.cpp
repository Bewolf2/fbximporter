// TKBMS v1.0 -----------------------------------------------------
//
// PLATFORM		: WIN32 X64
// PRODUCT		: ALL
// VISIBILITY	: PUBLIC
//
// ------------------------------------------------------TKBMS v1.0

#include "FbxToHkxConverter.h"

// This file is templated on the contents of hctMayaSceneExporter_Attributes/hctMaxSceneExporter_Attributes.cpp and will need to be adapted to FBX

#include <Common/SceneData/Graph/hkxNode.h>
#include <Common/SceneData/Spline/hkxSpline.h>
#include <Common/Base/Reflection/hkClass.h>

void FbxToHkxConverter::addSampledNodeAttributeGroups(hkxScene *scene, int animStackIndex, FbxObject* fbxObject, hkxAttributeHolder* hkx_attributeHolder, bool recurse)
{	
	hkxAttributeGroup* currentAttributeGroup = HK_NULL;

	// Step through the current object's properties to extract attributes and their groups
	for(FbxProperty prop = fbxObject->GetFirstProperty(); prop.IsValid(); prop = fbxObject->GetNextProperty(prop))
	{
		FbxString propName = prop.GetName();
		hkStringOld name(propName.Buffer(), (int) propName.GetLen());

		// Attributes named 'hkType___' create a new group
		if(name.asLowerCase().beginsWith("hktype") && prop.GetPropertyDataType().GetType() == eFbxString)
		{
			currentAttributeGroup = hkx_attributeHolder->m_attributeGroups.expandBy(1);

			// Store the group name
			FbxString groupName = prop.Get<FbxString>();
			currentAttributeGroup->m_name = hkStringOld(groupName.Buffer(), (int) groupName.GetLen()).cString();

			continue;
		}

		// Skip if attribute is hidden
		if(prop.GetFlag(FbxPropertyAttr::eHidden))
		{
			continue;
		}

		// We are not exporting yet
		if(!currentAttributeGroup)
		{
			continue;
		}

		hkxAttribute hkxAttr;
		// Skip if creation of the HKX attribute fails
		if ( !createAndSampleAttribute(scene, animStackIndex, prop, hkxAttr) )
		{
			continue;
		}

		// Store the hkx attribute
		currentAttributeGroup->m_attributes.pushBack(hkxAttr);
	}

	// Prune empty groups
	{
		for (int i = hkx_attributeHolder->m_attributeGroups.getSize()-1; i>=0; --i)
		{
			if (hkx_attributeHolder->m_attributeGroups[i].m_attributes.getSize() == 0)
			{
				hkx_attributeHolder->m_attributeGroups.removeAtAndCopy(i);
			}
		}
	}

	// In the case of mesh nodes, recurse to extract attributes setup on their material... all other hkxAttributeHolders will be actual hkxNodes
	if(recurse && ((FbxNode*)fbxObject)->GetMaterialCount() && m_options.m_exportMaterials)
	{
		hkxMesh* mesh = HK_NULL;
		const hkClass* classType = ((hkxNode*)hkx_attributeHolder)->m_object.getClass();

		if(classType->equals(&hkxMeshClass))
		{
			mesh = (hkxMesh*) ((hkxNode*)hkx_attributeHolder)->m_object.val();
		}
		else if(classType->equals(&hkxSkinBindingClass))
		{
			hkxSkinBinding* skinBinding = (hkxSkinBinding*) ((hkxNode*)hkx_attributeHolder)->m_object.val();
			mesh = skinBinding->m_mesh;
		}

		if(mesh)
		{
			HK_ASSERT(0x0, mesh->m_sections.getSize() > 0);
			hkxMaterial* mat = mesh->m_sections[0]->m_material;

			// We currently extract 1 material per mesh, so no need to iterate over all materials
			addSampledNodeAttributeGroups( scene, animStackIndex, ((FbxNode*)fbxObject)->GetMaterial(0), mat, false );
		}
	}
}

bool FbxToHkxConverter::createAndSampleAttribute(hkxScene *scene, int animStackIndex, FbxProperty& prop, hkxAttribute& hkx_attribute)
{
	hkx_attribute.m_name = HK_NULL;
	hkx_attribute.m_value = HK_NULL;

	const FbxAnimStack* lAnimStack = m_curFbxScene->GetSrcObject<FbxAnimStack>(animStackIndex);
	const int numAnimLayers = lAnimStack ? lAnimStack->GetMemberCount<FbxAnimLayer>() : 0;
	FbxAnimLayer* lAnimLayer = (numAnimLayers > 0) ? lAnimStack->GetMember<FbxAnimLayer>(0) : 0;
	FbxAnimCurveNode* lCurveNode = lAnimLayer ? prop.GetCurveNode(lAnimLayer) : 0;
	
	// In the case of animated vectors/matrices, there's one curve per element that must be sampled
	FbxAnimCurve* lAnimCurves[16];
	FbxAnimCurve* &lFirstAnimCurve = lAnimCurves[0] = 0;
	int numAnimCurves = 0;
	if(lAnimLayer)
	{
		lFirstAnimCurve = prop.GetCurve(lAnimLayer);
		numAnimCurves = 1;
	}

	// Determine the number of keys we'll store for this attribute based on whether it's animated
	const hkUint32 numKeys = (lFirstAnimCurve && lFirstAnimCurve->KeyGetCount() > 1) ? scene->m_numFrames + 1 : 1;

	FbxDataType type = prop.GetPropertyDataType();
	EFbxType dataType = type.GetType();

	FbxTimeSpan animTimeSpan = lAnimStack->GetLocalTimeSpan();
	FbxTime timePerFrame; timePerFrame.SetTime(0, 0, 0, 1, 0, m_curFbxScene->GetGlobalSettings().GetTimeMode());

	// Since the end time is assumed to be inclusive, sample up to one frame beyond it
	const FbxTime startTime = animTimeSpan.GetStart();
	const FbxTime endTime = animTimeSpan.GetStop();

	union
	{
		hkInt32 m_i;
		hkFloat32 m_f;
		hkFloat32 m_v[16];
	} dataStorage;
	hkxAttribute::Hint dataTypeHint = hkxAttribute::HINT_NONE;

	switch(dataType)
	{
	case eFbxBool:
		{
			hkxSparselyAnimatedBool* animatedData = new hkxSparselyAnimatedBool();
			hkx_attribute.m_value = animatedData;

			if(numKeys > 1)
			{
				int currentKeyIndex = 0;

				// Sample this attribute for each frame
				for(FbxTime time = startTime, priorSampleTime = endTime; time < endTime; priorSampleTime = time, time += timePerFrame)
				{
					const bool currentValue = (bool) lFirstAnimCurve->Evaluate(time, &currentKeyIndex);
					const bool priorValue = (bool) lFirstAnimCurve->Evaluate(priorSampleTime);
					// Sample the current frame's value or, if a value hasn't yet been keyed, fall back to using the value from the end of the sample time
					if(priorValue != currentValue || animatedData->m_bools.getSize() == 0)
					{
						animatedData->m_bools.expandOne() = currentValue;
						animatedData->m_times.expandOne() = (hkReal) (time - startTime).GetSecondDouble();
					}
				}
			}
			else
			{
				animatedData->m_bools.expandOne() = prop.Get<bool>();
				animatedData->m_times.expandOne() = 0.f;
			}

			animatedData->removeReference();
			break;
		}

	case eFbxUShort:
		dataStorage.m_i = (hkInt32) prop.Get<FbxUShort>();
		goto handleInts;
	case eFbxUChar:
		dataStorage.m_i = (hkInt32) prop.Get<FbxUChar>();
		goto handleInts;
	case eFbxChar:
		dataStorage.m_i = (hkInt32) prop.Get<FbxChar>();
		goto handleInts;
	case eFbxShort:
		dataStorage.m_i = (hkInt32) prop.Get<FbxShort>();
		goto handleInts;
	case eFbxInt:
		dataStorage.m_i = (hkInt32) prop.Get<FbxInt>();
		goto handleInts;
	case eFbxUInt:
		dataStorage.m_i = (hkInt32) prop.Get<FbxUInt>();
		goto handleInts;
	case eFbxLongLong:
		dataStorage.m_i = (hkInt32) prop.Get<FbxLongLong>();
		goto handleInts;
	case eFbxULongLong:
		{
			dataStorage.m_i = (hkInt32) prop.Get<FbxULongLong>();

handleInts:
			hkxSparselyAnimatedInt* animatedData = new hkxSparselyAnimatedInt();
			hkx_attribute.m_value = animatedData;

			if(numKeys > 1)
			{
				int currentKeyIndex = 0;
				// Sample this attribute for each frame
				for(FbxTime time = startTime, priorSampleTime = endTime; time < endTime; priorSampleTime = time, time += timePerFrame)
				{
					const int currentValue = (int) lFirstAnimCurve->Evaluate(time, &currentKeyIndex);
					const int priorValue = (int) lFirstAnimCurve->Evaluate(priorSampleTime);
					// Sample the current frame's value or, if a value hasn't yet been keyed, fall back to using the value from the end of the sample time
					if(priorValue != currentValue || animatedData->m_ints.getSize() == 0)
					{
						animatedData->m_ints.expandOne() = currentValue;
						animatedData->m_times.expandOne() = (hkReal) (time - startTime).GetSecondDouble();
					}
				}
			}
			else
			{
				animatedData->m_ints.expandOne() = dataStorage.m_i;
				animatedData->m_times.expandOne() = 0.f;
			}

			animatedData->removeReference();
			break;
		}
	case eFbxDistance:
		dataTypeHint = hkxAttribute::HINT_SCALE;
		dataStorage.m_f = (hkFloat32) prop.Get<FbxDistance>().valueAs(m_curFbxScene->GetGlobalSettings().GetSystemUnit());
		goto handleFloats;
	case eFbxHalfFloat:
		dataStorage.m_f = (hkFloat32) prop.Get<FbxHalfFloat>().value();
		goto handleFloats;
	case eFbxFloat:
		dataStorage.m_f = (hkFloat32) prop.Get<FbxFloat>();
		goto handleFloats;
	case eFbxDouble:
		{
			dataStorage.m_f = (hkFloat32) prop.Get<FbxDouble>();
handleFloats:
			hkxAnimatedFloat* animatedData = new hkxAnimatedFloat();
			animatedData->m_hint = dataTypeHint;
			hkx_attribute.m_value = animatedData;
			animatedData->m_floats.setSize(numKeys);

			if(numKeys > 1)
			{
				int currentKeyIndex = 0;
				hkUint32 numFrames = 0;
				// Sample this attribute for each frame
				for(FbxTime time = startTime; time < endTime; time += timePerFrame, ++numFrames)
				{
					animatedData->m_floats[numFrames] = (hkFloat32) lFirstAnimCurve->Evaluate(time, &currentKeyIndex);
				}
				HK_ASSERT(0x0, numFrames == numKeys);
			}
			else
			{
				animatedData->m_floats[0] = dataStorage.m_f;
			}

			animatedData->removeReference();
			break;
		}

	case eFbxDouble2:
		numAnimCurves = 2;
		dataStorage.m_v[0] = (hkFloat32) prop.Get<FbxDouble2>()[0];
		dataStorage.m_v[1] = (hkFloat32) prop.Get<FbxDouble2>()[1];
		goto handleVectors;
	case eFbxDouble3:
		numAnimCurves = 3;
		dataStorage.m_v[0] = (hkFloat32) prop.Get<FbxDouble3>()[0];
		dataStorage.m_v[1] = (hkFloat32) prop.Get<FbxDouble3>()[1];
		dataStorage.m_v[2] = (hkFloat32) prop.Get<FbxDouble3>()[2];
		goto handleVectors;
	case eFbxDouble4:
		{
			numAnimCurves = 4;
			dataStorage.m_v[0] = (hkFloat32) prop.Get<FbxDouble4>()[0];
			dataStorage.m_v[1] = (hkFloat32) prop.Get<FbxDouble4>()[1];
			dataStorage.m_v[2] = (hkFloat32) prop.Get<FbxDouble4>()[2];
			dataStorage.m_v[3] = (hkFloat32) prop.Get<FbxDouble4>()[3];
handleVectors:
			hkxAnimatedVector* animatedData = new hkxAnimatedVector();
			animatedData->m_hint = dataTypeHint;
			hkx_attribute.m_value = animatedData;

			animatedData->m_vectors.setSize(numKeys * 4);

			if(numKeys > 1)
			{
				HK_ASSERT(0x0, lCurveNode);
				hkFloat32 endValues[4];
				// Initialize all animation curves and get the values for each curve at the end of the animation, in case they need to be "looped" 
				// back to the front of the animation
				for(int a = 0; a < numAnimCurves; ++a)
				{
					HK_ASSERT(0x0, lCurveNode->GetCurveCount(a) > 0);
					lAnimCurves[a] = lCurveNode->GetCurve(a);
					endValues[a] = lAnimCurves[a]->Evaluate(endTime);
				}

				hkUint32 numFrames = 0;
				int storedFloatIndex = 0;
				// Sample this attribute for each frame
				for(FbxTime time = startTime, priorSampleTime = endTime; time < endTime; priorSampleTime = time, time += timePerFrame, ++numFrames)
				{
					for(int a = 0; a < 4; ++a, ++storedFloatIndex)
					{
						if(a < numAnimCurves)
						{
							animatedData->m_vectors[storedFloatIndex] = (hkFloat32) lAnimCurves[a]->Evaluate(time);
						}
						else
						{
							animatedData->m_vectors[storedFloatIndex] = 0.f;
						}
					}
				}
			}
			else
			{
				for(int a = 0; a < numAnimCurves; ++a)
				{
					animatedData->m_vectors[a] = dataStorage.m_v[a];
				}
				for(int a = numAnimCurves; a < 4; ++a)
				{
					animatedData->m_vectors[a] = 0.f;
				}
			}

			animatedData->removeReference();
			break;
		}
	case eFbxDouble4x4:
		{
			numAnimCurves = 16;
			hkxAnimatedMatrix* animatedData = new hkxAnimatedMatrix();
			// Assume matrices are always transformed
			animatedData->m_hint = hkxAttribute::HINT_TRANSFORM_AND_SCALE;
			hkx_attribute.m_value = animatedData;

			animatedData->m_matrices.setSize(numKeys * 16);

			if(numKeys > 1)
			{
				HK_ASSERT(0x0, lCurveNode);
				hkFloat32 endValues[16];
				// Initialize all animation curves and get the values for each curve at the end of the animation, in case they need to be "looped" 
				// back to the front of the animation
				for(int a = 0; a < numAnimCurves; ++a)
				{
					HK_ASSERT(0x0, lCurveNode->GetCurveCount(a) > 0);
					lAnimCurves[a] = lCurveNode->GetCurve(a);
					endValues[a] = lAnimCurves[a]->Evaluate(endTime);
				}

				hkUint32 numFrames = 0;
				// Sample this attribute for each frame
				for(FbxTime time = startTime, priorSampleTime = endTime; time < endTime; priorSampleTime = time, time += timePerFrame, ++numFrames)
				{
					FbxMatrix fbxMatrix;
					for(int a = 0; a < numAnimCurves; ++a)
					{
						fbxMatrix[a] = (hkFloat32) lAnimCurves[a]->Evaluate(time);
					}
					hkMatrix4 mat;
					convertFbxXMatrixToMatrix4(fbxMatrix, mat);
					mat.get4x4ColumnMajor(&animatedData->m_matrices[numFrames * 16]);
				}
			}
			else
			{
				hkMatrix4 mat;
				FbxDouble4x4 tempMat = prop.Get<FbxDouble4x4>();
				FbxMatrix fbxMatrix = *reinterpret_cast<FbxMatrix*>(&tempMat);
				convertFbxXMatrixToMatrix4(fbxMatrix, mat);
				mat.get4x4ColumnMajor(&animatedData->m_matrices[0]);
			}

			animatedData->removeReference();
			break;
		}
	case eFbxEnum:
		{
			hkxEnum* enumHk = new hkxEnum;
			const int numEntries = prop.GetEnumCount();
			enumHk->m_items.setSize(numEntries);
			// Setup the enum string->index mapping
			for(int c = 0; c < numEntries; ++c)
			{
				enumHk->m_items[c].m_value = c;
				enumHk->m_items[c].m_name = prop.GetEnumValue(c);
			}

			hkxSparselyAnimatedEnum* animatedData = new hkxSparselyAnimatedEnum();
			hkx_attribute.m_value = animatedData;
			animatedData->m_enum = enumHk;
			enumHk->removeReference();

			if(numKeys > 1)
			{
				int currentKeyIndex;
				for(FbxTime time = startTime, priorSampleTime = endTime; time < endTime; priorSampleTime = time, time += timePerFrame)
				{
					const int currentValue = (int) lFirstAnimCurve->Evaluate(time, &currentKeyIndex);
					const int priorValue = (int) lFirstAnimCurve->Evaluate(priorSampleTime);
					// Sample the current frame's value or, if a value hasn't yet been keyed, fall back to using the value from the end of the sample time
					if(priorValue != currentValue || animatedData->m_ints.getSize() == 0)
					{
						animatedData->m_ints.expandOne() = currentValue;
						animatedData->m_times.expandOne() = (hkReal) (time - startTime).GetSecondDouble();
					}
				}
			}
			else
			{
				animatedData->m_ints.expandOne() = prop.Get<FbxEnum>();
				animatedData->m_times.expandOne() = 0.f;
			}
			animatedData->removeReference();
			break;
		}
	case eFbxString:
		{
			hkxSparselyAnimatedString* animatedData = new hkxSparselyAnimatedString();
			hkx_attribute.m_value = animatedData;

			// Animated strings aren't supported (require use of FbxEnum), so assume a single string value
			animatedData->m_strings.setSize(1);
			animatedData->m_times.setSize(1);
			animatedData->m_strings[0] = prop.Get<FbxString>();
			animatedData->m_times[0] = 0.f;
			animatedData->removeReference();
			break;
		}

		// Unsupported types:
	case eFbxTime:
	case eFbxDateTime:
	case eFbxReference:
	case eFbxBlob:
		return false;
	}

	// Update the attribute name
	hkx_attribute.m_name = prop.GetNameAsCStr();

	return true;
}
