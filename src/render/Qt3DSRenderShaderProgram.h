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
#ifndef QT3DS_RENDER_SHADER_PROGRAM_H
#define QT3DS_RENDER_SHADER_PROGRAM_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderShaderConstant.h"
#include "EASTL/string.h"
#include "EASTL/utility.h"

namespace qt3ds {
namespace render {

    using namespace foundation;

    ///< forward declarations
    class NVRenderContextImpl;
    class NVRenderVertexShader;
    class NVRenderFragmentShader;
    class NVRenderTessControlShader;
    class NVRenderTessEvaluationShader;
    class NVRenderGeometryShader;
    class NVRenderShaderConstantBase;
    class NVRenderShaderBufferBase;
    class NVRenderComputeShader;

    typedef nvhash_map<CRegisteredString, NVRenderShaderConstantBase *> TShaderConstantMap;
    typedef nvhash_map<CRegisteredString, NVRenderShaderBufferBase *> TShaderBufferMap;

    ///< A shader program is an object composed of a multiple shaders (vertex, fragment,
    ///geometry,....)
    class QT3DS_AUTOTEST_EXPORT NVRenderShaderProgram : public NVRefCounted
    {
    public:
        struct ProgramType
        {
            enum Enum { Graphics, Compute };
        };

    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        const char *m_ProgramName; /// Name of the program
        NVRenderBackend::NVRenderBackendShaderProgramObject
            m_ProgramHandle; ///< opaque backend handle
        TShaderConstantMap m_Constants; ///< map of shader constants
        TShaderBufferMap m_ShaderBuffers; ///< map of shader buffers
        ProgramType::Enum m_ProgramType; ///< shader type
        eastl::string m_ErrorMessage; ///< contains the error message if linking fails

        /**
         * @brief create vertex shader
         *
         * @param[in] context					Pointer to render context
         * @param[in] vertexShaderSource		Fragment shader source code
         * @param[in] binaryProgram				True if binary program
         *
         * @return pointer to vertex shader object
         */
        static Option<NVRenderVertexShader *>
        createVertexShader(NVRenderContextImpl &context, NVConstDataRef<QT3DSI8> vertexShaderSource,
                           bool binaryProgram = false);

        /**
         * @brief create fragment shader
         *
         * @param[in] context					Pointer to render context
         * @param[in] fragmentShaderSource		Fragment shader source code
         * @param[in] binaryProgram				True if binary program
         *
         * @return pointer to fragment shader object
         */
        static Option<NVRenderFragmentShader *>
        createFragmentShader(NVRenderContextImpl &context,
                             NVConstDataRef<QT3DSI8> fragmentShaderSource, bool binaryProgram = false);

        /**
         * @brief create tesselation control shader
         *
         * @param[in] context					Pointer to render context
         * @param[in] tessControlShaderSource	Tessellation control shader source code
         * @param[in] binaryProgram				True if binary program
         *
         * @return pointer to tessellation control shader
         */
        static Option<NVRenderTessControlShader *>
        createTessControlShader(NVRenderContextImpl &context,
                                NVConstDataRef<QT3DSI8> tessControlShaderSource,
                                bool binaryProgram = false);

        /**
         * @brief create tesselation evaluation shader
         *
         * @param[in] context						Pointer to render context
         * @param[in] tessEvaluationShaderSource	Tessellation evaluation shader source code
         * @param[in] binaryProgram					True if binary program
         *
         * @return pointer to tessellation evaluation shader
         */
        static Option<NVRenderTessEvaluationShader *>
        createTessEvaluationShader(NVRenderContextImpl &context,
                                   NVConstDataRef<QT3DSI8> tessEvaluationShaderSource,
                                   bool binaryProgram = false);

        /**
         * @brief create geometry shader
         *
         * @param[in] context					Pointer to render context
         * @param[in] geometryShaderSource		Geometry shader source code
         * @param[in] binaryProgram				True if binary program
         *
         * @return pointer to geometry shader
         */
        static Option<NVRenderGeometryShader *>
        createGeometryShader(NVRenderContextImpl &context,
                             NVConstDataRef<QT3DSI8> geometryShaderSource, bool binaryProgram = false);

