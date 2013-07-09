// TKBMS v1.0 -----------------------------------------------------
//
// PLATFORM		: WIN32 X64
// PRODUCT		: ALL
// VISIBILITY	: PUBLIC
//
// ------------------------------------------------------TKBMS v1.0

#include "FbxToHkxConverter.h"
#include <Common/SceneData/Skin/hkxSkinUtils.h>

unsigned FbxToHkxConverter::floatsToARGB(const float r, const float g, const float b, const float a) 
{
	return((unsigned char)(a * 255.0f) << 24) |
		((unsigned char)(r * 255.0f) << 16) |
		((unsigned char)(g * 255.0f) << 8) |
		((unsigned char)(b * 255.0f));
}

void FbxToHkxConverter::addSpline(hkxScene *scene, FbxNode* splineNode, hkxNode* node)
{
	hkxSpline* newSpline = new hkxSpline;

	FbxNurbsCurve* splineAttrib =(FbxNurbsCurve*)splineNode->GetNodeAttribute();
	newSpline->m_isClosed =(splineAttrib->GetType()== FbxNurbsCurve::eClosed);

	// Try to get bezier curve data out of the function set
	const int numControlPoints = splineAttrib->GetControlPointsCount();
	for(int c=1; c<=numControlPoints; c+=3)
	{
		FbxVector4 cvPtL, cvPtM, cvPtR;
		cvPtL = splineAttrib->GetControlPointAt((c==0 && newSpline->m_isClosed)? numControlPoints-1 : hkMath::max2(0, c-2));  // If spline is closed, get 'in' from last knot for first i==0.
		cvPtM = splineAttrib->GetControlPointAt(c-1);
		cvPtR = splineAttrib->GetControlPointAt((c==numControlPoints && newSpline->m_isClosed)? 0 : hkMath::min2(c, numControlPoints-1));  // If spline is closed, get 'in' from last knot for first i==0.

		hkxSpline::ControlPoint& controlpoint = newSpline->m_controlPoints.expandOne();

		controlpoint.m_tangentIn.set((float)cvPtL[0],(float)cvPtL[1],(float)cvPtL[2]);
		controlpoint.m_position.set((float)cvPtM[0],(float)cvPtM[1],(float)cvPtM[2]);
		controlpoint.m_tangentOut.set((float)cvPtR[0],(float)cvPtR[1],(float)cvPtR[2]);
		controlpoint.m_inType = hkxSpline::CUSTOM; //
		controlpoint.m_outType = hkxSpline::CUSTOM; //
	}

	node->m_object = newSpline;
	scene->m_splines.pushBack(newSpline);
	newSpline->removeReference();
}

void FbxToHkxConverter::addCamera(hkxScene *scene, FbxNode* cameraNode, hkxNode* node)
{
	hkxCamera* newCamera = new hkxCamera();

	FbxCamera* cameraAttrib =(FbxCamera*)cameraNode->GetNodeAttribute();
	HK_ASSERT(0x0, cameraAttrib->GetAttributeType()== FbxNodeAttribute::eCamera);

	FbxDouble3 pos = cameraAttrib->Position.Get();
	newCamera->m_from.set((hkReal)pos[0],(hkReal)pos[1],(hkReal)pos[2]);
	FbxDouble3 up = cameraAttrib->UpVector.Get();
	newCamera->m_up.set((hkReal)up[0],(hkReal)up[1],(hkReal)up[2]);
	FbxDouble3 focus = cameraAttrib->InterestPosition.Get();
	newCamera->m_focus.set((hkReal)focus[0],(hkReal)focus[1],(hkReal)focus[2]);

	const hkReal degreesToRadians  = HK_REAL_PI / 180.0f;
	newCamera->m_fov =(hkReal)cameraAttrib->FieldOfViewY.Get()* degreesToRadians;
	newCamera->m_near =(hkReal)cameraAttrib->NearPlane.Get();
	newCamera->m_far =(hkReal)cameraAttrib->FarPlane.Get();

	newCamera->m_leftHanded = false;

	node->m_object = newCamera;
	scene->m_cameras.pushBack(newCamera);
	newCamera->removeReference();
}

