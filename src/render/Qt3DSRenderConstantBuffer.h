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
#ifndef QT3DS_RENDER_QT3DS_RENDER_CONSTANT_BUFFER_H
#define QT3DS_RENDER_QT3DS_RENDER_CONSTANT_BUFFER_H
#include "foundation/Qt3DSOption.h"
#include "foundation/Utils.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderDataBuffer.h"

namespace qt3ds {
namespace render {

    using namespace foundation;

    // forward declaration
    class NVRenderContextImpl;
    class ConstantBufferParamEntry;
    class NVRenderShaderProgram;

    typedef nvhash_map<CRegisteredString, ConstantBufferParamEntry *> TRenderConstantBufferEntryMap;

    ///< Constant (uniform) buffer representation
    class QT3DS_AUTOTEST_EXPORT NVRenderConstantBuffer : public NVRenderDataBuffer
    {
    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] bufferName	Name of the buffer. Must match the name used in programs
         * @param[in] size			Size of the buffer
         * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
         * @param[in] data			A pointer to the buffer data that is allocated by the
         * application.
         *
         * @return No return.
         */
        NVRenderConstantBuffer(NVRenderContextImpl &context, CRegisteredString bufferName,
                               size_t size, NVRenderBufferUsageType::Enum usageType,
                               NVDataRef<QT3DSU8> data);

        ///< destructor
        virtual ~NVRenderConstantBuffer();

        /**
         * @brief bind the buffer bypasses the context state
         *
         * @return no return.
         */
        void Bind() override;

        /**
         * @brief bind the buffer to a shader program
         *
         * @param[in] inShader		Pointer to active program
         * @param[in] blockIndex	Index of the constant block within the program
         * @param[in] binding		Binding point of constant buffer
         *
         * @return no return.
         */
        virtual void BindToShaderProgram(NVRenderShaderProgram *inShader, QT3DSU32 blockIndex,
                                         QT3DSU32 binding);

        /**
         * @brief update the buffer to hardware
         *
         * @return no return.
         */
        virtual void Update();

        /**
         * @brief setup constant buffer
         *
         * @param[in] pProgram		Pointer to the shader program
         * @param[in] index			Index of the constant buffer within the program
         * @param[in] bufSize		Size of the constant buffer
         * @param[in] paramCount	Parameter entry count of the constant buffer
         *
         * @return return if successful
         */
        bool SetupBuffer(NVRenderShaderProgram *pProgram, QT3DSI32 index, QT3DSI32 bufSize,
                         QT3DSI32 paramCount);

        /**
         * @brief add a parameter to the constant buffer
         *
         * @param[in] name		Name of the parameter (must match the name in the shader
         * program)
         * @param[in] type		Type of the parameter like Mat44
         * @param[in] count		One or size of array
         *
         * @return no return
         */
        void AddParam(CRegisteredString name, NVRenderShaderDataTypes::Enum type, QT3DSI32 count);

        /**
         * @brief update a parameter in the constant buffer
         *
         * @param[in] name		Name of the parameter (must match the name in the shader
         * program)
         * @param[in] value		New value
         *
         * @return no return
         */
        void UpdateParam(const char *name, NVDataRef<QT3DSU8> value);

        /**
         * @brief update a piece of memory directly within the constant buffer
         *
         * Note: When you use this function you should know what you are doing.
         *		 The memory layout within C++ must exactly match the memory layout in the
         *shader.
         *		 We use std140 layout which guarantees a specific layout behavior across all
         *HW vendors.
         *		 How the memory layout is computed can be found in the GL spec.
         *
         * @param[in] offset	offset into constant buffer
         * @param[in] data		pointer to new data
         *
         * @return no return
         */
        void UpdateRaw(QT3DSI32 offset, NVDataRef<QT3DSU8> data);

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendBufferObject GetBuffertHandle() const override
        {
            return m_BufferHandle;
        }

        // this will be obsolete
        const void *GetImplementationHandle() const override
        {
            return reinterpret_cast<void *>(m_BufferHandle);
        }

        /**
         * @brief create a NVRenderConstantBuffer object
         *
         * @param[in] context		Pointer to context
         * @param[in] size			Size of the buffer
         * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
         * @param[in] data			A pointer to the buffer data that is allocated by the
         * application.
         *
         * @return the backend object handle.
         */
        static NVRenderConstantBuffer *Create(NVRenderContextImpl &context, const char *bufferName,
                                              NVRenderBufferUsageType::Enum usageType, size_t size,
                                              NVConstDataRef<QT3DSU8> bufferData);

        /**
         * @brief get the buffer name
         *
         * @return the buffer name
         */
        CRegisteredString GetBufferName() const { return m_Name; }

    private:
        /**
         * @brief Create a parameter entry
         *
         * @param[in] name		Name of the parameter (must match the name in the shader
         * program)
         * @param[in] type		Type of the parameter like Mat44
         * @param[in] count		One or size of array
         * @param[in] offset	Offset of the parameter in the memory buffer
         *
         * @return return new Entry
         */
        ConstantBufferParamEntry *createParamEntry(CRegisteredString name,
                                                   NVRenderShaderDataTypes::Enum type, QT3DSI32 count,
                                                   QT3DSI32 offset);

        /**
         * @brief get size of a uniform type
         *
         * @param[in] type		type of uniform
         *
         * @return return uniform size
         */
        QT3DSI32
        getUniformTypeSize(NVRenderShaderDataTypes::Enum type);

        /**
         * @brief allocate the shadow buffer
         *
         * @param[in] size		size of buffer
         *
         * @return return true on success
         */
        bool allocateShadowBuffer(QT3DSU32 size);

        /**
         * @brief update a certain range of the buffer to hardware
         *
         * @return no return.
         */
        virtual void UpdateRange();

    private:
        CRegisteredString m_Name; ///< buffer name
        TRenderConstantBufferEntryMap
            m_ConstantBufferEntryMap; ///< holds the entries of a constant buffer
        QT3DSU32 m_CurrentOffset; ///< holds the current offset
        QT3DSU32 m_CurrentSize; ///< holds the current size
        bool m_HWBufferInitialized; ///< true if the hardware version of the buffer is initialized
        bool m_Dirty; ///< true if buffer is dirty
        QT3DSU32 m_RangeStart; ///< start offset of the range to update
        QT3DSU32 m_RangeEnd; ///< size of the range to update
        QT3DSI32 m_MaxBlockSize; ///< maximum size for a single constant buffer
        NVDataRef<QT3DSU8> m_ShadowCopy; ///< host copy of the data in the GPU
    };
}
}

#endif