    public:
        /**
         * @brief constructor
         *
         * @param[in] context			Pointer to render context
         * @param[in] fnd				Pointer to foundation
         * @param[in] programName		Pointer to string of program name
         * @param[in] separableProgram	True if this is a separable program
         *
         * @return No return.
         */
        NVRenderShaderProgram(NVRenderContextImpl &context, NVFoundationBase &fnd,
                              const char *programName, bool separableProgram);

        /// destructor
        ~NVRenderShaderProgram();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief attach a shader to the program
         *
         * @param[in] pShader		Pointer to shader object
         *
         * @return No return.
         */
        template <typename TShaderObject>
        void Attach(TShaderObject *pShader);

        /**
         * @brief detach a shader from the program
         *
         * @param[in] pShader		Pointer to shader object
         *
         * @return No return.
         */
        template <typename TShaderObject>
        void Detach(TShaderObject *pShader);

        /**
         * @brief link a program
         *
         *
         * @return true if succesfuly linked.
         */
        bool link(QT3DSU32 binaryFormat = 0, const QByteArray *binary = nullptr);

        /**
         * @brief set a shader type
         *
         * @param[in] type		shader type ( graphics or compute )
         *
         * @return No return.
         */
        void SetProgramType(ProgramType::Enum type) { m_ProgramType = type; }
        ProgramType::Enum GetProgramType() const { return m_ProgramType; }

        /**
         * @brief Get Error Message
         *
         * @param[out] messageLength	Pointer to error string
         * @param[out] messageLength	Size of error meesage
         *
         * @return no return.
         */
        void GetErrorMessage(QT3DSI32 *messageLength, const char *errorMessage);

        /**
         * @brief Get Error Message
         *
         *
         * @return error message.
         */
        const char *GetErrorMessage();

        /**
         * @brief Query constant class
         *
         * @param[in] constantName	Pointer to constant name
         *
         * @return return a pointer to a constant class.
         */
        NVRenderShaderConstantBase *GetShaderConstant(const char *constantName);

        /**
         * @brief Query a shader buffer (constant, ... )
         *
         * @param[in] bufferName	Pointer to constant name
         *
         * @return return a pointer to a constant class.
         */
        NVRenderShaderBufferBase *GetShaderBuffer(const char *bufferName);