void FbxToHkxConverter::addLight(hkxScene *scene, FbxNode* lightNode, hkxNode* node)
{
	hkxLight* newLight = new hkxLight();

	FbxLight* lightAttrib =(FbxLight*)lightNode->GetNodeAttribute();
	HK_ASSERT(0x0, lightAttrib->GetAttributeType()== FbxNodeAttribute::eLight);

	const FbxAMatrix& lightTransform = lightNode->EvaluateLocalTransform();
	const FbxVector4& lightPos = lightTransform.GetT();
	newLight->m_position.set((hkReal)lightPos[0],(hkReal)lightPos[1],(hkReal)lightPos[2]);

	// FBX lights point along their node's negative Y axis
	const FbxVector4& negLightDir = lightTransform.GetRow(1);
	newLight->m_direction.set((hkReal)-negLightDir[0],(hkReal)-negLightDir[1],(hkReal)-negLightDir[2]);

	const FbxDouble3 color = lightAttrib->Color.Get();
	newLight->m_color = floatsToARGB((hkReal)color[0],(hkReal)color[1],(hkReal)color[2]); 

	newLight->m_intensity =(hkReal)lightAttrib->Intensity.Get();
	newLight->m_decayRate =(hkInt16)lightAttrib->DecayType.Get();
	newLight->m_shadowCaster = lightAttrib->CastShadows.Get();

	switch (lightAttrib->LightType.Get())
	{
	case FbxLight::ePoint:
		{
			newLight->m_type = hkxLight::DIRECTIONAL_LIGHT;
			if (newLight->m_decayRate)
			{
				float cutOff = 0.01f;
				if (newLight->m_decayRate)
				{
					// Calculate range of new light
					newLight->m_range = hkMath::pow((hkReal)(newLight->m_intensity / cutOff),(hkReal)(1.f / newLight->m_decayRate));
				}
				else
				{
					HK_WARN_ALWAYS(0x0, "Point lights with no decay are not supported. Please use a directional light, or ambient lighting instead.");
					newLight->removeReference();
					newLight = HK_NULL;
				}
			}
			break;
		}
	case FbxLight::eDirectional:
		{
			newLight->m_type = hkxLight::DIRECTIONAL_LIGHT;
			newLight->m_range =(hkReal)lightAttrib->FarAttenuationEnd.Get();
			break;
		}
	case FbxLight::eSpot:
		{
			newLight->m_angle =(hkReal)lightAttrib->InnerAngle.Get();
			newLight->m_type = hkxLight::SPOT_LIGHT;
			newLight->m_range =(hkReal)lightAttrib->FarAttenuationEnd.Get();
			break;
		}
	default:
		HK_WARN(0x0, "Unsupported light type encountered. Expected Point, Directional, or Spot light.");
		break;
	}

	if (newLight)
	{
		// This is the range that the camera will start to fade the light out(because it is too far away)
		newLight->m_fadeStart = newLight->m_range * 2;
		newLight->m_fadeEnd = newLight->m_range * 3;

		node->m_object = newLight;
		scene->m_lights.pushBack(newLight);
		newLight->removeReference();
	}
}

static hkxMaterial* createDefaultMaterial(const char* name)
{
	hkxMaterial* mat = new hkxMaterial();
	mat->m_name = name;
	mat->m_diffuseColor.set(1.0f, 1.0f, 1.0f, 1.0f);
	mat->m_ambientColor.setAll(0);
	mat->m_specularColor = mat->m_diffuseColor;
	mat->m_specularColor(3)= 75.0f; // Spec power
	mat->m_emissiveColor.setAll(0);
	mat->m_specularMultiplier = 0.f;
	mat->m_specularExponent = 1.f;
	mat->m_transparency = hkxMaterial::transp_none;
	mat->m_uvMapOffset[0] = mat->m_uvMapOffset[1] = 0.f;
	mat->m_uvMapScale[0] = mat->m_uvMapScale[1] = 1.f;
	mat->m_uvMapRotation = 0.f;
	mat->m_uvMapAlgorithm = hkxMaterial::UVMA_3DSMAX_STYLE;
	return mat;
}

