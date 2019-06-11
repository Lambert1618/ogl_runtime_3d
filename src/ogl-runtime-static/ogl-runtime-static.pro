TEMPLATE = lib
TARGET = qt3dsruntimestatic
CONFIG += staticlib
include(../commoninclude.pri)

!boot2qt:!integrity:!ios {
    RESOURCES += ../../res.qrc
}

linux {
    DEFINES += _POSIX_C_SOURCE=199309L
    QMAKE_LFLAGS += -lrt
}

DEFINES += QT3DS_BUILDING_LIBRARY DISABLE_MESH_OPTIMIZATION

QT += qml
QT += quick-private

mingw {
    win32 {
    LIBS += \
        -lWs2_32 \
    }
    LIBS += \
        -lEASTL$$qtPlatformTargetSuffix() \
        -lQT3DSDM$$qtPlatformTargetSuffix() \
        -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()
}

# Foundation
SOURCES += \
    ../foundation/ConvertUTF.cpp \
    ../foundation/EASTL_new.cpp \
    ../foundation/FileTools.cpp \
    ../foundation/IOStreams.cpp \
    ../foundation/Qt3DSLogging.cpp \
    ../foundation/Qt3DSFoundation.cpp \
    ../foundation/Qt3DSMathUtils.cpp \
    ../foundation/Qt3DSPerfTimer.cpp \
    ../foundation/Qt3DSSystem.cpp \
    ../foundation/Socket.cpp \
    ../foundation/StringTable.cpp \
    ../foundation/XML.cpp \
    ../foundation/TrackingAllocator.cpp \
    ../runtimerender/q3dsqmlrender.cpp \
    ../engine/Qt3DSRenderRuntimeBinding.cpp \
    ../engine/Qt3DSRenderRuntimeBindingImplRenderer.cpp \
    ../engine/Qt3DSRenderRuntimeBindingImplTranslation.cpp \
    ../engine/Qt3DSTegraInputEngine.cpp \
    ../runtime/Qt3DSActivationManager.cpp \
    ../runtime/Qt3DSAnimationSystem.cpp \
    ../runtime/Qt3DSApplication.cpp \
    ../runtime/Qt3DSAttributeHashes.cpp \
    ../runtime/Qt3DSComponentManager.cpp \
    ../runtime/Qt3DSElementSystem.cpp \
    ../runtime/Qt3DSEventCallbacks.cpp \
    ../runtime/Qt3DSInputEngine.cpp \
    ../runtime/Qt3DSLogicSystem.cpp \
    ../runtime/Qt3DSCommandHelper.cpp \
    ../runtime/Qt3DSElementHelper.cpp \
    ../runtime/Qt3DSOutputMemoryStream.cpp \
    ../runtime/Qt3DSParametersSystem.cpp \
    ../runtime/Qt3DSPresentation.cpp \
    ../runtime/Qt3DSPresentationFrameData.cpp \
    ../runtime/Qt3DSQmlElementHelper.cpp \
    ../runtime/Qt3DSQmlEngine.cpp \
    ../runtime/Qt3DSSlideSystem.cpp \
    ../runtime/Qt3DSTimePolicy.cpp \
    ../runtime/q3dsvariantconfig.cpp \
    ../runtime/q3dsmaterialdefinitionparser.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderCamera.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderDefaultMaterial.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderDynamicObject.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderEffect.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderImage.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderLayer.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderLight.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderLightmaps.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderModel.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderNode.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderPath.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderPresentation.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderScene.cpp \
    ../runtimerender/graphobjects/Qt3DSRenderText.cpp \
    ../runtimerender/rendererimpl/Qt3DSRenderableObjects.cpp \
    ../runtimerender/rendererimpl/Qt3DSRendererImpl.cpp \
    ../runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderData.cpp \
    ../runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderHelper.cpp \
    ../runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderPreparationData.cpp \
    ../runtimerender/rendererimpl/Qt3DSRendererImplShaders.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderBufferLoader.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderBufferManager.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderImageBatchLoader.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTexture.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureBMP.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureDDS.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureGIF.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureHDR.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderPrefilterTexture.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderResourceBufferObjects.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderResourceManager.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderResourceTexture2D.cpp \
    ../runtimerender/Qt3DSOffscreenRenderManager.cpp \
    ../runtimerender/Qt3DSOldNBustedRenderPlugin.cpp \
    ../runtimerender/Qt3DSOnscreenTextRenderer.cpp \
    ../runtimerender/Qt3DSQtTextRenderer.cpp \
    ../runtimerender/Qt3DSRenderClippingFrustum.cpp \
    ../runtimerender/Qt3DSRenderCustomMaterialShaderGenerator.cpp \
    ../runtimerender/Qt3DSRenderCustomMaterialSystem.cpp \
    ../runtimerender/Qt3DSRenderDefaultMaterialShaderGenerator.cpp \
    ../runtimerender/Qt3DSRenderDynamicObjectSystem.cpp \
    ../runtimerender/Qt3DSRenderEffectSystem.cpp \
    ../runtimerender/Qt3DSRendererUtil.cpp \
    ../runtimerender/Qt3DSRenderEulerAngles.cpp \
    ../runtimerender/Qt3DSRenderGpuProfiler.cpp \
    ../runtimerender/Qt3DSRenderGraphObjectSerializer.cpp \
    ../runtimerender/Qt3DSRenderImageScaler.cpp \
    ../runtimerender/Qt3DSRenderInputStreamFactory.cpp \
    ../runtimerender/Qt3DSRenderPathManager.cpp \
    ../runtimerender/Qt3DSRenderPixelGraphicsRenderer.cpp \
    ../runtimerender/Qt3DSRenderPixelGraphicsTypes.cpp \
    ../runtimerender/Qt3DSRenderPlugin.cpp \
    ../runtimerender/Qt3DSRenderRay.cpp \
    ../runtimerender/Qt3DSRenderRenderList.cpp \
    ../runtimerender/Qt3DSRenderShaderCache.cpp \
    ../runtimerender/Qt3DSRenderShaderCodeGenerator.cpp \
    ../runtimerender/Qt3DSRenderShaderCodeGeneratorV2.cpp \
    ../runtimerender/Qt3DSRenderShadowMap.cpp \
    ../runtimerender/Qt3DSRenderSubpresentation.cpp \
    ../runtimerender/Qt3DSRenderTextTextureAtlas.cpp \
    ../runtimerender/Qt3DSRenderTextTextureCache.cpp \
    ../runtimerender/Qt3DSRenderTextureAtlas.cpp \
    ../runtimerender/Qt3DSRenderThreadPool.cpp \
    ../runtimerender/Qt3DSRenderUIPLoader.cpp \
    ../runtimerender/Qt3DSRenderUIPSharedTranslation.cpp \
    ../runtimerender/Qt3DSRenderWidgets.cpp \
    ../runtimerender/Qt3DSTextRenderer.cpp \
    ../system/Qt3DSAssert.cpp \
    ../system/Qt3DSBoundingBox.cpp \
    ../system/Qt3DSColor.cpp \
    ../system/Qt3DSCubicRoots.cpp \
    ../system/Qt3DSDataLogger.cpp \
    ../system/Qt3DSDLLManager.cpp \
    ../system/Qt3DSEulerAngles.cpp \
    ../system/Qt3DSFile.cpp \
    ../system/Qt3DSFileStream.cpp \
    ../system/Qt3DSFunctionWrappers.cpp \
    ../system/Qt3DSMatrix.cpp \
    ../system/Qt3DSMemory.cpp \
    ../system/Qt3DSMemoryFilter.cpp \
    ../system/Qt3DSMemoryHeap.cpp \
    ../system/Qt3DSMemoryManager.cpp \
    ../system/Qt3DSMemoryPool.cpp \
    ../system/Qt3DSMemoryProbe.cpp \
    ../system/Qt3DSMemoryStatistics.cpp \
    ../system/Qt3DSMemoryTracker.cpp \
    ../system/Qt3DSTimer.cpp \
    ../system/Qt3DSTypes.cpp \
    ../system/Qt3DSVector3.cpp \
    ../uipparser/Qt3DSUIPParserActionHelper.cpp \
    ../uipparser/Qt3DSUIPParserImpl.cpp \
    ../uipparser/Qt3DSUIPParserObjectRefHelper.cpp \
    ../runtimerender/Qt3DSRenderContextCore.cpp \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureKTX.cpp \
    ../runtimerender/Qt3DSDistanceFieldRenderer.cpp \
    ../runtimerender/Qt3DSFontDatabase.cpp \
    ../runtimerender/Qt3DSDistanceFieldGlyphCacheManager.cpp \
    ../runtimerender/Qt3DSDistanceFieldGlyphCache.cpp \
    ../engine/Qt3DSRuntimeView.cpp

