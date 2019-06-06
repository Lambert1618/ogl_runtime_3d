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
#ifndef QT3DS_RENDER_SHADER_CONSTANT_H
#define QT3DS_RENDER_SHADER_CONSTANT_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSMat33.h"
#include "foundation/Qt3DSMat44.h"
#include "EASTL/string.h"
#include "EASTL/utility.h"
#include "render/Qt3DSRenderContext.h"

namespace qt3ds {
namespace render {
    using namespace foundation;

    ///< forward declarations
    class NVRenderContextImpl;
    class NVRenderConstantBuffer;

    ///< A shader constant belongs to a program
    class QT3DS_AUTOTEST_EXPORT NVRenderShaderConstantBase
    {
    public:
        NVRenderBackend *m_Backend; ///< pointer to backend
        CRegisteredString m_Name; ///< register constant name
        QT3DSI32 m_Location; ///< constant index
        QT3DSI32 m_ElementCount; ///< constant element count for arrays
        NVRenderShaderDataTypes::Enum m_Type; ///< constant type
        QT3DSI32 m_Binding; ///< sampler/imnage binding point

    public:
        NVRenderShaderConstantBase(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                                   QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                                   QT3DSI32 binding)
            : m_Backend(backend)
            , m_Name(name)
            , m_Location(location)
            , m_ElementCount(elementCount)
            , m_Type(type)
            , m_Binding(binding)
        {
        }

        NVRenderShaderDataTypes::Enum GetShaderConstantType() const { return m_Type; }

        virtual void Release() = 0;
    };

    ///< A general class for shader types
    template <typename TDataType>
    class NVRenderShaderConstant : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        TDataType m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            memset(&m_Value, 0, sizeof(TDataType));
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< A specialized class for textures
    template <>
    class NVRenderShaderConstant<NVRenderTexture2DPtr> : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSU32 m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            m_Value = QT3DS_MAX_U32;
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< A specialized class for textures
    template <>
    class NVRenderShaderConstant<NVRenderTexture2DHandle> : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QVector<QT3DSU32> m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            m_Value.resize(elementCount);
            m_Value.fill(QT3DS_MAX_U32);
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< A specialized class for texture arrays
    template <>
    class NVRenderShaderConstant<NVRenderTexture2DArrayPtr> : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSU32 m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            m_Value = QT3DS_MAX_U32;
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< A specialized class for cubemap textures
    template <>
    class NVRenderShaderConstant<NVRenderTextureCubePtr> : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSU32 m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            m_Value = QT3DS_MAX_U32;
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< A specialized class for cubemap textures
    template <>
    class NVRenderShaderConstant<NVRenderTextureCubeHandle> : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QVector<QT3DSU32> m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            m_Value.resize(elementCount);
            m_Value.fill(QT3DS_MAX_U32);
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< A specialized class for texture image buffer
    template <>
    class NVRenderShaderConstant<NVRenderImage2DPtr> : public NVRenderShaderConstantBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSU32 m_Value; ///< constant value

    public:
        NVRenderShaderConstant(NVRenderBackend *backend, CRegisteredString name, QT3DSI32 location,
                               QT3DSI32 elementCount, NVRenderShaderDataTypes::Enum type,
                               QT3DSI32 binding, NVFoundationBase &allocator)
            : NVRenderShaderConstantBase(backend, name, location, elementCount, type, binding)
            , m_Foundation(allocator)
        {
            m_Value = QT3DS_MAX_U32;
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override { NVDelete(GetFoundation().getAllocator(), this); }
    };

    ///< Base for any buffer ( constant, texture, ... ) which is used by this program
    class NVRenderShaderBufferBase
    {
    public:
        NVRenderContextImpl &m_Context; ///< pointer to context
        CRegisteredString m_Name; ///< buffer name
        QT3DSU32 m_Location; ///< program buffer block location
        QT3DSU32 m_Binding; ///< program buffer binding
        QT3DSI32 m_Size; ///< buffer size

    public:
        NVRenderShaderBufferBase(NVRenderContextImpl &context, CRegisteredString name,
                                 QT3DSI32 location, QT3DSI32 binding, QT3DSI32 size)
            : m_Context(context)
            , m_Name(name)
            , m_Location(location)
            , m_Binding(binding)
            , m_Size(size)
        {
        }

        virtual void Release() = 0;

        virtual void Validate(NVRenderShaderProgram *inShader) = 0;
        virtual void BindToProgram(NVRenderShaderProgram *inShader) = 0;
        virtual void Update() = 0;
    };

    class NVRenderShaderConstantBuffer : public NVRenderShaderBufferBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSI32 m_ParamCount; ///< count of parameters contained in the constant buffer
        NVRenderConstantBuffer *m_pCB; ///< pointer to constant buffer

