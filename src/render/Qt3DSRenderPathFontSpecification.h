/****************************************************************************
**
** Copyright (C) 2015 NVIDIA Corporation.
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
****************************************************************************/ #pragma once
#ifndef QT3DS_RENDER_PATH_FONT_SPECIFICATION_H
#define QT3DS_RENDER_PATH_FONT_SPECIFICATION_H
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/StringTable.h"
#include "EASTL/string.h"
#include "render/backends/Qt3DSRenderBackend.h"

    namespace qt3ds
{
    namespace render {

        using namespace foundation;

        class NVRenderContextImpl;
        class NVRenderPathRender;
        class NVRenderPathFontItem;

        class NVRenderPathFontSpecification : public NVRefCounted
        {
            NVRenderContextImpl &m_Context; ///< pointer to context
            NVFoundationBase &m_Foundation; ///< pointer to foundation
            NVRenderBackend *m_Backend; ///< pointer to backend
            volatile QT3DSI32
                mRefCount; ///< Using foundations' naming convention to ease implementation

        public:
            /**
             * @brief constructor
             *
             * @param[in] context		Pointer to render context
             * @param[in] fnd			Pointer to foundation
             * @param[in]  fontName					Name of font ( may include path
             * )
             *
             * @return No return.
             */
            NVRenderPathFontSpecification(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                          CRegisteredString fontName);

            /// @NVRenderPathSpecification destructor
            ~NVRenderPathFontSpecification();

            // define refcount functions
            QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

            /**
             * @brief Load numGlyphs glyphs from specified font file
             *
             * @param[in]  pathBase					Base of path objects
             * @param[in]  fontName					Name of font ( may include path
             * )
             * @param[in]  numGlyphs				Glyph count
             * @param[in]  type						type ( byte, int,... )
             * @param[in]  charCodes				character string
             *
             * @return No return
             */
            virtual void LoadPathGlyphs(const char *fontName, NVRenderPathFormatType::Enum type);

            /**
             * @brief Render a stencil fill pass for fonts
             *
             * @param[in] inPathFontSpec		Pointer to NVRenderPathFontSpecification
             *
             * @return no return
             */
            void StencilFillPathInstanced(NVRenderPathFontItem &inPathFontItem);

            /**
             * @brief Render a cover fill pass for fonts
             *
             * @param[in] inPathFontSpec		Pointer to NVRenderPathFontSpecification
             *
             * @return no return
             */
            void CoverFillPathInstanced(NVRenderPathFontItem &inPathFontItem);

            /**
             * @brief get type for font path set
             *
             * @return path font type
             */
            NVRenderPathFormatType::Enum GetPathFontType() { return m_Type; }

            /**
             * @brief get font glyph count
             *
             * @return get glyph count
             */
            QT3DSU32 GetFontGlyphsCount() { return m_NumFontGlyphs; }

            /**
             * @brief get spacing for char set
             *
             * @return spacing array
             */
            QT3DSF32 GetEmScale() const { return m_EmScale; }

            /**
             * @brief Get font name
             *
             * @return name set
             */
            CRegisteredString GetFontName() const { return m_FontName; }

        private:
            QT3DSU32 m_NumFontGlyphs; ///< glyph count of the entire font set
            QT3DSF32 m_EmScale; ///< true type scale
            NVRenderPathFormatType::Enum m_Type; ///< type ( byte, int,... )
            NVRenderPathTransformType::Enum m_TransformType; ///< transform type default 2D
            CRegisteredString m_FontName; ///< Name of Font
            NVRenderBackend::NVRenderBackendPathObject
                m_PathRenderHandle; ///< opaque backend handle

        private:
            /**
             * @brief Get size of type
             *
             * @param[in]  type						type ( byte, int,... )
             *
             * @return true if successful
             */
            QT3DSU32 getSizeofType(NVRenderPathFormatType::Enum type);

        public:
            static NVRenderPathFontSpecification *
            CreatePathFontSpecification(NVRenderContextImpl &context, CRegisteredString fontName);
        };
    }
}

#endif