HEADERS += \
    ../foundation/ConvertUTF.h \
    ../foundation/FileTools.h \
    ../foundation/StringTools.h \
    ../foundation/IOStreams.h \
    ../foundation/Qt3DSLogging.h \
    ../foundation/Qt3DSFoundation.h \
    ../foundation/Qt3DSMathUtils.h \
    ../foundation/Qt3DSPerfTimer.h \
    ../foundation/Qt3DSSystem.h \
    ../foundation/Socket.h \
    ../foundation/StringTable.h \
    ../foundation/XML.h \
    ../foundation/AutoDeallocatorAllocator.h \
    ../foundation/FastAllocator.h \
    ../foundation/PoolingAllocator.h \
    ../foundation/PreAllocatedAllocator.h \
    ../foundation/Qt3DS.h \
    ../foundation/Qt3DSAllocator.h \
    ../foundation/Qt3DSAllocatorCallback.h \
    ../foundation/Qt3DSAssert.h \
    ../foundation/Qt3DSAtomic.h \
    ../foundation/Qt3DSBasicTemplates.h \
    ../foundation/Qt3DSBounds3.h \
    ../foundation/Qt3DSBroadcastingAllocator.h \
    ../foundation/Qt3DSContainers.h \
    ../foundation/Qt3DSDataRef.h \
    ../foundation/Qt3DSDiscriminatedUnion.h \
    ../foundation/Qt3DSFastIPC.h \
    ../foundation/Qt3DSFlags.h \
    ../foundation/Qt3DSFPU.h \
    ../foundation/Qt3DSIndexableLinkedList.h \
    ../foundation/Qt3DSIntrinsics.h \
    ../foundation/Qt3DSInvasiveLinkedList.h \
    ../foundation/Qt3DSInvasiveSet.h \
    ../foundation/Qt3DSIPC.h \
    ../foundation/Qt3DSMat33.h \
    ../foundation/Qt3DSMat44.h \
    ../foundation/Qt3DSMath.h \
    ../foundation/Qt3DSMemoryBuffer.h \
    ../foundation/Qt3DSMutex.h \
    ../foundation/Qt3DSNoCopy.h \
    ../foundation/Qt3DSOption.h \
    ../foundation/Qt3DSPlane.h \
    ../foundation/Qt3DSPool.h \
    ../foundation/Qt3DSPreprocessor.h \
    ../foundation/Qt3DSQuat.h \
    ../foundation/Qt3DSRefCounted.h \
    ../foundation/Qt3DSSemaphore.h \
    ../foundation/Qt3DSSimpleTypes.h \
    ../foundation/Qt3DSStringTokenizer.h \
    ../foundation/Qt3DSSync.h \
    ../foundation/Qt3DSThread.h \
    ../foundation/Qt3DSTime.h \
    ../foundation/Qt3DSTransform.h \
    ../foundation/Qt3DSUnionCast.h \
    ../foundation/Qt3DSUtilities.h \
    ../foundation/Qt3DSVec2.h \
    ../foundation/Qt3DSVec3.h \
    ../foundation/Qt3DSVec4.h \
    ../foundation/Qt3DSVersionNumber.h \
    ../foundation/SerializationTypes.h \
    ../foundation/StrConvertUTF.h \
    ../foundation/StringConversion.h \
    ../foundation/StringConversionImpl.h \
    ../foundation/TaggedPointer.h \
    ../foundation/ThreadSafeQueue.h \
    ../foundation/TrackingAllocator.h \
    ../foundation/Utils.h \
    ../runtimerender/q3dsqmlrender.h \
    ../engine/Qt3DSRenderRuntimeBinding.h \
    ../engine/Qt3DSRenderRuntimeBindingImpl.h \
    ../engine/Qt3DSTegraInputEngine.h \
    ../runtime/Qt3DSActivationManager.h \
    ../runtime/Qt3DSAnimationSystem.h \
    ../runtime/Qt3DSApplication.h \
    ../runtime/Qt3DSAttributeHashes.h \
    ../runtime/Qt3DSComponentManager.h \
    ../runtime/Qt3DSElementSystem.h \
    ../runtime/Qt3DSEventCallbacks.h \
    ../runtime/Qt3DSInputEngine.h \
    ../runtime/Qt3DSLogicSystem.h \
    ../runtime/Qt3DSCommandHelper.h \
    ../runtime/Qt3DSElementHelper.h \
    ../runtime/Qt3DSOutputMemoryStream.h \
    ../runtime/Qt3DSParametersSystem.h \
    ../runtime/Qt3DSPresentation.h \
    ../runtime/Qt3DSPresentationFrameData.h \
    ../runtime/Qt3DSQmlElementHelper.h \
    ../runtime/Qt3DSQmlEngine.h \
    ../runtime/Qt3DSSlideSystem.h \
    ../runtime/Qt3DSTimePolicy.h \
    ../runtime/Qt3DSApplicationValues.h \
    ../runtime/Qt3DSIComponentManager.h \
    ../runtime/Qt3DSIInputSystem.h \
    ../runtime/Qt3DSInputDefs.h \
    ../runtime/Qt3DSInputEventTypes.h \
    ../runtime/Qt3DSIStateful.h \
    ../runtime/Qt3DSIText.h \
    ../runtime/Qt3DSKernelTypes.h \
    ../runtime/q3dsvariantconfig_p.h \
    ../runtime/q3dsmaterialdefinitionparser.h \
    ../runtimerender/graphobjects/Qt3DSRenderCamera.h \
    ../runtimerender/graphobjects/Qt3DSRenderCustomMaterial.h \
    ../runtimerender/graphobjects/Qt3DSRenderDefaultMaterial.h \
    ../runtimerender/graphobjects/Qt3DSRenderDynamicObject.h \
    ../runtimerender/graphobjects/Qt3DSRenderEffect.h \
    ../runtimerender/graphobjects/Qt3DSRenderGraphObject.h \
    ../runtimerender/graphobjects/Qt3DSRenderImage.h \
    ../runtimerender/graphobjects/Qt3DSRenderLayer.h \
    ../runtimerender/graphobjects/Qt3DSRenderLight.h \
    ../runtimerender/graphobjects/Qt3DSRenderLightmaps.h \
    ../runtimerender/graphobjects/Qt3DSRenderMaterialDirty.h \
    ../runtimerender/graphobjects/Qt3DSRenderModel.h \
    ../runtimerender/graphobjects/Qt3DSRenderNode.h \
    ../runtimerender/graphobjects/Qt3DSRenderPath.h \
    ../runtimerender/graphobjects/Qt3DSRenderPathSubPath.h \
    ../runtimerender/graphobjects/Qt3DSRenderPresentation.h \
    ../runtimerender/graphobjects/Qt3DSRenderReferencedMaterial.h \
    ../runtimerender/graphobjects/Qt3DSRenderScene.h \
    ../runtimerender/graphobjects/Qt3DSRenderText.h \
    ../runtimerender/Qt3DSOffscreenRenderKey.h \
    ../runtimerender/Qt3DSOffscreenRenderManager.h \
    ../runtimerender/Qt3DSOldNBustedRenderPlugin.h \
    ../runtimerender/Qt3DSRender.h \
    ../runtimerender/Qt3DSRenderableImage.h \
    ../runtimerender/Qt3DSRenderClippingFrustum.h \
    ../runtimerender/Qt3DSRenderCustomMaterialRenderContext.h \
    ../runtimerender/Qt3DSRenderCustomMaterialShaderGenerator.h \
    ../runtimerender/Qt3DSRenderCustomMaterialSystem.h \
    ../runtimerender/Qt3DSRenderDefaultMaterialShaderGenerator.h \
    ../runtimerender/Qt3DSRenderDynamicObjectSystem.h \
    ../runtimerender/Qt3DSRenderDynamicObjectSystemCommands.h \
    ../runtimerender/Qt3DSRenderDynamicObjectSystemUtil.h \
    ../runtimerender/Qt3DSRenderEffectSystem.h \
    ../runtimerender/Qt3DSRenderer.h \
    ../runtimerender/Qt3DSRendererUtil.h \
    ../runtimerender/Qt3DSRenderEulerAngles.h \
    ../runtimerender/Qt3DSRenderGraphObjectPickQuery.h \
    ../runtimerender/Qt3DSRenderGraphObjectSerializer.h \
    ../runtimerender/Qt3DSRenderGraphObjectTypes.h \
    ../runtimerender/Qt3DSRenderImageScaler.h \
    ../runtimerender/Qt3DSRenderImageTextureData.h \
    ../runtimerender/Qt3DSRenderInputStreamFactory.h \
    ../runtimerender/Qt3DSRenderMaterialHelpers.h \
    ../runtimerender/Qt3DSRenderMaterialShaderGenerator.h \
    ../runtimerender/Qt3DSRenderMesh.h \
    ../runtimerender/Qt3DSRenderPathManager.h \
    ../runtimerender/Qt3DSRenderPathMath.h \
    ../runtimerender/Qt3DSRenderPathRenderContext.h \
    ../runtimerender/Qt3DSRenderPixelGraphicsRenderer.h \
    ../runtimerender/Qt3DSRenderPixelGraphicsTypes.h \
    ../runtimerender/Qt3DSRenderPlugin.h \
    ../runtimerender/Qt3DSRenderPluginCInterface.h \
    ../runtimerender/Qt3DSRenderPluginGraphObject.h \
    ../runtimerender/Qt3DSRenderPluginPropertyValue.h \
    ../runtimerender/Qt3DSRenderProfiler.h \
    ../runtimerender/Qt3DSRenderRay.h \
    ../runtimerender/Qt3DSRenderRenderList.h \
    ../runtimerender/Qt3DSRenderRotationHelper.h \
    ../runtimerender/Qt3DSRenderShaderCache.h \
    ../runtimerender/Qt3DSRenderShaderCodeGenerator.h \
    ../runtimerender/Qt3DSRenderShaderCodeGeneratorV2.h \
    ../runtimerender/Qt3DSRenderShaderKeys.h \
    ../runtimerender/Qt3DSRenderShadowMap.h \
    ../runtimerender/Qt3DSRenderSubpresentation.h \
    ../runtimerender/Qt3DSRenderSubPresentationHelper.h \
    ../runtimerender/Qt3DSRenderTaggedPointer.h \
    ../runtimerender/Qt3DSRenderTessModeValues.h \
    ../runtimerender/Qt3DSRenderTextTextureAtlas.h \
    ../runtimerender/Qt3DSRenderTextTextureCache.h \
    ../runtimerender/Qt3DSRenderTextTypes.h \
    ../runtimerender/Qt3DSRenderTextureAtlas.h \
    ../runtimerender/Qt3DSRenderThreadPool.h \
    ../runtimerender/Qt3DSRenderUIPLoader.h \
    ../runtimerender/Qt3DSRenderUIPSharedTranslation.h \
    ../runtimerender/Qt3DSRenderWidgets.h \
    ../runtimerender/Qt3DSTextRenderer.h \
    ../runtimerender/rendererimpl/Qt3DSRenderableObjects.h \
    ../runtimerender/rendererimpl/Qt3DSRendererImpl.h \
    ../runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderData.h \
    ../runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderHelper.h \
    ../runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderPreparationData.h \
    ../runtimerender/rendererimpl/Qt3DSRendererImplShaders.h \
    ../runtimerender/rendererimpl/Qt3DSVertexPipelineImpl.h \
    ../runtimerender/resourcemanager/Qt3DSRenderBufferLoader.h \
    ../runtimerender/resourcemanager/Qt3DSRenderBufferManager.h \
    ../runtimerender/resourcemanager/Qt3DSRenderImageBatchLoader.h \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTexture.h \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureDDS.h \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureFreeImageCompat.h \
    ../runtimerender/resourcemanager/Qt3DSRenderPrefilterTexture.h \
    ../runtimerender/resourcemanager/Qt3DSRenderResourceBufferObjects.h \
    ../runtimerender/resourcemanager/Qt3DSRenderResourceManager.h \
    ../runtimerender/resourcemanager/Qt3DSRenderResourceTexture2D.h \
    ../system/Qt3DSArray.h \
    ../system/Qt3DSAssert.h \
    ../system/Qt3DSAudioPlayer.h \
    ../system/Qt3DSBasicPluginDLL.h \
    ../system/Qt3DSBezierEval.h \
    ../system/Qt3DSBoundingBox.h \
    ../system/Qt3DSCircularArray.h \
    ../system/Qt3DSColor.h \
    ../system/Qt3DSConfig.h \
    ../system/Qt3DSCubicRoots.h \
    ../system/Qt3DSCubicRootsImpl.h \
    ../system/Qt3DSDataLogger.h \
    ../system/Qt3DSDataLogger.hpp \
    ../system/Qt3DSDataLoggerEnums.h \
    ../system/Qt3DSDataLoggerViewer.h \
    ../system/Qt3DSDLLManager.h \
    ../system/Qt3DSEGLTimer.h \
    ../system/Qt3DSEndian.h \
    ../system/Qt3DSEulerAngles.h \
    ../system/Qt3DSFile.h \
    ../system/Qt3DSFileStream.h \
    ../system/Qt3DSFixedArray.h \
    ../system/Qt3DSFNDTimer.h \
    ../system/Qt3DSFunctionWrappers.h \
    ../system/Qt3DSHash.h \
    ../system/Qt3DSIFileStream.h \
    ../system/Qt3DSIStream.h \
    ../system/Qt3DSITimer.h \
    ../system/Qt3DSMacros.h \
    ../system/Qt3DSMatrix.h \
    ../system/Qt3DSMemory.h \
    ../system/Qt3DSMemoryFilter.h \
    ../system/Qt3DSMemoryHeap.h \
    ../system/Qt3DSMemoryManager.h \
    ../system/Qt3DSMemoryPool.h \
    ../system/Qt3DSMemoryProbe.h \
    ../system/Qt3DSMemorySettings.h \
    ../system/Qt3DSMemoryStatistics.h \
    ../system/Qt3DSMemoryTracker.h \
    ../system/Qt3DSPlatformSpecific.h \
    ../system/Qt3DSTimer.h \
    ../system/Qt3DSTypes.h \
    ../system/Qt3DSVector3.h \
    ../system/Qt3DSArray.inl \
    ../system/Qt3DSCircularArray.inl \
    ../system/Qt3DSFixedArray.inl \
    ../state/Qt3DSState.h \
    ../state/Qt3DSStateTypes.h \
    ../state/Qt3DSStateScriptContext.h \
    ../state/Qt3DSStateVisualBindingContextCommands.h \
    ../uipparser/Qt3DSIPresentation.h \
    ../uipparser/Qt3DSUIPParser.h \
    ../uipparser/Qt3DSUIPParserActionHelper.h \
    ../uipparser/Qt3DSUIPParserImpl.h \
    ../uipparser/Qt3DSUIPParserObjectRefHelper.h \
    ../runtime/Qt3DSCommandEventTypes.h \
    ../runtime/Qt3DSEvent.h \
    ../runtime/Qt3DSFrameworkTypes.h \
    ../runtime/Qt3DSInputFrame.h \
    ../runtime/Qt3DSIScene.h \
    ../runtime/Qt3DSIScriptBridge.h \
    ../runtime/Qt3DSPickFrame.h \
    ../runtime/Qt3DSRuntimeFactory.h \
    ../runtime/Qt3DSSceneManager.h \
    ../engine/Qt3DSEGLInfo.h \
    ../engine/Qt3DSEGLWindowSystem.h \
    ../engine/Qt3DSPluginDLL.h \
    ../engine/Qt3DSWindowSystem.h \
    ../runtimerender/Qt3DSRenderContextCore.h \
    ../runtimerender/Qt3DSRenderLightConstantProperties.h \
    ../runtimerender/resourcemanager/Qt3DSRenderLoadedTextureKTX.h \
    ../runtimerender/Qt3DSDistanceFieldRenderer.h \
    ../runtimerender/Qt3DSFontDatabase_p.h \
    ../runtimerender/Qt3DSDistanceFieldGlyphCacheManager_p.h \
    ../runtimerender/Qt3DSDistanceFieldGlyphCache_p.h \
    ../engine/Qt3DSRuntimeView.h