void FbxToHkxConverter::addMesh(hkxScene *scene, FbxNode* meshNode, hkxNode* node)
{
	FbxMesh* originalMesh =(FbxMesh*)meshNode->GetNodeAttribute();
	FbxMesh* triMesh;

	if (!originalMesh->IsTriangleMesh())
	{
		 FbxGeometryConverter lGeometryConverter(m_options.m_fbxSdkManager);
		 bool status;
		 triMesh = lGeometryConverter.TriangulateMeshAdvance(originalMesh,status);

		 HK_ASSERT(0,status);
	}
	else
	{
		triMesh = originalMesh;
	}

	hkxMesh* newMesh = HK_NULL;
	hkxSkinBinding* newSkin = HK_NULL;

	hkArray<hkxMeshSection*> exportedSections;

	// Get material
	hkxMaterial* sectMat = HK_NULL;
	if (m_options.m_exportMaterials)
	{
		sectMat = createMaterial(scene, originalMesh);	// Use original mesh for materials
		if (sectMat == HK_NULL)
		{
			sectMat = createDefaultMaterial("default_material");
		}
		scene->m_materials.pushBack(sectMat);
	}

	// Get skinning info	
	const int lSkinCount = triMesh->GetDeformerCount(FbxDeformer::eSkin);
	hkArray<float> skinControlPointWeights;
	hkArray<int> skinIndicesToClusters;
	{
		if (lSkinCount>0)
		{
			const int skinDataCount = triMesh->GetControlPointsCount()*4;
			skinControlPointWeights.setSize(skinDataCount,0.0f);
			skinIndicesToClusters.setSize(skinDataCount,-1);
	
			const int lClusterCount =((FbxSkin *)triMesh->GetDeformer(0, FbxDeformer::eSkin))->GetClusterCount();
			for (int curClusterIndex=0; curClusterIndex<lClusterCount; ++curClusterIndex)
			{
				FbxCluster* lCluster =((FbxSkin *)triMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(curClusterIndex);
				const int lIndexCount = lCluster->GetControlPointIndicesCount();
				int* lIndices = lCluster->GetControlPointIndices();
				double* lWeights = lCluster->GetControlPointWeights();
	
				for (int k = 0; k < lIndexCount; k++)
				{
					const int controlPointIndexFour = lIndices[k] * 4;
					for(int i=controlPointIndexFour; i<controlPointIndexFour+4; ++i)
					{
						if (skinIndicesToClusters[i]<0)
						{
							skinIndicesToClusters[i] = curClusterIndex;
							skinControlPointWeights[i] =(float)lWeights[k];
							break;
						}
					}
				}
			}
		}
	
		// Zero unused indices
		for (int i = 0; i <skinIndicesToClusters.getSize(); ++i)
		{
			if (skinIndicesToClusters[i]<0)
			{
				skinIndicesToClusters[i] = 0;
			}
		}
	}	

	// Vertex buffer
	hkxVertexBuffer* newVB = new hkxVertexBuffer();
	hkxIndexBuffer* newIB = new hkxIndexBuffer();
	fillBuffers(triMesh, meshNode, newVB, newIB, skinControlPointWeights, skinIndicesToClusters);

	hkxMeshSection* newSection = new hkxMeshSection();
	newSection->m_material = sectMat;
	newSection->m_vertexBuffer = newVB;
	newSection->m_indexBuffers.setSize(1);
	newSection->m_indexBuffers[0] = newIB;
	exportedSections.pushBack(newSection);

	if (sectMat)
	{
		sectMat->removeReference();
	}

	newVB->removeReference();
	newIB->removeReference();

	newMesh = new hkxMesh();
	newMesh->m_sections.setSize(exportedSections.getSize());
	for(int cs =0; cs < newMesh->m_sections.getSize(); ++cs)
	{
		newMesh->m_sections[cs] = exportedSections[cs];
		exportedSections[cs]->removeReference();
	}

	// Add skin bindings
	if (lSkinCount > 0)
	{
		newSkin = new hkxSkinBinding();
		newSkin->m_mesh = newMesh;

		const int lClusterCount =((FbxSkin *)triMesh->GetDeformer(0, FbxDeformer::eSkin))->GetClusterCount();
		newSkin->m_bindPose.setSize(lClusterCount);
		newSkin->m_nodeNames.setSize(lClusterCount);

		// Extract bind pose transforms & bone names
		for(int curClusterIndex=0; curClusterIndex<lClusterCount; ++curClusterIndex)
		{
			FbxCluster* lCluster =((FbxSkin *)triMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(curClusterIndex);

			newSkin->m_nodeNames[curClusterIndex] =(char*)lCluster->GetLink()->GetName();

			FbxAMatrix lMatrix;
			lMatrix = lCluster->GetLink()->EvaluateGlobalTransform();

			convertFbxXMatrixToMatrix4(lMatrix, newSkin->m_bindPose[curClusterIndex]);
		}

		// Extract the world transform of the original, skinned mesh
		{
			FbxAMatrix lMatrix;			
			lMatrix = meshNode->EvaluateGlobalTransform();
			convertFbxXMatrixToMatrix4(lMatrix, newSkin->m_initSkinTransform);
		}
	}

	if (newMesh)
	{
 		if (newSkin)
 		{
 			node->m_object = newSkin;

 			scene->m_meshes.pushBack(newMesh);
 			scene->m_skinBindings.pushBack(newSkin);
 			newMesh->removeReference();
 			newSkin->removeReference();
 		}
 		else
 		{
 			node->m_object = newMesh;

 			scene->m_meshes.pushBack(newMesh);
 			newMesh->removeReference();
 		}
	}
}

void FbxToHkxConverter::fillBuffers(FbxMesh* pMesh, FbxNode* originalNode, hkxVertexBuffer* newVB, hkxIndexBuffer* newIB, const hkArray<float>& skinControlPointWeights, 
				 const hkArray<int>& skinIndicesToClusters)
{
	// Vertex buffer
	{
		const int lPolygonCount = pMesh->GetPolygonCount();		
		hkxVertexDescription desiredVertDesc;

		desiredVertDesc.m_decls.pushBack(hkxVertexDescription::ElementDecl(hkxVertexDescription::HKX_DU_POSITION, hkxVertexDescription::HKX_DT_FLOAT, 3)); 

		if (pMesh->GetElementNormal(0)!=NULL)
		{
			desiredVertDesc.m_decls.pushBack(hkxVertexDescription::ElementDecl(hkxVertexDescription::HKX_DU_NORMAL, hkxVertexDescription::HKX_DT_FLOAT, 3));
		}

		if (pMesh->GetElementVertexColor(0)!=NULL)
		{
			desiredVertDesc.m_decls.pushBack(hkxVertexDescription::ElementDecl(hkxVertexDescription::HKX_DU_COLOR, hkxVertexDescription::HKX_DT_UINT32, 1));
		}

		for (int c = 0, numUVs = pMesh->GetElementUVCount(); c < numUVs; ++c)
		{
			desiredVertDesc.m_decls.pushBack(hkxVertexDescription::ElementDecl(hkxVertexDescription::HKX_DU_TEXCOORD, hkxVertexDescription::HKX_DT_FLOAT, 2));
		}

		if (skinControlPointWeights.getSize()>0 && skinIndicesToClusters.getSize()>0)
		{
			desiredVertDesc.m_decls.pushBack(hkxVertexDescription::ElementDecl(hkxVertexDescription::HKX_DU_BLENDWEIGHTS, hkxVertexDescription::HKX_DT_UINT8, 4));
			desiredVertDesc.m_decls.pushBack(hkxVertexDescription::ElementDecl(hkxVertexDescription::HKX_DU_BLENDINDICES, hkxVertexDescription::HKX_DT_UINT8, 4)); 
		}

		FbxAMatrix geometricTransform;
		{
			FbxNode* meshNode = originalNode;
			FbxVector4 T = meshNode->GetGeometricTranslation(FbxNode::eSourcePivot);
			FbxVector4 R = meshNode->GetGeometricRotation(FbxNode::eSourcePivot);
			FbxVector4 S = meshNode->GetGeometricScaling(FbxNode::eSourcePivot);
			geometricTransform.SetTRS(T,R,S);
		}
		
		// XXX be safe, set the maximum possible vertex num... assuming triangle lists
		const int numVertices = lPolygonCount*3; 
		newVB->setNumVertices(numVertices, desiredVertDesc);

		const hkxVertexDescription& vertDesc = newVB->getVertexDesc();
		const hkxVertexDescription::ElementDecl* posDecl = vertDesc.getElementDecl(hkxVertexDescription::HKX_DU_POSITION, 0);
		const hkxVertexDescription::ElementDecl* normDecl = vertDesc.getElementDecl(hkxVertexDescription::HKX_DU_NORMAL, 0);
		const hkxVertexDescription::ElementDecl* colorDecl = vertDesc.getElementDecl(hkxVertexDescription::HKX_DU_COLOR, 0);
		const hkxVertexDescription::ElementDecl* weightsDecl = vertDesc.getElementDecl(hkxVertexDescription::HKX_DU_BLENDWEIGHTS, 0);
		const hkxVertexDescription::ElementDecl* indicesDecl = vertDesc.getElementDecl(hkxVertexDescription::HKX_DU_BLENDINDICES, 0);

		const int posStride = posDecl? posDecl->m_byteStride : 0;
		const int normStride = normDecl? normDecl->m_byteStride : 0;
		const int colorStride = colorDecl? colorDecl->m_byteStride : 0;
		const int weightsStride = weightsDecl? weightsDecl->m_byteStride : 0;
		const int indicesStride = indicesDecl? indicesDecl->m_byteStride : 0;

		char* posBuf = static_cast<char*>(posDecl? newVB->getVertexDataPtr(*posDecl): HK_NULL);
		char* normBuf = static_cast<char*>(normDecl? newVB->getVertexDataPtr(*normDecl): HK_NULL);
		char* colorBuf = static_cast<char*>(colorDecl? newVB->getVertexDataPtr(*colorDecl): HK_NULL);
		char* weightsBuf = static_cast<char*>(weightsDecl? newVB->getVertexDataPtr(*weightsDecl): HK_NULL);
		char* indicesBuf = static_cast<char*>(indicesDecl? newVB->getVertexDataPtr(*indicesDecl): HK_NULL);

		const int maxNumUVs = (int) hkxMaterial::PROPERTY_MTL_UV_ID_STAGE_MAX - (int) hkxMaterial::PROPERTY_MTL_UV_ID_STAGE0;
		hkArray<int>::Temp textureCoordinateArrayPositions(maxNumUVs);
		textureCoordinateArrayPositions.setSize(maxNumUVs);
		hkString::memSet4(textureCoordinateArrayPositions.begin(), 0, textureCoordinateArrayPositions.getSize());

		FbxVector4* lControlPoints = pMesh->GetControlPoints(); 
		int vertexId = 0;
		for (int i = 0; i < lPolygonCount; i++)
		{
			const int lPolygonSize = pMesh->GetPolygonSize(i);

			HK_ASSERT(0x0,lPolygonSize==3);

			for (int j = 0; j < lPolygonSize; j++)
			{
				const int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
				
				if (posBuf)
				{
					FbxVector4 fbxPos = lControlPoints[lControlPointIndex];
					fbxPos = geometricTransform.MultT(fbxPos);

					float* _pos =(float*)(posBuf);
					_pos[0] =(float)fbxPos[0]; _pos[1] =(float)fbxPos[1]; _pos[2] =(float)fbxPos[2]; _pos[3] = 0;
					posBuf += posStride;
				}

				if (normBuf)
				{
					FbxVector4 fbxNormal;

					FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(0);

					const FbxGeometryElement::EMappingMode mappingMode = leNormal->GetMappingMode();

					if (mappingMode == FbxGeometryElement::eByPolygonVertex)
					{
						switch(leNormal->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							fbxNormal = leNormal->GetDirectArray().GetAt(vertexId);
							break;
						case FbxGeometryElement::eIndexToDirect:
							{
								int id = leNormal->GetIndexArray().GetAt(vertexId);
								fbxNormal = leNormal->GetDirectArray().GetAt(id);
							}
							break;
						default:
							break; // Other reference modes not shown here!
						}
					}
					else if (mappingMode == FbxGeometryElement::eByControlPoint)
					{
						switch(leNormal->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							fbxNormal = leNormal->GetDirectArray().GetAt(lControlPointIndex);
							break;
						case FbxGeometryElement::eIndexToDirect:
							{
								int id = leNormal->GetIndexArray().GetAt(lControlPointIndex);
								fbxNormal = leNormal->GetDirectArray().GetAt(id);
							}
							break;
						default:
							break; // Other reference modes not shown here!
						}
					}

					float* _normal =(float*)(normBuf);
					_normal[0] =(float)fbxNormal[0]; _normal[1] =(float)fbxNormal[1]; _normal[2] =(float)fbxNormal[2]; _normal[3] = 0;
					normBuf += normStride;
				}
				

				FbxStringList lUVSetNameList;
				pMesh->GetUVSetNames(lUVSetNameList);
				// Tex coord UV channels
				for(int t = 0, numUVs = hkMath::min2(lUVSetNameList.GetCount(), maxNumUVs); t < numUVs; ++t)
				{
					const char* lUVSetName = lUVSetNameList.GetStringAt(t);
					const FbxGeometryElementUV* leUV = pMesh->GetElementUV(lUVSetName);
					const hkxVertexDescription::ElementDecl* texDecl = vertDesc.getElementDecl(hkxVertexDescription::HKX_DU_TEXCOORD, t);
					HK_ASSERT(0x0, leUV && texDecl);

					const int texCoordStride = texDecl->m_byteStride;
					char* texCoordBuf = static_cast<char*>(newVB->getVertexDataPtr(*texDecl));
					FbxVector2 fbxUV;
 
 					switch (leUV->GetMappingMode())
 					{
					case FbxGeometryElement::eByControlPoint:
 						switch(leUV->GetReferenceMode())
 						{
						case FbxGeometryElement::eDirect:
 							fbxUV = leUV->GetDirectArray().GetAt(lControlPointIndex);
 							break;
						case FbxGeometryElement::eIndexToDirect:
 							{
 								int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
 								fbxUV = leUV->GetDirectArray().GetAt(id);
 							}
 							break;
 						default:
 							break; // Other reference modes not shown here!
 						}
 						break;
 
 					case FbxGeometryElement::eByPolygonVertex:
 						{
 							int lTextureUVIndex = pMesh->GetTextureUVIndex(i, j);
 							switch(leUV->GetReferenceMode())
 							{
 							case FbxGeometryElement::eDirect:
 							case FbxGeometryElement::eIndexToDirect:
 								{
 									fbxUV = leUV->GetDirectArray().GetAt(lTextureUVIndex);
 								}
 								break;
 							default:
 								break; // Other reference modes not shown here!
 							}
 						}
 						break;
 
 					case FbxGeometryElement::eByPolygon: // Doesn't make much sense for UVs
 					case FbxGeometryElement::eAllSame:   // Doesn't make much sense for UVs
 					case FbxGeometryElement::eNone:       // Doesn't make much sense for UVs
 						break;
 					}

					float* _uv =(float*)(texCoordBuf + textureCoordinateArrayPositions[t]);
					_uv[0] =(float)fbxUV[0];
					_uv[1] =(float)fbxUV[1];
					textureCoordinateArrayPositions[t] += texCoordStride;
				}

 				if (colorBuf)
 				{
 					FbxGeometryElementVertexColor* leVtxc = pMesh->GetElementVertexColor(0);
					FbxColor fbxColor;
 
 					if (leVtxc != NULL)
 					{
	 					switch(leVtxc->GetMappingMode())
	 					{
	 					case FbxGeometryElement::eByControlPoint:
	 						switch(leVtxc->GetReferenceMode())
	 						{
	 						case FbxGeometryElement::eDirect:
	 							fbxColor = leVtxc->GetDirectArray().GetAt(lControlPointIndex);
	 							break;
	 						case FbxGeometryElement::eIndexToDirect:
	 							{
	 								int id = leVtxc->GetIndexArray().GetAt(lControlPointIndex);
	 								fbxColor = leVtxc->GetDirectArray().GetAt(id);
	 							}
	 							break;
	 						default:
	 							break; // Other reference modes not shown here!
	 						}
	 						break;
	 
	 					case FbxGeometryElement::eByPolygonVertex:
	 						{
	 							switch(leVtxc->GetReferenceMode())
	 							{
	 							case FbxGeometryElement::eDirect:
	 								fbxColor = leVtxc->GetDirectArray().GetAt(vertexId);
	 								break;
	 							case FbxGeometryElement::eIndexToDirect:
	 								{
	 									int id = leVtxc->GetIndexArray().GetAt(vertexId);
	 									fbxColor = leVtxc->GetDirectArray().GetAt(id);
	 								}
	 								break;
	 							default:
	 								break; // Other reference modes not shown here!
	 							}
	 						}
	 						break;
	 
	 					case FbxGeometryElement::eByPolygon: // Doesn't make much sense for UVs
	 					case FbxGeometryElement::eAllSame:   // Doesn't make much sense for UVs
	 					case FbxGeometryElement::eNone:      // Doesn't make much sense for UVs
	 						break;
	 					}
 					}

					// Vert color
					unsigned color = floatsToARGB((float)fbxColor.mRed,(float)fbxColor.mGreen,(float)fbxColor.mBlue,(float)fbxColor.mAlpha); 

					hkUint32* _color =(hkUint32*)(colorBuf);
					*_color = color;

					_color += colorStride;
 				}

				if (weightsBuf && indicesBuf)
				{
					const int controlPointFour = lControlPointIndex * 4;
					
					// Add skin indices
					{
						unsigned int compressedI =  unsigned int(skinIndicesToClusters[controlPointFour]  )<< 24 | 
													unsigned int(skinIndicesToClusters[controlPointFour+1])<< 16 | 
													unsigned int(skinIndicesToClusters[controlPointFour+2])<< 8  | 
													unsigned int(skinIndicesToClusters[controlPointFour+3]);

						hkUint32* curIndexBuf =(hkUint32*)(indicesBuf);
						*curIndexBuf = compressedI;
					}
					
					// Add skin weights
					{
						unsigned int compressedW = 0;
						{
							hkReal tempWeights[4];
							for(int i=0; i<4; i++)
							{
								tempWeights[i] = skinControlPointWeights[controlPointFour+i];
							}
	
							hkUint8 tempQWeights[4];
							hkxSkinUtils::quantizeWeights(tempWeights, tempQWeights);
	
							compressedW =	unsigned int(tempQWeights[0])<< 24 |
											unsigned int(tempQWeights[1])<< 16 | 
											unsigned int(tempQWeights[2])<< 8  | 
											unsigned int(tempQWeights[3]);
						}

						hkUint32* _w =(hkUint32*)(weightsBuf);
						*_w = compressedW;
					}					

					weightsBuf += weightsStride;
					indicesBuf += indicesStride;
				}				
	
				vertexId++;
			} // For polygonSize
		} // For polygonCount
	}

	// Index buffer... assumes triangle list for now
	{
		newIB->m_indexType = hkxIndexBuffer::INDEX_TYPE_TRI_LIST;
		newIB->m_vertexBaseOffset = 0;
		newIB->m_length = pMesh->GetPolygonCount()* 3;
		newIB->m_indices16.setSize(newIB->m_length);

		hkUint16* curIndex = newIB->m_indices16.begin();
		for(int i = 0, vertexId = 0; i < pMesh->GetPolygonCount(); i++)
		{
			for(int j = 0; j < 3; j++)
			{
				*curIndex =(hkUint16)vertexId++;
				curIndex++;
			}
		}
	}
}

template<typename T> static T* getFbxTexture(FbxProperty& materialProperty)
{
	const int textureCount = materialProperty.GetSrcObjectCount<T>();
	if (textureCount > 0)
	{
		if (textureCount > 1)
		{
			HK_WARN(0x0, "More than 1 " << T::ClassId.GetName()<< " found... using first one.");
		}
		return materialProperty.GetSrcObject<T>(0);
	}
	return HK_NULL;
}

hkReferencedObject* FbxToHkxConverter::convertTexture(
	hkxScene *scene,
	FbxSurfaceMaterial* material,
	const FbxStringList& uvSetNames,
	hkxMaterial* mat,
	const char* fbxTextureType,
	int& uvSetIndex)
{
	FbxProperty lProperty = material->FindProperty(fbxTextureType);
	FbxFileTexture* fbxTextureFile = getFbxTexture<FbxFileTexture>(lProperty);
	if (fbxTextureFile)
	{
		// Determine the UV set index to which this texture is associated... fall back to the 0th index in case it can't be found
		{
			const char* str = fbxTextureFile->UVSet.Get().Buffer();
			FbxStringListItem listItem(str);
			uvSetIndex = uvSetNames.Find(listItem);
			const int maxNumUVs = (int) hkxMaterial::PROPERTY_MTL_UV_ID_STAGE_MAX - (int) hkxMaterial::PROPERTY_MTL_UV_ID_STAGE0;
			if (uvSetIndex < 0 || uvSetIndex >= maxNumUVs)
			{
				uvSetIndex = 0;
			}
		}

		// Update the UV mapping parameters stored within the material
		mat->m_uvMapAlgorithm = hkxMaterial::UVMA_3DSMAX_STYLE;
		mat->m_uvMapOffset[0] =(hkReal)fbxTextureFile->GetUVTranslation()[0];
		mat->m_uvMapOffset[1] =(hkReal)fbxTextureFile->GetUVTranslation()[1];
		mat->m_uvMapScale[0] =(hkReal)fbxTextureFile->GetUVScaling()[0];
		mat->m_uvMapScale[1] =(hkReal)fbxTextureFile->GetUVScaling()[1];
		mat->m_uvMapRotation =(hkReal)fbxTextureFile->GetRotationW();

		hkPointerMap<FbxTexture*, hkRefVariant*>::Iterator it = m_convertedTextures.findKey(fbxTextureFile);
		if (m_convertedTextures.isValid(it))
		{
			hkReferencedObject* texture = m_convertedTextures.getValue(it)->val();
			texture->addReference();
			return texture;
		}
		else
		{
			hkxTextureFile* textureFile = new hkxTextureFile;
			{
				textureFile->m_name = fbxTextureFile->GetName();
				textureFile->m_originalFilename = textureFile->m_filename = fbxTextureFile->GetFileName();
			}
			hkRefVariant* convertedData = new hkRefVariant(textureFile, &textureFile->staticClass());
			m_convertedTextures.insert(fbxTextureFile, convertedData);
			scene->m_externalTextures.pushBack(textureFile);
			return textureFile;
		}
	}

	if (getFbxTexture<FbxLayeredTexture>(lProperty)|| getFbxTexture<FbxProceduralTexture>(lProperty))
	{
		HK_WARN(0x0, "Encountered unsupported texture type - only file textures are currently supported.");
	}

	return HK_NULL;
}

void FbxToHkxConverter::convertTexture(hkxScene *scene, FbxSurfaceMaterial* fbxMat, const FbxStringList& uvSetNames, hkxMaterial* mat, const char* textureTypeName, hkxMaterial::TextureType textureType)
{
	int uvSetIndex = 0;
	hkReferencedObject* convertedTexture = convertTexture(scene, fbxMat, uvSetNames, mat, textureTypeName, uvSetIndex);
	if (convertedTexture)
	{
		hkxMaterial::TextureStage& stage = mat->m_stages.expandOne();
		stage.m_texture = convertedTexture;
		convertedTexture->removeReference();
		stage.m_usageHint = textureType;
		stage.m_tcoordChannel = uvSetIndex;
	}
}

void FbxToHkxConverter::convertTextures(hkxScene *scene, FbxSurfaceMaterial* fbxMat, const FbxStringList& uvSetNames, hkxMaterial* mat)
{
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sDiffuse, hkxMaterial::TEX_DIFFUSE);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sSpecular, hkxMaterial::TEX_SPECULAR);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sEmissive, hkxMaterial::TEX_EMISSIVE);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sBump, hkxMaterial::TEX_BUMP);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sDisplacementFactor, hkxMaterial::TEX_DISPLACEMENT);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sNormalMap, hkxMaterial::TEX_NORMAL);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sReflection, hkxMaterial::TEX_REFLECTION);
	convertTexture(scene, fbxMat, uvSetNames, mat, FbxSurfaceMaterial::sTransparencyFactor, hkxMaterial::TEX_OPACITY);

	FbxProperty lProperty = fbxMat->FindProperty(FbxSurfaceMaterial::sTransparencyFactor);

	// Adjust whether this material has alpha blending enabled
	if (lProperty.IsValid())
	{
		mat->m_transparency = hkxMaterial::transp_alpha;
	}
}

