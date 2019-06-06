/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DS_RENDER_H
#define QT3DS_RENDER_H

namespace qt3ds {
class NVAllocatorCallback;
class NVFoundationBase;
namespace foundation {
    class CRegisteredString;
    class IStringTable;
    class CStrTableOrDataRef;
    struct SStrRemapMap;
    struct SWriteBuffer;
    struct SDataReader;
    struct SPtrOffsetMap;
    class IPerfTimer;
    class Qt3DSString;
    class Qt3DSStringUtils;
}
namespace intrinsics {
}
namespace render {
    class NVRenderTexture2D;
    class NVRenderTexture2DArray;
    class NVRenderTextureCube;
    class NVRenderImage2D;
    class NVRenderFrameBuffer;
    class NVRenderRenderBuffer;
    class NVRenderVertexBuffer;
    class NVRenderIndexBuffer;
    class NVRenderDrawIndirectBuffer;
    class NVRenderInputAssembler;
    class NVRenderAttribLayout;
    class NVRenderDepthStencilState;
    class NVRenderContext;
    class NVRenderConstantBuffer;
    class NVRenderShaderProgram;
    class NVRenderShaderConstantBase;
    struct NVRenderDrawMode;
    struct NVRenderWinding;
    struct NVRenderSrcBlendFunc;
    struct NVRenderBlendEquation;
    struct NVRenderState;
    struct NVRenderTextureCoordOp;
    struct NVRenderTextureMagnifyingOp;
    struct NVRenderDstBlendFunc;
    struct NVRenderContextValues;
    struct NVRenderClearValues;
    struct NVRenderRect;
    struct NVRenderRectF;
    struct NVRenderRenderBufferFormats;
    struct NVRenderTextureFormats;
    struct NVRenderTextureSwizzleMode;
    struct NVRenderFrameBufferAttachments;
    struct NVRenderRect;
    struct NVRenderTextureCubeFaces;
    struct STextureDetails;
    struct NVRenderShaderDataTypes;
    class NVRenderTextureOrRenderBuffer;
    struct NVRenderTextureMinifyingOp;
    struct NVReadFaces;
    struct NVRenderVertexBufferEntry;
    struct NVRenderPtrPtrMap;
    class NVRenderComputeShader;
    class NVRenderAttribLayout;
    struct NVRenderBufferAccessTypeValues;
    struct NVRenderImageAccessType;
    struct NVRenderBufferBindValues;
    struct DrawArraysIndirectCommand;
    struct NVRenderShaderTypeValue;
    class NVRenderPathRender;
    class NVRenderPathSpecification;
    class NVRenderPathFontSpecification;
    class NVRenderPathFontItem;
    struct NVRenderTextureTypeValue;
    class NVRenderProgramPipeline;
}
class NVPlane;
}

namespace eastl {
}