win32 {
SOURCES += \
    ../foundation/windows/Qt3DSWindowsAtomic.cpp \
    ../foundation/windows/Qt3DSWindowsFPU.cpp \
    ../foundation/windows/Qt3DSWindowsMutex.cpp \
    ../foundation/windows/Qt3DSWindowsSemaphore.cpp \
    ../foundation/windows/Qt3DSWindowsSync.cpp \
    ../foundation/windows/Qt3DSWindowsThread.cpp \
    ../foundation/windows/Qt3DSWindowsTime.cpp

HEADERS += \
    ../foundation/windows/Qt3DSWindowsAoS.h \
    ../foundation/windows/Qt3DSWindowsFile.h \
    ../foundation/windows/Qt3DSWindowsInclude.h \
    ../foundation/windows/Qt3DSWindowsInlineAoS.h \
    ../foundation/windows/Qt3DSWindowsIntrinsics.h \
    ../foundation/windows/Qt3DSWindowsString.h \
    ../foundation/windows/Qt3DSWindowsTrigConstants.h
}

macos: SOURCES += \
    ../foundation/macos/Qt3DSUnixAtomic.cpp \
    ../foundation/macos/Qt3DSUnixFPU.cpp \
    ../foundation/macos/Qt3DSUnixMutex.cpp \
    ../foundation/macos/Qt3DSUnixSemaphore.cpp \
    ../foundation/macos/Qt3DSUnixSync.cpp \
    ../foundation/linux/Qt3DSLinuxThread.cpp \
    ../foundation/macos/Qt3DSUnixTime.cpp