        // concrete set functions
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, QT3DSI32 inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSI32_2 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSI32_3 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSI32_4 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, QT3DSRenderBool inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const bool_2 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const bool_3 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const bool_4 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSF32 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSVec2 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSVec3 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSVec4 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSU32 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSU32_2 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSU32_3 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSU32_4 &inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSMat33 &inValue,
                              const QT3DSI32 inCount, bool inTranspose = false);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, const QT3DSMat44 &inValue,
                              const QT3DSI32 inCount, bool inTranspose = false);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant,
                              const NVConstDataRef<QT3DSMat44> inValue, const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, NVRenderTexture2D *inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, NVRenderTexture2D **inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant,
                              NVRenderTexture2DArray *inValue, const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, NVRenderTextureCube *inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, NVRenderTextureCube **inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, NVRenderImage2D *inValue,
                              const QT3DSI32 inCount);
        void SetConstantValue(NVRenderShaderConstantBase *inConstant, NVRenderDataBuffer *inValue,
                              const QT3DSI32 inCount);

        /**
         * @brief Template to set constant value via name
         *
         * @param[in] inConstantName	Pointer to constant name
         * @param[in] inValue			Pointer to data
         * @param[in] inCount			Number of elements (array count)
         *
         * @return return a pointer to a constant class.
         */
        template <typename TDataType>
        void SetPropertyValue(const char *inConstantName, const TDataType &inValue,
                              const QT3DSI32 inCount = 1)
        {
            NVRenderShaderConstantBase *theConstant = GetShaderConstant(inConstantName);

            if (theConstant) {
                if (theConstant->GetShaderConstantType()
                    == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
                    SetConstantValue(theConstant, inValue, inCount);
                } else {
                    // Types don't match or property not found
                    QT3DS_ASSERT(false);
                }
            }
        }

        /**
         * @brief Template to set constant value shader constant object
         *
         * @param[in] inConstant	Pointer shader constant object
         * @param[in] inValue		Pointer to data
         * @param[in] inCount		Number of elements (array count)
         *
         * @return return a pointer to a constant class.
         */
        template <typename TDataType>
        void SetPropertyValue(NVRenderShaderConstantBase *inConstant, const TDataType &inValue,
                              const QT3DSI32 inCount = 1)
        {
            if (inConstant) {
                if (inConstant->GetShaderConstantType()
                    == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
                    SetConstantValue(inConstant, inValue, inCount);
                } else {
                    // Types don't match or property not found
                    QT3DS_ASSERT(false);
                }
            }
        }

        virtual void BindComputeInput(NVRenderDataBuffer *inBuffer, QT3DSU32 inIndex);

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendShaderProgramObject GetShaderProgramHandle() const
        {
            return m_ProgramHandle;
        }

        void getProgramBinary(QT3DSU32 &outFormat, QByteArray &outBinary) const
        {
            m_Backend->getProgramBinary(m_ProgramHandle, outFormat, outBinary);
        }

        /**
         * @brief get the context object
         *
         * @return context which this shader belongs to.
         */
        NVRenderContextImpl &GetRenderContext();

        /**
         * @brief Create a shader program
         *
         * @param[in] context						Pointer to context
         * @param[in] programName					Name of the program
         * @param[in] vertShaderSource				Vertex shader source code
         * @param[in] fragShaderSource				Fragment shader source code
         * @param[in] tessControlShaderSource		tessellation control shader source code
         * @param[in] tessEvaluationShaderSource	tessellation evaluation shader source code
         * @param[in] separateProgram				True if this will we a separate
         * program
         * @param[in] type							Binary program type
         * @param[in] binaryProgram					True if program is binary
         *
         * @return a render result
         */
        static NVRenderVertFragCompilationResult Create(
            NVRenderContextImpl &context, const char *programName,
            NVConstDataRef<QT3DSI8> vertShaderSource, NVConstDataRef<QT3DSI8> fragShaderSource,
            NVConstDataRef<QT3DSI8> tessControlShaderSource = NVConstDataRef<QT3DSI8>(),
            NVConstDataRef<QT3DSI8> tessEvaluationShaderSource = NVConstDataRef<QT3DSI8>(),
            NVConstDataRef<QT3DSI8> geometryShaderSource = NVConstDataRef<QT3DSI8>(),
            bool separateProgram = false,
            NVRenderShaderProgramBinaryType::Enum type = NVRenderShaderProgramBinaryType::Unknown,
            bool binaryProgram = false);

        static NVRenderVertFragCompilationResult createFromBinary(
                NVRenderContextImpl &context, const char *programName, QT3DSU32 format,
                const QByteArray &binary);

        /**
         * @brief Create a compute shader program
         *
         * @param[in] context						Pointer to context
         * @param[in] programName					Name of the program
         * @param[in] computeShaderSource			Compute shader source code
         *
         * @return a render result
         */
        static NVRenderVertFragCompilationResult
        CreateCompute(NVRenderContextImpl &context, const char *programName,
                      NVConstDataRef<QT3DSI8> computeShaderSource);
    };

    // Helper class to cache the lookup of shader properties and apply them quickly in a typesafe
    // way.
    template <typename TDataType>
    struct NVRenderCachedShaderProperty
    {
        NVRenderShaderProgram *m_Shader; ///< pointer to shader program
        NVRenderShaderConstantBase *m_Constant; ///< poiner to shader constant object

        NVRenderCachedShaderProperty(const QString &inConstantName, NVRenderShaderProgram &inShader)
            : NVRenderCachedShaderProperty(qPrintable(inConstantName), inShader)
        {
        }

        NVRenderCachedShaderProperty(const char *inConstantName, NVRenderShaderProgram &inShader)
            : m_Shader(&inShader)
            , m_Constant(NULL)
        {
            NVRenderShaderConstantBase *theConstant = inShader.GetShaderConstant(inConstantName);
            if (theConstant) {
                if (theConstant->GetShaderConstantType()
                    == NVDataTypeToShaderDataTypeMap<TDataType>::GetType()) {
                    m_Constant = theConstant;
                } else {
                    // Property types do not match, this probably indicates that the shader changed
                    // while the
                    // code creating this object did not change.
                    QT3DS_ASSERT(false);
                }
            }
        }

        NVRenderCachedShaderProperty()
            : m_Shader(NULL)
            , m_Constant(NULL)
        {
        }

        void Set(const TDataType &inValue)
        {
            if (m_Constant)
                m_Shader->SetPropertyValue(m_Constant, inValue);
        }

        bool IsValid() const { return m_Constant != 0; }
    };

    template <typename TDataType, int size>
    struct NVRenderCachedShaderPropertyArray
    {
        NVRenderShaderProgram *m_Shader; ///< pointer to shader program
        NVRenderShaderConstantBase *m_Constant; ///< poiner to shader constant object
        TDataType m_array[size];

        NVRenderCachedShaderPropertyArray(const QString &inConstantName,
                                          NVRenderShaderProgram &inShader)
            : NVRenderCachedShaderPropertyArray(qPrintable(inConstantName), inShader)
        {

        }

        NVRenderCachedShaderPropertyArray(const char *inConstantName,
                                          NVRenderShaderProgram &inShader)
            : m_Shader(&inShader)
            , m_Constant(nullptr)
        {
            memset(m_array,  0, sizeof(m_array));
            NVRenderShaderConstantBase *theConstant = inShader.GetShaderConstant(inConstantName);
            if (theConstant) {
                if (theConstant->m_ElementCount > 1 && theConstant->m_ElementCount <= size &&
                        theConstant->GetShaderConstantType()
                    == NVDataTypeToShaderDataTypeMap<TDataType*>::GetType()) {
                    m_Constant = theConstant;
                } else {
                    // Property types do not match, this probably indicates that the shader changed
                    // while the code creating this object did not change.
                    QT3DS_ASSERT(false);
                }
            }
        }

        NVRenderCachedShaderPropertyArray()
            : m_Shader(nullptr)
            , m_Constant(nullptr)
        {
            memset(m_array,  0, sizeof(m_array));
        }

        void Set(int count)
        {
            if (m_Constant)
                m_Shader->SetPropertyValue(m_Constant, (TDataType*)m_array, qMin(size, count));
        }

        bool IsValid() const { return m_Constant != 0; }
    };

    // Helper class to cache the lookup of shader properties and apply them quickly in a typesafe
    // way.
    template <typename TDataType>
    struct NVRenderCachedShaderBuffer
    {
        NVRenderShaderProgram *m_Shader; ///< pointer to shader program
        TDataType m_ShaderBuffer; ///< poiner to shader buffer object

        NVRenderCachedShaderBuffer(const char *inShaderBufferName, NVRenderShaderProgram &inShader)
            : m_Shader(&inShader)
            , m_ShaderBuffer(NULL)
        {
            TDataType theShaderBuffer =
                static_cast<TDataType>(inShader.GetShaderBuffer(inShaderBufferName));
            if (theShaderBuffer) {
                m_ShaderBuffer = theShaderBuffer;
            }
        }
        NVRenderCachedShaderBuffer()
            : m_Shader(NULL)
            , m_ShaderBuffer(NULL)
        {
        }

        void Set()
        {
            if (m_ShaderBuffer) {
                m_ShaderBuffer->Validate(m_Shader);
                m_ShaderBuffer->Update();
                m_ShaderBuffer->BindToProgram(m_Shader);
            }
        }

        bool IsValid() const { return m_ShaderBuffer != 0; }
    };
}
}

#endif