hkxMaterial* FbxToHkxConverter::createMaterial(hkxScene *scene, FbxMesh* pMesh)
{
	hkxMaterial* mat = HK_NULL;
	FbxSurfaceMaterial *lMaterial = 0;
	FbxNode* lNode = pMesh->GetNode();

	int lMaterialCount = 0;
	if (lNode)
	{
		lMaterialCount = lNode->GetMaterialCount();
	}

	// Currently assuming just one material per mesh
	if (lMaterialCount>0)
	{
		lMaterial = lNode->GetMaterial(0);
		mat = createDefaultMaterial(lMaterial->GetName());

		if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{			
			FbxSurfacePhong* phongMaterial =(FbxSurfacePhong *)lMaterial;

			FbxPropertyT<FbxDouble> lKFbxDouble1 = phongMaterial->TransparencyFactor;
			float fbxMatAlpha =  1.0f -(float)lKFbxDouble1.Get();

			lKFbxDouble1 =phongMaterial->Shininess;
			float fbxMatShininess =(float)lKFbxDouble1.Get();
	
			FbxPropertyT<FbxDouble3> lKFbxDouble3 = phongMaterial->Ambient;
			mat->m_ambientColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2]);
	
			lKFbxDouble3 = phongMaterial->Diffuse;
			mat->m_diffuseColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2], fbxMatAlpha);
	
			lKFbxDouble3 = phongMaterial->Specular;
			mat->m_specularColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2], fbxMatShininess);
	
			lKFbxDouble3 = phongMaterial->Emissive;
			mat->m_emissiveColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2]);		

			mat->m_specularExponent = fbxMatShininess;
			mat->m_specularMultiplier =(hkReal)phongMaterial->SpecularFactor.Get();
		}
		else if (lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			FbxSurfaceLambert* lamberMaterial =(FbxSurfaceLambert *)lMaterial;

			FbxPropertyT<FbxDouble> lKFbxDouble1 = lamberMaterial->TransparencyFactor;
			float fbxMatAlpha = (float)lKFbxDouble1.Get();

			FbxPropertyT<FbxDouble3> lKFbxDouble3 =lamberMaterial->Ambient;
			mat->m_ambientColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2]);

			lKFbxDouble3 =lamberMaterial->Diffuse;
			mat->m_diffuseColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2], fbxMatAlpha);

			lKFbxDouble3 =lamberMaterial->Emissive;
			mat->m_emissiveColor.set((float)lKFbxDouble3.Get()[0],(float)lKFbxDouble3.Get()[1],(float)lKFbxDouble3.Get()[2]);
		}
		else
		{
			// TODO: create from shaders
			HK_WARN(0x0, "Material \"" << mat->m_name << "\" is of an unsupported type. Expecting Phong or Lambert.");
			lMaterial = 0;
		}

		// Extract texture stage info
		if (lMaterial)
		{
			//Get all UV set names from the mesh
			FbxStringList lUVSetNameList;
			pMesh->GetUVSetNames(lUVSetNameList);

			convertTextures(scene, lMaterial, lUVSetNameList, mat);
		}
	}
	return mat;
}