linux|integrity|qnx {
SOURCES += \
    ../foundation/linux/Qt3DSLinuxAtomic.cpp \
    ../foundation/linux/Qt3DSLinuxFPU.cpp \
    ../foundation/linux/Qt3DSLinuxMutex.cpp \
    ../foundation/linux/Qt3DSLinuxSemaphore.cpp \
    ../foundation/linux/Qt3DSLinuxSync.cpp \
    ../foundation/linux/Qt3DSLinuxThread.cpp \
    ../foundation/linux/Qt3DSLinuxTime.cpp

HEADERS += \
    ../foundation/linux/Qt3DSLinuxAoS.h \
    ../foundation/linux/Qt3DSLinuxFile.h \
    ../foundation/linux/Qt3DSLinuxInlineAoS.h \
    ../foundation/linux/Qt3DSLinuxIntrinsics.h \
    ../foundation/linux/Qt3DSLinuxString.h \
    ../foundation/linux/Qt3DSLinuxTrigConstants.h
}

# Libs
SOURCES += \
    ../platformspecific/$$PlatformSpecificDir/libs/nv_math/nv_math.cpp \
    ../platformspecific/$$PlatformSpecificDir/libs/nv_math/nv_matrix.cpp \
    ../platformspecific/$$PlatformSpecificDir/libs/nv_math/nv_quat.cpp