namespace qt3ds {

namespace render {
    using namespace qt3ds;
    using namespace qt3ds::foundation;
    using namespace qt3ds::intrinsics;
    using qt3ds::render::NVRenderTexture2D;
    using qt3ds::render::NVRenderTexture2DArray;
    using qt3ds::render::NVRenderTextureCube;
    using qt3ds::render::NVRenderImage2D;
    using qt3ds::render::NVRenderFrameBuffer;
    using qt3ds::render::NVRenderRenderBuffer;
    using qt3ds::render::NVRenderVertexBuffer;
    using qt3ds::render::NVRenderIndexBuffer;
    using qt3ds::render::NVRenderDrawIndirectBuffer;
    using qt3ds::render::NVRenderInputAssembler;
    using qt3ds::render::NVRenderAttribLayout;
    using qt3ds::render::NVRenderDepthStencilState;
    using qt3ds::render::NVRenderContext;
    using qt3ds::render::NVRenderConstantBuffer;
    using qt3ds::render::NVRenderShaderProgram;
    using qt3ds::render::NVRenderShaderConstantBase;
    using qt3ds::render::NVRenderDrawMode;
    using qt3ds::render::NVRenderWinding;
    using qt3ds::foundation::CRegisteredString;
    using qt3ds::foundation::IStringTable;
    using qt3ds::render::NVRenderSrcBlendFunc;
    using qt3ds::render::NVRenderBlendEquation;
    using qt3ds::render::NVRenderState;
    using qt3ds::foundation::IStringTable;
    using qt3ds::foundation::CRegisteredString;
    using qt3ds::render::NVRenderTextureCoordOp;
    using qt3ds::render::NVRenderDstBlendFunc;
    using qt3ds::render::NVRenderRect;
    using qt3ds::render::NVRenderRectF;
    using qt3ds::render::NVRenderRenderBufferFormats;
    using qt3ds::render::NVRenderTextureFormats;
    using qt3ds::render::NVRenderTextureSwizzleMode;
    using qt3ds::render::NVRenderFrameBufferAttachments;
    using qt3ds::render::NVRenderRect;
    using qt3ds::render::NVRenderContextValues;
    using qt3ds::render::NVRenderClearValues;
    using qt3ds::render::STextureDetails;
    using qt3ds::render::NVRenderShaderDataTypes;
    using qt3ds::render::NVRenderTextureMagnifyingOp;
    using qt3ds::render::NVRenderTextureOrRenderBuffer;
    using qt3ds::render::NVRenderTextureMinifyingOp;
    using qt3ds::render::NVReadFaces;
    using qt3ds::render::NVRenderTextureCubeFaces;
    using qt3ds::foundation::SStrRemapMap;
    using qt3ds::foundation::SWriteBuffer;
    using qt3ds::foundation::SDataReader;
    using qt3ds::foundation::Qt3DSString;
    using qt3ds::foundation::Qt3DSStringUtils;
    using qt3ds::render::NVRenderPtrPtrMap;
    using qt3ds::foundation::CStrTableOrDataRef;
    using qt3ds::foundation::SPtrOffsetMap;
    using qt3ds::foundation::IPerfTimer;
    using qt3ds::render::NVRenderVertexBufferEntry;
    using qt3ds::render::NVRenderComputeShader;
    using qt3ds::render::NVRenderAttribLayout;
    using qt3ds::render::NVRenderBufferAccessTypeValues;
    using qt3ds::render::NVRenderImageAccessType;
    using qt3ds::render::NVRenderBufferBindValues;
    using qt3ds::render::DrawArraysIndirectCommand;
    using qt3ds::render::NVRenderShaderTypeValue;
    using qt3ds::render::NVRenderPathRender;
    using qt3ds::render::NVRenderPathSpecification;
    using qt3ds::render::NVRenderPathFontSpecification;
    using qt3ds::render::NVRenderPathFontItem;
    using qt3ds::render::NVRenderTextureTypeValue;
    using qt3ds::render::NVRenderProgramPipeline;

    class IQt3DSRenderContextCore;
    class IQt3DSRenderContext;
    class IQt3DSRenderer;
    class IBufferManager;
    struct SRenderMesh;
    class IRenderableObject;
    class IQt3DSRenderer;
    class IBufferManager;
    class IResourceManager;
    class IOffscreenRenderManager;
    struct SNode;
    struct SGraphObject;
    class ITextRenderer;
    class ITextRendererCore;
    class IInputStreamFactory;
    class IRefCountedInputStream;
    class IEffectSystem;
    class IEffectSystemCore;
    class IShaderCache;
    class IQt3DSRenderNodeFilter;
    class IRenderWidget;
    class IRenderWidgetContext;
    struct SShaderVertexCodeGenerator;
    struct SShaderFragmentCodeGenerator;
    class IThreadPool;
    struct SRenderMesh;
    struct SLoadedTexture;
    class IImageBatchLoader;
    class ITextTextureCache;
    class ITextTextureAtlas;
    class IRenderPluginInstance;
    class IRenderPluginClass;
    class IRenderPluginManager;
    class IRenderPluginManagerCore;
    struct SRenderPlugin;
    class IDynamicObjectSystemCore;
    class IDynamicObjectSystem;
    class IDynamicObjectClass;
    struct SRenderSubset;
    struct SModel;
    namespace dynamic {
        struct SPropertyDefinition;
    }
    struct SLight;
    struct SCamera;
    struct SCustomMaterial;
    class ICustomMaterialSystem;
    class ICustomMaterialSystemCore;
    struct SLayer;
    struct SReferencedMaterial;
    struct SPGGraphObject;
    class IPixelGraphicsRenderer;
    class IBufferLoader;
    struct SEffect;
    class IRenderList;
    class IRenderTask;
    class CResourceTexture2D;
    class IPathManagerCore;
    class IPathManager;
    struct SPath;
    struct SPathSubPath;
    class IShaderProgramGenerator;
    class IShaderStageGenerator;
    class IDefaultMaterialShaderGenerator;
    class ICustomMaterialShaderGenerator;
    struct SRenderableImage;
    class Qt3DSShadowMap;
    struct SLightmaps;
}
}

#endif
