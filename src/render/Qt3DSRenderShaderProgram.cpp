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

#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderVertexShader.h"
#include "render/Qt3DSRenderFragmentShader.h"
#include "render/Qt3DSRenderTessellationShader.h"
#include "render/Qt3DSRenderGeometryShader.h"
#include "render/Qt3DSRenderComputeShader.h"
#include "render/Qt3DSRenderImageTexture.h"

namespace qt3ds {
namespace render {

    template <typename TDataType>
    struct ShaderConstantApplier
    {
        bool force_compile_error;
    };

    template <>
    struct ShaderConstantApplier<QT3DSI32>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSI32 &inValue,
                           QT3DSI32 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSI32_2>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSI32_2 &inValue,
                           QT3DSI32_2 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue.x);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSI32_3>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSI32_3 &inValue,
                           QT3DSI32_3 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue.x);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSI32_4>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSI32_4 &inValue,
                           QT3DSI32_4 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue.x);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSRenderBool>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           const QT3DSRenderBool inValue, QT3DSRenderBool &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<bool_2>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const bool_2 &inValue,
                           bool_2 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<bool_3>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const bool_3 &inValue,
                           bool_3 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<bool_4>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const bool_4 &inValue,
                           bool_4 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSF32>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSF32 &inValue,
                           QT3DSF32 &oldValue)
        {
            if (count > 1 || !(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSVec2>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSVec2 &inValue,
                           QT3DSVec2 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSVec3>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSVec3 &inValue,
                           QT3DSVec3 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSVec4>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSVec4 &inValue,
                           QT3DSVec4 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSU32>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSU32 &inValue,
                           QT3DSU32 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSU32_2>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSU32_2 &inValue,
                           QT3DSU32_2 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue.x);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSU32_3>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSU32_3 &inValue,
                           QT3DSU32_3 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue.x);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSU32_4>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSU32_4 &inValue,
                           QT3DSU32_4 &oldValue)
        {
            if (!(inValue == oldValue)) {
                backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                          &inValue.x);
                oldValue = inValue;
            }
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSMat33>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSMat33 inValue,
                           QT3DSMat33 &, bool inTranspose)
        {
            backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                      inValue.front(), inTranspose);
        }
    };

    template <>
    struct ShaderConstantApplier<QT3DSMat44>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type, const QT3DSMat44 inValue,
                           QT3DSMat44 &, bool inTranspose)
        {
            backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                      inValue.front(), inTranspose);
        }

        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVConstDataRef<QT3DSMat44> inValue, QT3DSMat44 &, bool inTranspose)
        {
            backend->SetConstantValue(program->GetShaderProgramHandle(), location, type, count,
                                      reinterpret_cast<const GLfloat *>(inValue.begin()),
                                      inTranspose);
        }
    };

    template <>
    struct ShaderConstantApplier<NVRenderTexture2DPtr>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVRenderTexture2DPtr inValue, QT3DSU32 &oldValue)
        {
            if (inValue) {
                NVRenderTexture2D *texObj = reinterpret_cast<NVRenderTexture2D *>(inValue);
                texObj->Bind();
                QT3DSU32 texUnit = texObj->GetTextureUnit();
                if (texUnit != oldValue) {
                    backend->SetConstantValue(program->GetShaderProgramHandle(), location, type,
                                              count, &texUnit);
                    oldValue = texUnit;
                }
            }
        }
    };

    template <>
    struct ShaderConstantApplier<NVRenderTexture2DHandle>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend,
                           QT3DSI32 location, QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVRenderTexture2DHandle inValue, QVector<QT3DSU32> &oldValue)
        {
            Q_UNUSED(type)
            if (inValue) {
                bool update = false;
                for (int i = 0; i < count; i++) {
                    NVRenderTexture2D *texObj = reinterpret_cast<NVRenderTexture2D *>(inValue[i]);
                    QT3DSU32 texUnit = QT3DS_MAX_U32;
                    if (texObj) {
                        texObj->Bind();
                        texUnit = texObj->GetTextureUnit();
                    }
                    if (texUnit != oldValue[i]) {
                        update = true;
                        oldValue[i] = texUnit;
                    }
                }
                if (update)
                    backend->SetConstantValue(program->GetShaderProgramHandle(), location,
                                              NVRenderShaderDataTypes::NVRenderTexture2DPtr,
                                              count, oldValue.data());
            }
        }
    };

    template <>
    struct ShaderConstantApplier<NVRenderTexture2DArrayPtr>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVRenderTexture2DArrayPtr inValue, QT3DSU32 &oldValue)
        {
            if (inValue) {
                NVRenderTexture2DArray *texObj =
                    reinterpret_cast<NVRenderTexture2DArray *>(inValue);
                texObj->Bind();
                QT3DSU32 texUnit = texObj->GetTextureUnit();
                if (texUnit != oldValue) {
                    backend->SetConstantValue(program->GetShaderProgramHandle(), location, type,
                                              count, &texUnit);
                    oldValue = texUnit;
                }
            }
        }
    };

    template <>
    struct ShaderConstantApplier<NVRenderTextureCubePtr>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVRenderTextureCubePtr inValue, QT3DSU32 &oldValue)
        {
            if (inValue) {
                NVRenderTextureCube *texObj = reinterpret_cast<NVRenderTextureCube *>(inValue);
                texObj->Bind();
                QT3DSU32 texUnit = texObj->GetTextureUnit();
                if (texUnit != oldValue) {
                    backend->SetConstantValue(program->GetShaderProgramHandle(), location, type,
                                              count, &texUnit);
                    oldValue = texUnit;
                }
            }
        }
    };

    template <>
    struct ShaderConstantApplier<NVRenderTextureCubeHandle>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend,
                           QT3DSI32 location, QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVRenderTextureCubeHandle inValue, QVector<QT3DSU32> &oldValue)
        {
            Q_UNUSED(type)
            if (inValue) {
                bool update = false;
                for (int i = 0; i < count; i++) {
                    NVRenderTextureCube *texObj = reinterpret_cast<NVRenderTextureCube *>(inValue[i]);
                    QT3DSU32 texUnit = QT3DS_MAX_U32;
                    if (texObj) {
                        texObj->Bind();
                        texUnit = texObj->GetTextureUnit();
                    }
                    if (texUnit != oldValue[i]) {
                        update = true;
                        oldValue[i] = texUnit;
                    }
                }
                if (update)
                    backend->SetConstantValue(program->GetShaderProgramHandle(), location,
                                              NVRenderShaderDataTypes::NVRenderTextureCubePtr,
                                              count, oldValue.data());
            }
        }
    };

    template <>
    struct ShaderConstantApplier<NVRenderImage2DPtr>
    {
        void ApplyConstant(NVRenderShaderProgram *program, NVRenderBackend *backend, QT3DSI32 location,
                           QT3DSI32 count, NVRenderShaderDataTypes::Enum type,
                           NVRenderImage2DPtr inValue, QT3DSU32 &oldValue, QT3DSI32 binding)
        {
            if (inValue) {
                NVRenderImage2D *imgObj = reinterpret_cast<NVRenderImage2D *>(inValue);
                imgObj->Bind(binding);
                QT3DSU32 texUnit = imgObj->GetTextureUnit();
                if (texUnit != oldValue) {
                    // on ES we need a explicit binding value
                    QT3DS_ASSERT(backend->GetRenderContextType() != NVRenderContextValues::GLES3PLUS
                              || binding != -1);
                    // this is not allowed on ES 3+ for image types
                    if (backend->GetRenderContextType() != NVRenderContextValues::GLES3PLUS)
                        backend->SetConstantValue(program->GetShaderProgramHandle(), location, type,
                                                  count, &texUnit);

                    oldValue = texUnit;
                }
            }
        }
    };

    NVRenderShaderProgram::NVRenderShaderProgram(NVRenderContextImpl &context,
                                                 NVFoundationBase &fnd, const char *programName,
                                                 bool separableProgram)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_ProgramName(programName)
        , m_ProgramHandle(NULL)
        , m_Constants(context.GetFoundation().getAllocator(), "NVRenderShaderProgram::m_Constants")
        , m_ShaderBuffers(context.GetFoundation().getAllocator(),
                          "NVRenderShaderProgram::m_ShaderBuffers")
        , m_ProgramType(ProgramType::Graphics)
    {
        m_ProgramHandle = m_Backend->CreateShaderProgram(separableProgram);

        QT3DS_ASSERT(m_ProgramHandle);
    }

    NVRenderShaderProgram::~NVRenderShaderProgram()
    {
        m_Context.ShaderDestroyed(*this);

        if (m_ProgramHandle)
            m_Backend->ReleaseShaderProgram(m_ProgramHandle);

        for (TShaderConstantMap::iterator iter = m_Constants.begin(), end = m_Constants.end();
             iter != end; ++iter) {
            iter->second->Release();
        }

        m_Constants.clear();

        for (TShaderBufferMap::iterator iter = m_ShaderBuffers.begin(), end = m_ShaderBuffers.end();
             iter != end; ++iter) {
            iter->second->Release();
        }

        m_ShaderBuffers.clear();

        m_ProgramHandle = NULL;
    }

    template <typename TShaderObject>
    void NVRenderShaderProgram::Attach(TShaderObject *pShader)
    {
        m_Backend->AttachShader(m_ProgramHandle, pShader->GetShaderHandle());
    }

    template <typename TShaderObject>
    void NVRenderShaderProgram::Detach(TShaderObject *pShader)
    {
        m_Backend->DetachShader(m_ProgramHandle, pShader->GetShaderHandle());
    }

    static NVRenderShaderConstantBase *
    ShaderConstantFactory(NVRenderBackend *backend, CRegisteredString inName,
                          NVFoundationBase &alloc, QT3DSI32 uniLoc, QT3DSI32 elementCount,
                          NVRenderShaderDataTypes::Enum inConstantType, QT3DSI32 binding)
    {
        switch (inConstantType) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(nv)                                                             \
    case NVRenderShaderDataTypes::nv:                                                              \
        return QT3DS_NEW(alloc.getAllocator(), NVRenderShaderConstant<nv>)(                           \
            backend, inName, uniLoc, elementCount, inConstantType, binding, alloc);
            ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
        default:
            break;
        }
        QT3DS_ASSERT(false);
        return NULL;
    }

    template <typename TShaderBufferType, typename TBufferDataType>
    static NVRenderShaderBufferBase *
    ShaderBufferFactory(NVRenderContextImpl &context, CRegisteredString inName,
                        NVFoundationBase &alloc, QT3DSI32 cbLoc, QT3DSI32 cbBinding, QT3DSI32 cbSize,
                        QT3DSI32 cbCount, TBufferDataType *pBuffer)
    {
        return QT3DS_NEW(alloc.getAllocator(), TShaderBufferType)(context, inName, cbLoc, cbBinding,
                                                               cbSize, cbCount, pBuffer, alloc);
    }

    bool NVRenderShaderProgram::Link()
    {
        bool success = m_Backend->LinkProgram(m_ProgramHandle, m_ErrorMessage);

        if (success) {
            char nameBuf[512];
            QT3DSI32 location, elementCount, binding;
            NVRenderShaderDataTypes::Enum type;

            QT3DSI32 constantCount = m_Backend->GetConstantCount(m_ProgramHandle);

            QT3DS_FOREACH(idx, constantCount)
            {
                location = m_Backend->GetConstantInfoByID(m_ProgramHandle, idx, 512, &elementCount,
                                                          &type, &binding, nameBuf);

                // sampler arrays have different type
                if (type == NVRenderShaderDataTypes::NVRenderTexture2DPtr && elementCount > 1) {
                    type = NVRenderShaderDataTypes::NVRenderTexture2DHandle;
                } else if (type == NVRenderShaderDataTypes::NVRenderTextureCubePtr
                           && elementCount > 1) {
                    type = NVRenderShaderDataTypes::NVRenderTextureCubeHandle;
                }
                if (location != -1) {
                    CRegisteredString theName(m_Context.GetStringTable().RegisterStr(nameBuf));
                    m_Constants.insert(eastl::make_pair(
                        theName,
                        ShaderConstantFactory(m_Backend, theName, m_Context.GetFoundation(),
                                              location, elementCount, type, binding)));
                }
            }

            // next query constant buffers info
            QT3DSI32 length, bufferSize, paramCount;
            QT3DSI32 constantBufferCount = m_Backend->GetConstantBufferCount(m_ProgramHandle);
            QT3DS_FOREACH(idx, constantBufferCount)
            {
                location = m_Backend->GetConstantBufferInfoByID(
                    m_ProgramHandle, idx, 512, &paramCount, &bufferSize, &length, nameBuf);

                if (location != -1) {
                    CRegisteredString theName(m_Context.GetStringTable().RegisterStr(nameBuf));

                    // find constant buffer in our DB
                    NVRenderConstantBuffer *cb = m_Context.GetConstantBuffer(theName);
                    if (cb) {
                        cb->SetupBuffer(this, location, bufferSize, paramCount);
                        cb->addRef();
                    }

                    m_ShaderBuffers.insert(eastl::make_pair(
                        theName,
                        ShaderBufferFactory<NVRenderShaderConstantBuffer, NVRenderConstantBuffer>(
                            m_Context, theName, m_Context.GetFoundation(), location, -1, bufferSize,
                            paramCount, cb)));
                }
            }

            // next query storage buffers
            QT3DSI32 storageBufferCount = m_Backend->GetStorageBufferCount(m_ProgramHandle);
            QT3DS_FOREACH(idx, storageBufferCount)
            {
                location = m_Backend->GetStorageBufferInfoByID(
                    m_ProgramHandle, idx, 512, &paramCount, &bufferSize, &length, nameBuf);

                if (location != -1) {
                    CRegisteredString theName(m_Context.GetStringTable().RegisterStr(nameBuf));

                    // find constant buffer in our DB
                    NVRenderStorageBuffer *sb = m_Context.GetStorageBuffer(theName);
                    if (sb) {
                        sb->addRef();
                    }

                    m_ShaderBuffers.insert(eastl::make_pair(
                        theName,
                        ShaderBufferFactory<NVRenderShaderStorageBuffer, NVRenderStorageBuffer>(
                            m_Context, theName, m_Context.GetFoundation(), location, -1, bufferSize,
                            paramCount, sb)));
                }
            }

            // next query atomic counter buffers
            QT3DSI32 atomicBufferCount = m_Backend->GetAtomicCounterBufferCount(m_ProgramHandle);
            QT3DS_FOREACH(idx, atomicBufferCount)
            {
                location = m_Backend->GetAtomicCounterBufferInfoByID(
                    m_ProgramHandle, idx, 512, &paramCount, &bufferSize, &length, nameBuf);

                if (location != -1) {
                    CRegisteredString theName(m_Context.GetStringTable().RegisterStr(nameBuf));

                    // find atomic counter buffer in our DB
                    // The buffer itself is not used in the program itself.
                    // Instead uniform variables are used but the interface to set the value is like
                    // for buffers.
                    // This is a bit insane but that is how it is.
                    // The theName variable contains the uniform name associated with an atomic
                    // counter buffer.
                    // We get the actual buffer name by searching for this uniform name
                    // See NVRenderTestAtomicCounterBuffer.cpp how the setup works
                    NVRenderAtomicCounterBuffer *acb =
                        m_Context.GetAtomicCounterBufferByParam(theName);
                    if (acb) {
                        acb->addRef();

                        m_ShaderBuffers.insert(eastl::make_pair(
                            acb->GetBufferName(),
                            ShaderBufferFactory<NVRenderShaderAtomicCounterBuffer,
                                                NVRenderAtomicCounterBuffer>(
                                m_Context, acb->GetBufferName(), m_Context.GetFoundation(),
                                location, -1, bufferSize, paramCount, acb)));
                    }
                }
            }
        }

        return success;
    }

    void NVRenderShaderProgram::GetErrorMessage(QT3DSI32 *messageLength, const char *errorMessage)
    {
        *messageLength = m_ErrorMessage.size();
        errorMessage = m_ErrorMessage.c_str();
    }

    const char *NVRenderShaderProgram::GetErrorMessage() { return m_ErrorMessage.c_str(); }

    NVRenderShaderConstantBase *NVRenderShaderProgram::GetShaderConstant(const char *constantName)
    {
        TShaderConstantMap::iterator theIter =
            m_Constants.find(m_Context.GetStringTable().RegisterStr(constantName));

        if (theIter != m_Constants.end()) {
            NVRenderShaderConstantBase *theConstant =
                static_cast<NVRenderShaderConstantBase *>(theIter->second);
            return theConstant;
        }

        return NULL;
    }

    NVRenderShaderBufferBase *NVRenderShaderProgram::GetShaderBuffer(const char *bufferName)
    {
        TShaderBufferMap::iterator theIter =
            m_ShaderBuffers.find(m_Context.GetStringTable().RegisterStr(bufferName));

        if (theIter != m_ShaderBuffers.end()) {
            return theIter->second;
        }

        return NULL;
    }

    NVRenderContextImpl &NVRenderShaderProgram::GetRenderContext() { return m_Context; }

    template <typename TDataType>
    void SetConstantValueOfType(NVRenderShaderProgram *program,
                                NVRenderShaderConstantBase *inConstantBase,
                                const TDataType &inValue, const QT3DSI32 inCount)
    {
        if (inConstantBase == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        QT3DS_ASSERT(inConstantBase->m_ElementCount >= inCount);

        if (inConstantBase->GetShaderConstantType()
            == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
            NVRenderShaderConstant<TDataType> *inConstant =
                static_cast<NVRenderShaderConstant<TDataType> *>(inConstantBase);
            ShaderConstantApplier<TDataType>().ApplyConstant(
                program, inConstant->m_Backend, inConstant->m_Location, inCount, inConstant->m_Type,
                inValue, inConstant->m_Value);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    template <typename TDataType>
    void SetSamplerConstantValueOfType(NVRenderShaderProgram *program,
                                       NVRenderShaderConstantBase *inConstantBase,
                                       const TDataType &inValue, const QT3DSI32 inCount)
    {
        if (inConstantBase == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        QT3DS_ASSERT(inConstantBase->m_ElementCount >= inCount);

        if (inConstantBase->GetShaderConstantType()
            == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
            NVRenderShaderConstant<TDataType> *inConstant =
                static_cast<NVRenderShaderConstant<TDataType> *>(inConstantBase);
            ShaderConstantApplier<TDataType>().ApplyConstant(
                program, inConstant->m_Backend, inConstant->m_Location, inCount, inConstant->m_Type,
                inValue, inConstant->m_Value, inConstant->m_Binding);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    template <typename TDataType>
    void SetMatrixConstantValueOfType(NVRenderShaderProgram *program,
                                      NVRenderShaderConstantBase *inConstantBase,
                                      const TDataType &inValue, const QT3DSI32 inCount,
                                      bool inTranspose)
    {
        if (inConstantBase == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        QT3DS_ASSERT(inConstantBase->m_ElementCount >= inCount);

        if (inConstantBase->GetShaderConstantType()
            == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
            NVRenderShaderConstant<TDataType> *inConstant =
                static_cast<NVRenderShaderConstant<TDataType> *>(inConstantBase);
            ShaderConstantApplier<TDataType>().ApplyConstant(
                program, inConstant->m_Backend, inConstant->m_Location, inCount, inConstant->m_Type,
                inValue, inConstant->m_Value, inTranspose);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    template <typename TDataType>
    void SetMatrixConstantValueOfType(NVRenderShaderProgram *program,
                                      NVRenderShaderConstantBase *inConstantBase,
                                      const NVConstDataRef<TDataType> inValue,
                                      const QT3DSI32 /*inCount*/, bool inTranspose)
    {
        if (inConstantBase == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        QT3DS_ASSERT(inConstantBase->m_ElementCount >= (QT3DSI32)inValue.size());

        if (inConstantBase->GetShaderConstantType()
            == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
            NVRenderShaderConstant<TDataType> *inConstant =
                static_cast<NVRenderShaderConstant<TDataType> *>(inConstantBase);
            ShaderConstantApplier<TDataType>().ApplyConstant(
                program, inConstant->m_Backend, inConstant->m_Location, inValue.size(),
                inConstant->m_Type, inValue, inConstant->m_Value, inTranspose);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 QT3DSI32 inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSI32_2 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSI32_3 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSI32_4 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 QT3DSRenderBool inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const bool_2 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const bool_3 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const bool_4 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSF32 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSVec2 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSVec3 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSVec4 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSU32 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSU32_2 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSU32_3 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSU32_4 &inValue, const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSMat33 &inValue, const QT3DSI32 inCount,
                                                 bool inTranspose)
    {
        SetMatrixConstantValueOfType(this, inConstant, inValue, inCount, inTranspose);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const QT3DSMat44 &inValue, const QT3DSI32 inCount,
                                                 bool inTranspose)
    {
        SetMatrixConstantValueOfType(this, inConstant, inValue, inCount, inTranspose);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 const NVConstDataRef<QT3DSMat44> inValue,
                                                 const QT3DSI32 inCount)
    {
        SetMatrixConstantValueOfType(this, inConstant, inValue, inCount, false);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 NVRenderTexture2D *inValue, const QT3DSI32 inCount)
    {
        Q_UNUSED(inCount)
        SetConstantValueOfType(this, inConstant, inValue, 1);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 NVRenderTexture2D **inValue,
                                                 const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 NVRenderTexture2DArray *inValue,
                                                 const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 NVRenderTextureCube *inValue, const QT3DSI32 inCount)
    {
        Q_UNUSED(inCount)
        SetConstantValueOfType(this, inConstant, inValue, 1);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 NVRenderTextureCube **inValue,
                                                 const QT3DSI32 inCount)
    {
        SetConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *inConstant,
                                                 NVRenderImage2D *inValue, const QT3DSI32 inCount)
    {
        SetSamplerConstantValueOfType(this, inConstant, inValue, inCount);
    }
    void NVRenderShaderProgram::SetConstantValue(NVRenderShaderConstantBase *, NVRenderDataBuffer *,
                                                 const QT3DSI32)
    {
        // this is merely a dummy right now
    }

    void NVRenderShaderProgram::BindComputeInput(NVRenderDataBuffer *inBuffer, QT3DSU32 inIndex)
    {
        NVRenderBackend::NVRenderBackendBufferObject obj(NULL);
        if (inBuffer)
            obj = inBuffer->GetBuffertHandle();
        m_Backend->ProgramSetStorageBuffer(inIndex, obj);
    }

    namespace {
        void WriteErrorMessage(NVFoundationBase &fnd, const char *tag, const char *message)
        {
            Q_UNUSED(fnd)
            eastl::string messageData(nonNull(message));
            eastl::vector<eastl::string> lines;
            for (eastl::string::size_type pos = messageData.find('\n'); pos != eastl::string::npos;
                 pos = messageData.find('\n')) {
                eastl::string line = messageData.substr(0, pos);
                messageData.erase(messageData.begin(), messageData.begin() + pos + 1);
                if (line.size())
                    qCCritical(INVALID_OPERATION, "%s: %s", tag, line.c_str());
            }
        }
    }

    Option<NVRenderVertexShader *> NVRenderShaderProgram::createVertexShader(
        NVRenderContextImpl &context, NVConstDataRef<QT3DSI8> vertexShaderSource, bool binaryProgram)
    {
        if (vertexShaderSource.size() == 0)
            return Empty();

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderVertexShader(context, context.GetFoundation(), vertexShaderSource,
                                           binaryProgram));
    }

    Option<NVRenderFragmentShader *> NVRenderShaderProgram::createFragmentShader(
        NVRenderContextImpl &context, NVConstDataRef<QT3DSI8> fragmentShaderSource, bool binaryProgram)
    {
        if (fragmentShaderSource.size() == 0)
            return Empty();

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderFragmentShader(context, context.GetFoundation(), fragmentShaderSource,
                                             binaryProgram));
    }

    Option<NVRenderTessControlShader *>
    NVRenderShaderProgram::createTessControlShader(NVRenderContextImpl &context,
                                                   NVConstDataRef<QT3DSI8> tessControlShaderSource,
                                                   bool binaryProgram)
    {
        if (tessControlShaderSource.size() == 0)
            return Empty();

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderTessControlShader(context, context.GetFoundation(),
                                                tessControlShaderSource, binaryProgram));
    }

    Option<NVRenderTessEvaluationShader *>
    NVRenderShaderProgram::createTessEvaluationShader(NVRenderContextImpl &context,
                                                      NVConstDataRef<QT3DSI8> tessControlShaderSource,
                                                      bool binaryProgram)
    {
        if (tessControlShaderSource.size() == 0)
            return Empty();

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderTessEvaluationShader(context, context.GetFoundation(),
                                                   tessControlShaderSource, binaryProgram));
    }

    Option<NVRenderGeometryShader *> NVRenderShaderProgram::createGeometryShader(
        NVRenderContextImpl &context, NVConstDataRef<QT3DSI8> geometryShaderSource, bool binaryProgram)
    {
        if (geometryShaderSource.size() == 0)
            return Empty();

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderGeometryShader(context, context.GetFoundation(), geometryShaderSource,
                                             binaryProgram));
    }

    NVRenderVertFragCompilationResult NVRenderShaderProgram::Create(
        NVRenderContextImpl &context, const char *programName,
        NVConstDataRef<QT3DSI8> vertShaderSource, NVConstDataRef<QT3DSI8> fragShaderSource,
        NVConstDataRef<QT3DSI8> tessControlShaderSource,
        NVConstDataRef<QT3DSI8> tessEvaluationShaderSource, NVConstDataRef<QT3DSI8> geometryShaderSource,
        bool separateProgram, NVRenderShaderProgramBinaryType::Enum type, bool binaryProgram)
    {
        NVRenderVertFragCompilationResult result;
        NVRenderShaderProgram *pProgram = NULL;
        bool bProgramIsValid = false;

        result.mShaderName = programName;

        // our minimum requirement is a vertex and a fragment shader or geometry shader
        // if we should treat it as a separate program we don't care
        if (!separateProgram
            && (vertShaderSource.size() == 0
                || (fragShaderSource.size() == 0 && geometryShaderSource.size() == 0))) {
            qCCritical(INVALID_PARAMETER,
                "Vertex or fragment (geometry) source have 0 length");
            QT3DS_ASSERT(false);
            return result;
        }

        if (binaryProgram && type != NVRenderShaderProgramBinaryType::NVBinary) {
            qCCritical(INVALID_PARAMETER, "Unrecoginzed binary format");
            QT3DS_ASSERT(false);
            return result;
        }

        // first create and compile shader
        Option<NVRenderVertexShader *> vtxShader =
            createVertexShader(context, vertShaderSource, binaryProgram);
        Option<NVRenderFragmentShader *> fragShader =
            createFragmentShader(context, fragShaderSource, binaryProgram);
        Option<NVRenderTessControlShader *> tcShader =
            createTessControlShader(context, tessControlShaderSource, binaryProgram);
        Option<NVRenderTessEvaluationShader *> teShader =
            createTessEvaluationShader(context, tessEvaluationShaderSource, binaryProgram);
        Option<NVRenderGeometryShader *> geShader =
            createGeometryShader(context, geometryShaderSource, binaryProgram);

        bool vertexValid = (vtxShader.hasValue()) ? vtxShader.getValue()->IsValid() : true;
        bool fragValid = (fragShader.hasValue()) ? fragShader.getValue()->IsValid() : true;
        bool tcValid = (tcShader.hasValue()) ? tcShader.getValue()->IsValid() : true;
        bool teValid = (teShader.hasValue()) ? teShader.getValue()->IsValid() : true;
        bool geValid = (geShader.hasValue()) ? geShader.getValue()->IsValid() : true;

        if (vertexValid && fragValid && tcValid && teValid && geValid) {
            // shaders were succesfuly created
            pProgram = QT3DS_NEW(context.GetFoundation().getAllocator(), NVRenderShaderProgram)(
                context, context.GetFoundation(), programName, separateProgram);

            if (pProgram) {
                // attach programs
                if (vtxShader.hasValue() && vtxShader.getValue()->IsValid())
                    pProgram->Attach(vtxShader.getValue());
                if (fragShader.hasValue() && fragShader.getValue()->IsValid())
                    pProgram->Attach(fragShader.getValue());
                if (tcShader.hasValue() && tcShader.getValue()->IsValid())
                    pProgram->Attach(tcShader.getValue());
                if (teShader.hasValue() && teShader.getValue()->IsValid())
                    pProgram->Attach(teShader.getValue());
                if (geShader.hasValue() && geShader.getValue()->IsValid())
                    pProgram->Attach(geShader.getValue());

                // link program
                bProgramIsValid = pProgram->Link();
            }
        }

        // if anything went wrong print out
        if (!vertexValid || !fragValid || !tcValid || !teValid || !geValid || !bProgramIsValid) {
            NVFoundationBase &foundation(context.GetFoundation());

            if (!vertexValid) {
                qCCritical(INTERNAL_ERROR, "Failed to generate vertex shader!!");
                qCCritical(INTERNAL_ERROR, "Vertex source:\n%s",
                    nonNull((const char *)vertShaderSource.begin()));
                WriteErrorMessage(foundation, "Vertex compilation output:",
                                  vtxShader.getValue()->GetErrorMessage());
            }

            if (!fragValid) {
                qCCritical(INTERNAL_ERROR, "Failed to generate fragment shader!!");
                qCCritical(INTERNAL_ERROR, "Fragment source:\n%s",
                    nonNull((const char *)fragShaderSource.begin()));
                WriteErrorMessage(foundation, "Fragment compilation output:",
                                  fragShader.getValue()->GetErrorMessage());
            }

            if (!tcValid) {
                qCCritical(INTERNAL_ERROR,
                    "Failed to generate tessellation control shader!!");
                qCCritical(INTERNAL_ERROR, "Tessellation control source:\n%s",
                    nonNull((const char *)tessControlShaderSource.begin()));
                WriteErrorMessage(foundation, "Tessellation control compilation output:",
                                  tcShader.getValue()->GetErrorMessage());
            }

            if (!teValid) {
                qCCritical(INTERNAL_ERROR,
                    "Failed to generate tessellation evaluation shader!!");
                qCCritical(INTERNAL_ERROR, "Tessellation evaluation source:\n%s",
                    nonNull((const char *)tessEvaluationShaderSource.begin()));
                WriteErrorMessage(foundation, "Tessellation evaluation compilation output:",
                                  teShader.getValue()->GetErrorMessage());
            }

            if (!geValid) {
                qCCritical(INTERNAL_ERROR, "Failed to generate geometry shader!!");
                qCCritical(INTERNAL_ERROR, "Geometry source:\n%s",
                    nonNull((const char *)geometryShaderSource.begin()));
                WriteErrorMessage(foundation, "Geometry compilation output:",
                                  geShader.getValue()->GetErrorMessage());
            }

            if (!bProgramIsValid && pProgram) {
                qCCritical(INTERNAL_ERROR, "Failed to link program!!");
                WriteErrorMessage(foundation, "Program link output:", pProgram->GetErrorMessage());

                // delete program
                QT3DS_FREE(context.GetFoundation().getAllocator(), pProgram);
                pProgram = NULL;
            }
        }

        // clean up
        if (vtxShader.hasValue()) {
            if (bProgramIsValid && vtxShader.getValue()->IsValid())
                pProgram->Detach(vtxShader.getValue());
            QT3DS_FREE(context.GetFoundation().getAllocator(), vtxShader.getValue());
        }
        if (fragShader.hasValue()) {
            if (bProgramIsValid && fragShader.getValue()->IsValid())
                pProgram->Detach(fragShader.getValue());
            QT3DS_FREE(context.GetFoundation().getAllocator(), fragShader.getValue());
        }
        if (tcShader.hasValue()) {
            if (bProgramIsValid && tcShader.getValue()->IsValid())
                pProgram->Detach(tcShader.getValue());
            QT3DS_FREE(context.GetFoundation().getAllocator(), tcShader.getValue());
        }
        if (teShader.hasValue()) {
            if (bProgramIsValid && teShader.getValue()->IsValid())
                pProgram->Detach(teShader.getValue());
            QT3DS_FREE(context.GetFoundation().getAllocator(), teShader.getValue());
        }
        if (geShader.hasValue()) {
            if (bProgramIsValid && geShader.getValue()->IsValid())
                pProgram->Detach(geShader.getValue());
            QT3DS_FREE(context.GetFoundation().getAllocator(), geShader.getValue());
        }

        // set program
        result.mShader = pProgram;

        return result;
    }

    NVRenderVertFragCompilationResult
    NVRenderShaderProgram::CreateCompute(NVRenderContextImpl &context, const char *programName,
                                         NVConstDataRef<QT3DSI8> computeShaderSource)
    {
        NVRenderVertFragCompilationResult result;
        NVRenderShaderProgram *pProgram = NULL;
        bool bProgramIsValid = true;

        result.mShaderName = programName;

        // check source
        if (computeShaderSource.size() == 0) {
            qCCritical(INVALID_PARAMETER, "compute source has 0 length");
            QT3DS_ASSERT(false);
            return result;
        }

        NVRenderComputeShader computeShader(context, context.GetFoundation(), computeShaderSource,
                                            false);

        if (computeShader.IsValid()) {
            // shaders were succesfuly created
            pProgram = QT3DS_NEW(context.GetFoundation().getAllocator(), NVRenderShaderProgram)(
                context, context.GetFoundation(), programName, false);

            if (pProgram) {
                // attach programs
                pProgram->Attach(&computeShader);

                // link program
                bProgramIsValid = pProgram->Link();

                // set program type
                pProgram->SetProgramType(ProgramType::Compute);
            }
        }

        // if anything went wrong print out
        if (!computeShader.IsValid() || !bProgramIsValid) {
            NVFoundationBase &foundation(context.GetFoundation());

            if (!computeShader.IsValid()) {
                qCCritical(INTERNAL_ERROR, "Failed to generate compute shader!!");
                qCCritical(INTERNAL_ERROR, "Vertex source:\n%s",
                    nonNull((const char *)computeShaderSource.begin()));
                WriteErrorMessage(foundation, "Compute shader compilation output:",
                                  computeShader.GetErrorMessage());
            }
        }

        // set program
        result.mShader = pProgram;

        return result;
    }
}
}