# RenderBase
SOURCES += \
    ../render/Qt3DSRenderAtomicCounterBuffer.cpp \
    ../render/Qt3DSRenderAttribLayout.cpp \
    ../render/Qt3DSRenderBaseTypes.cpp \
    ../render/Qt3DSRenderComputeShader.cpp \
    ../render/Qt3DSRenderConstantBuffer.cpp \
    ../render/Qt3DSRenderContext.cpp \
    ../render/Qt3DSRenderDataBuffer.cpp \
    ../render/Qt3DSRenderDepthStencilState.cpp \
    ../render/Qt3DSRenderDrawIndirectBuffer.cpp \
    ../render/Qt3DSRenderFragmentShader.cpp \
    ../render/Qt3DSRenderFrameBuffer.cpp \
    ../render/Qt3DSRenderGeometryShader.cpp \
    ../render/Qt3DSRenderImageTexture.cpp \
    ../render/Qt3DSRenderIndexBuffer.cpp \
    ../render/Qt3DSRenderInputAssembler.cpp \
    ../render/Qt3DSRenderOcclusionQuery.cpp \
    ../render/Qt3DSRenderPathFontSpecification.cpp \
    ../render/Qt3DSRenderPathFontText.cpp \
    ../render/Qt3DSRenderPathRender.cpp \
    ../render/Qt3DSRenderPathSpecification.cpp \
    ../render/Qt3DSRenderProgramPipeline.cpp \
    ../render/Qt3DSRenderQueryBase.cpp \
    ../render/Qt3DSRenderRasterizerState.cpp \
    ../render/Qt3DSRenderRenderBuffer.cpp \
    ../render/Qt3DSRenderSampler.cpp \
    ../render/Qt3DSRenderShaderProgram.cpp \
    ../render/Qt3DSRenderStorageBuffer.cpp \
    ../render/Qt3DSRenderSync.cpp \
    ../render/Qt3DSRenderTessellationShader.cpp \
    ../render/Qt3DSRenderTexture2D.cpp \
    ../render/Qt3DSRenderTexture2DArray.cpp \
    ../render/Qt3DSRenderTextureBase.cpp \
    ../render/Qt3DSRenderTextureCube.cpp \
    ../render/Qt3DSRenderTimerQuery.cpp \
    ../render/Qt3DSRenderVertexBuffer.cpp \
    ../render/Qt3DSRenderVertexShader.cpp

