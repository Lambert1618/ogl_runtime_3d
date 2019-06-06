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
#ifndef QT3DS_RENDER_PATH_FONT_TEXT_H
#define QT3DS_RENDER_PATH_FONT_TEXT_H
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "EASTL/string.h"
#include "render/backends/Qt3DSRenderBackend.h"

    namespace qt3ds
{
    namespace render {

        using namespace foundation;

        class NVRenderContextImpl;
        class NVRenderPathFontSpecification;

        class NVRenderPathFontItem : public NVRefCounted
        {
            NVFoundationBase &m_Foundation; ///< pointer to foundation
            volatile QT3DSI32
                mRefCount; ///< Using foundations' naming convention to ease implementation

        public:
            /**
             * @brief constructor
             *
             * @param[in] fnd			Pointer to foundation
             *
             * @return No return.
             */
            NVRenderPathFontItem(NVFoundationBase &fnd);

            /// @NVRenderPathFontItem destructor
            ~NVRenderPathFontItem();

            // define refcount functions
            QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

            /**
             * @brief Setup text
             *
             * @param[in] glyphCount		number of glyphs
             * @param[in] glyphIDs			array of glyhp ID's
             * @param[in] type				type ( byte, int,... )
             * @param[in] posArray			array of glyhp positions
             * @param[in] pixelBound		pixel boundary
             * @param[in] logicalBound		logical boundary
             * @param[in] emScale			true type scale
             *
             * @return No return.
             */
            void InitTextItem(size_t glyphCount, const QT3DSU32 *glyphIDs,
                              NVRenderPathFormatType::Enum type, QT3DSF32 *posArray, QT3DSVec2 pixelBound,
                              QT3DSVec2 logicalBound, QT3DSF32 emScale);

            /**
             * @brief get glyph count
             *
             * @return get glyph count
             */
            size_t GetGlyphsCount() { return m_NumGlyphs; }

            /**
             * @brief get spacing for char set
             *
             * @return spacing array
             */
            const QT3DSF32 *GetSpacing() { return m_TranslateXY; }

            /**
             * @brief get name set
             *
             * @return name set
             */
            const void *GetGlyphIDs() { return (void *)m_GlyphIDs; }

            /**
             * @brief Get Y bound of font metric
             *
             * @return transform matrix
             */
            const QT3DSMat44 GetTransform();

        private:
            /**
             * @brief Get size of type
             *
             * @param[in]  type						type ( byte, int,... )
             *
             * @return true if successful
             */
            QT3DSU32 getSizeofType(NVRenderPathFormatType::Enum type);

        private:
            size_t m_NumGlyphs; ///< glyph count
            QT3DSU32 *m_GlyphIDs; ///< array glyph ID's
            QT3DSF32 *
                m_TranslateXY; ///< pointer to arrray for character advance information like kerning
            QT3DSMat44 m_ModelMatrix; ///< Matrix which converts from font space to box space

        public:
            static NVRenderPathFontItem *CreatePathFontItem(NVRenderContextImpl &context);
        };
    }
}

#endif