    public:
        NVRenderShaderConstantBuffer(NVRenderContextImpl &context, CRegisteredString name,
                                     QT3DSU32 location, QT3DSI32 binding, QT3DSI32 size, QT3DSI32 count,
                                     NVRenderConstantBuffer *pCB, NVFoundationBase &allocator)
            : NVRenderShaderBufferBase(context, name, location, binding, size)
            , m_Foundation(allocator)
            , m_ParamCount(count)
            , m_pCB(pCB)
        {
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override
        {
            if (m_pCB)
                m_pCB->release();

            NVDelete(GetFoundation().getAllocator(), this);
        }

        void Validate(NVRenderShaderProgram *inShader) override
        {
            // A constant buffer might not be set at first call
            // due to the fact that they are compiled from a cache file
            // Now it must exists.
            if (m_pCB)
                return;

            NVRenderConstantBuffer *cb = m_Context.GetConstantBuffer(m_Name);
            if (cb) {
                cb->SetupBuffer(inShader, m_Location, m_Size, m_ParamCount);
                cb->addRef();
                m_pCB = cb;
            } else {
                QT3DS_ASSERT(false);
            }
        }

        void Update() override
        {
            if (m_pCB)
                m_pCB->Update();
        }

        void BindToProgram(NVRenderShaderProgram *inShader) override
        {
            if (m_pCB)
                m_pCB->BindToShaderProgram(inShader, m_Location, m_Binding);
        }
    };

    class NVRenderShaderStorageBuffer : public NVRenderShaderBufferBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSI32 m_ParamCount; ///< count of parameters contained in the constant buffer
        NVRenderStorageBuffer *m_pSB; ///< pointer to storage buffer

    public:
        NVRenderShaderStorageBuffer(NVRenderContextImpl &context, CRegisteredString name,
                                    QT3DSU32 location, QT3DSI32 binding, QT3DSI32 size, QT3DSI32 count,
                                    NVRenderStorageBuffer *pSB, NVFoundationBase &allocator)
            : NVRenderShaderBufferBase(context, name, location, binding, size)
            , m_Foundation(allocator)
            , m_ParamCount(count)
            , m_pSB(pSB)
        {
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override
        {
            if (m_pSB)
                m_pSB->release();

            NVDelete(GetFoundation().getAllocator(), this);
        }

        void Validate(NVRenderShaderProgram * /*inShader*/) override
        {
            // A constant buffer might not be set at first call
            // due to the fact that they are compile from a cache file
            // Now it must exists.
            if (m_pSB)
                return;

            NVRenderStorageBuffer *sb = m_Context.GetStorageBuffer(m_Name);
            if (sb) {
                sb->addRef();
                m_pSB = sb;
            } else {
                QT3DS_ASSERT(false);
            }
        }

        void Update() override
        {
            if (m_pSB)
                m_pSB->Update();
        }

        void BindToProgram(NVRenderShaderProgram * /*inShader*/) override
        {
            if (m_pSB)
                m_pSB->BindToShaderProgram(m_Location);
        }
    };

    class NVRenderShaderAtomicCounterBuffer : public NVRenderShaderBufferBase
    {
    public:
        NVFoundationBase &m_Foundation; ///< allocator
        QT3DSI32 m_ParamCount; ///< count of parameters contained in the constant buffer
        NVRenderAtomicCounterBuffer *m_pAcB; ///< pointer to atomic counter buffer

    public:
        NVRenderShaderAtomicCounterBuffer(NVRenderContextImpl &context, CRegisteredString name,
                                          QT3DSU32 location, QT3DSI32 binding, QT3DSI32 size, QT3DSI32 count,
                                          NVRenderAtomicCounterBuffer *pAcB,
                                          NVFoundationBase &allocator)
            : NVRenderShaderBufferBase(context, name, location, binding, size)
            , m_Foundation(allocator)
            , m_ParamCount(count)
            , m_pAcB(pAcB)
        {
        }

        NVFoundationBase &GetFoundation() { return m_Foundation; }

        void Release() override
        {
            if (m_pAcB)
                m_pAcB->release();

            NVDelete(GetFoundation().getAllocator(), this);
        }

        void Validate(NVRenderShaderProgram * /*inShader*/) override
        {
            // A constant buffer might not be set at first call
            // due to the fact that they are compile from a cache file
            // Now it must exists.
            if (m_pAcB)
                return;

            NVRenderAtomicCounterBuffer *acb = m_Context.GetAtomicCounterBuffer(m_Name);
            if (acb) {
                acb->addRef();
                m_pAcB = acb;
            } else {
                QT3DS_ASSERT(false);
            }
        }

        void Update() override
        {
            if (m_pAcB)
                m_pAcB->Update();
        }

        void BindToProgram(NVRenderShaderProgram * /*inShader*/) override
        {
            if (m_pAcB)
                m_pAcB->BindToShaderProgram(m_Location);
        }
    };
}
}

#endif