HEADERS += \
    ../render/Qt3DSRenderAtomicCounterBuffer.h \
    ../render/Qt3DSRenderAttribLayout.h \
    ../render/Qt3DSRenderBaseTypes.h \
    ../render/Qt3DSRenderComputeShader.h \
    ../render/Qt3DSRenderConstantBuffer.h \
    ../render/Qt3DSRenderContext.h \
    ../render/Qt3DSRenderDataBuffer.h \
    ../render/Qt3DSRenderDepthStencilState.h \
    ../render/Qt3DSRenderDrawable.h \
    ../render/Qt3DSRenderDrawIndirectBuffer.h \
    ../render/Qt3DSRenderFragmentShader.h \
    ../render/Qt3DSRenderFrameBuffer.h \
    ../render/Qt3DSRenderGeometryShader.h \
    ../render/Qt3DSRenderImageTexture.h \
    ../render/Qt3DSRenderIndexBuffer.h \
    ../render/Qt3DSRenderInputAssembler.h \
    ../render/Qt3DSRenderOcclusionQuery.h \
    ../render/Qt3DSRenderPathFontSpecification.h \
    ../render/Qt3DSRenderPathFontText.h \
    ../render/Qt3DSRenderPathRender.h \
    ../render/Qt3DSRenderPathSpecification.h \
    ../render/Qt3DSRenderProgramPipeline.h \
    ../render/Qt3DSRenderQueryBase.h \
    ../render/Qt3DSRenderRasterizerState.h \
    ../render/Qt3DSRenderRenderBuffer.h \
    ../render/Qt3DSRenderSampler.h \
    ../render/Qt3DSRenderShader.h \
    ../render/Qt3DSRenderShaderConstant.h \
    ../render/Qt3DSRenderShaderProgram.h \
    ../render/Qt3DSRenderStorageBuffer.h \
    ../render/Qt3DSRenderSync.h \
    ../render/Qt3DSRenderTessellationShader.h \
    ../render/Qt3DSRenderTexture2D.h \
    ../render/Qt3DSRenderTexture2DArray.h \
    ../render/Qt3DSRenderTextureBase.h \
    ../render/Qt3DSRenderTextureCube.h \
    ../render/Qt3DSRenderTimerQuery.h \
    ../render/Qt3DSRenderVertexBuffer.h \
    ../render/Qt3DSRenderVertexShader.h \
    ../render/glg/Qt3DSGLImplObjects.h

