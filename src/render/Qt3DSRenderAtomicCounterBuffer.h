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
#ifndef QT3DS_RENDER_QT3DS_RENDER_ATOMIC_COUNTER_BUFFER_H
#define QT3DS_RENDER_QT3DS_RENDER_ATOMIC_COUNTER_BUFFER_H
#include "foundation/Qt3DSOption.h"
#include "foundation/Utils.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderDataBuffer.h"

namespace qt3ds {
namespace render {

    using namespace foundation;

    // forward declaration
    class NVRenderContextImpl;
    class AtomicCounterBufferEntry;

    typedef nvhash_map<CRegisteredString, AtomicCounterBufferEntry *>
        TRenderAtomiCounterBufferEntryMap;

    ///< Constant (uniform) buffer representation
    class QT3DS_AUTOTEST_EXPORT NVRenderAtomicCounterBuffer : public NVRenderDataBuffer
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
        NVRenderAtomicCounterBuffer(NVRenderContextImpl &context, CRegisteredString bufferName,
                                    size_t size, NVRenderBufferUsageType::Enum usageType,
                                    NVDataRef<QT3DSU8> data);

        ///< destructor
        virtual ~NVRenderAtomicCounterBuffer();

        /**
         * @brief bind the buffer bypasses the context state
         *
         * @return no return.
         */
        void Bind() override;

        /**
         * @brief bind the buffer to a shader program
         *
         * @param[in] index			Index of the constant buffer within the program
         *
         * @return no return.
         */
        virtual void BindToShaderProgram(QT3DSU32 index);

        /**
         * @brief update the buffer to hardware
         *
         * @return no return.
         */
        virtual void Update();

        /**
         * @brief update a piece of memory directly within the storage buffer
         *
         * Note: When you use this function you should know what you are doing.
         *		 The memory layout within C++ must exactly match the memory layout in the
         *shader.
         *		 We use std140 (430) layout which guarantees a specific layout behavior across
         *all HW vendors.
         *		 How the memory layout is computed can be found in the GL spec.
         *
         * @param[in] offset	offset into storage buffer
         * @param[in] data		pointer to data
         *
         * @return no return
         */
        void UpdateData(QT3DSI32 offset, NVDataRef<QT3DSU8> data);

        /**
         * @brief add a parameter to the atomic counter buffer
         *
         * @param[in] name		Name of the parameter (must match the name in the shader
         * program)
         * @param[in] offset	Offset in bytes into the buffer
         *
         * @return no return
         */
        void AddParam(CRegisteredString name, QT3DSU32 offset);

        /**
         * @brief Check if the buffer contains this param
         *
         * @param[in] name		Name of the parameter (must match the name in the shader
         * program)
         *
         * @return no return
         */
        bool ContainsParam(CRegisteredString name);

        /**
         * @brief get the buffer name
         *
         * @return the buffer name
         */
        CRegisteredString GetBufferName() const { return m_Name; }

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
         * @brief create a NVRenderAtomicCounterBuffer object
         *
         * @param[in] context		Pointer to context
         * @param[in] size			Size of the buffer
         * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
         * @param[in] data			A pointer to the buffer data that is allocated by the
         * application.
         *
         * @return the buffer object or NULL
         */
        static NVRenderAtomicCounterBuffer *Create(NVRenderContextImpl &context,
                                                   const char *bufferName,
                                                   NVRenderBufferUsageType::Enum usageType,
                                                   size_t size, NVConstDataRef<QT3DSU8> bufferData);

    private:
        CRegisteredString m_Name; ///< buffer name
        TRenderAtomiCounterBufferEntryMap
            m_AtomicCounterBufferEntryMap; ///< holds the entries of a atomic counter buffer
        bool m_Dirty; ///< true if buffer is dirty
    };
}
}

#endif