# Render
SOURCES += \
    ../render/backends/gl/Qt3DSOpenGLExtensions.cpp \
    ../render/backends/gl/Qt3DSRenderBackendGL3.cpp \
    ../render/backends/gl/Qt3DSRenderBackendGL4.cpp \
    ../render/backends/gl/Qt3DSRenderBackendGLBase.cpp \
    ../render/backends/gl/Qt3DSRenderContextGL.cpp \
    ../render/backends/software/Qt3DSRenderBackendNULL.cpp \
    ../render/backends/gl/Q3DSRenderBackendGLES2.cpp

HEADERS += \
    ../render/backends/Qt3DSRenderBackend.h \
    ../render/backends/gl/Qt3DSOpenGLPrefix.h \
    ../render/backends/gl/Qt3DSOpenGLUtil.h \
    ../render/backends/gl/Qt3DSOpenGLExtensions.h \
    ../render/backends/gl/Qt3DSRenderBackendGL3.h \
    ../render/backends/gl/Qt3DSRenderBackendGL4.h \
    ../render/backends/gl/Qt3DSRenderBackendGLBase.h \
    ../render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h \
    ../render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h \
    ../render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h \
    ../render/backends/software/Qt3DSRenderBackendNULL.h \
    ../render/backends/gl/Q3DSRenderBackendGLES2.h

# DataModel
SOURCES += \
    ../datamodel/Qt3DSMetadata.cpp \
    ../importlib/Qt3DSImportMesh.cpp \
    ../importlib/Qt3DSImportMeshBuilder.cpp \
    ../importlib/Qt3DSImportPath.cpp \
    ../dm/systems/Qt3DSDMMetaData.cpp \
    ../dm/systems/Qt3DSDMXML.cpp \
    ../dm/systems/Qt3DSDMStringTable.cpp \
    ../dm/systems/Qt3DSDMComposerTypeDefinitions.cpp \
    ../dm/systems/Qt3DSDMValue.cpp \
    ../dm/systems/cores/SimpleDataCore.cpp

HEADERS += \
    ../datamodel/Qt3DSMetadata.h \
    ../datamodel/DocumentResourceManagerScriptParser.h \
    ../importlib/Qt3DSImportMesh.h \
    ../importlib/Qt3DSImportPath.h \
    ../dm/systems/Qt3DSDMMetaData.h \
    ../dm/systems/Qt3DSDMXML.h \
    ../dm/systems/Qt3DSDMStringTable.h \
    ../dm/systems/Qt3DSDMHandles.h \
    ../dm/systems/Qt3DSDMComposerTypeDefinitions.h \
    ../dm/systems/Qt3DSDMValue.h \
    ../dm/systems/cores/SimpleDataCore.h

# Engine
HEADERS += \
    ../engine/EnginePrefix.h

# Event
SOURCES += \
    ../event/EventFactory.cpp \
    ../event/EventPoller.cpp \
    ../event/EventSystemC.cpp

HEADERS += \
    ../event/EventPollingSystem.h \
    ../event/EventSystem.h \
    ../event/EventSystemC.h

# Render
HEADERS += \
    ../runtimerender/android/DynamicLibLoader.h \
    ../runtimerender/linux/DynamicLibLoader.h \
    ../runtimerender/macos/DynamicLibLoader.h \
    ../runtimerender/qnx/DynamicLibLoader.h \
    ../runtimerender/windows/DynamicLibLoader.h

# Runtime
HEADERS += \
    ../runtime/RuntimePrefix.h \
    ../runtime/q3dsqmlscript.h \
    ../runtime/q3dsqmlbehavior.h

SOURCES += \
    ../runtime/q3dsqmlscript.cpp \
    ../runtime/q3dsqmlbehavior.cpp

# System
HEADERS += \
    ../system/SystemPrefix.h

DISTFILES += \
    ../runtime/Qt3DSAttributeHashes.txt
