/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSPickFrame.h"
#include "Qt3DSIStateful.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSIScene.h"

namespace qt3ds {
namespace foundation {
    class IInStream;
    class IOutStream;
    class Mutex;
    class CRegisteredString;
}
}

namespace qt3ds {
class Q3DSVariantConfig;
namespace render {
    class IQt3DSRenderContextCore;
    class ILoadedBuffer;
}
}

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class IScene;
class IPresentation;
class CRenderEngine;
class CRenderEngine;
class IUIPParser;
class CFileStream;
class IRuntimeFactory;
class IScriptBridge;
class ISceneManager;

class ISceneBinaryLoader : public qt3ds::foundation::NVRefCounted
{
protected:
    virtual ~ISceneBinaryLoader() {}

public:
    virtual qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8>
    BinaryLoadManagerData(qt3ds::foundation::IInStream &inStream, const char *inBinaryDir) = 0;

    // threadsafe
    // Can be called from any thread
    virtual bool GetBinaryLoadFileName(eastl::string &inPresentationFilename,
                                       eastl::string &outResult) = 0;

    // threadsafe
    // returns a handle to the loaded object. Return value of zero means error.
    virtual qt3ds::QT3DSU32 LoadSceneStage1(qt3ds::foundation::CRegisteredString inPresentationDirectory,
                                      qt3ds::render::ILoadedBuffer &inData) = 0;

    // threadsafe
    // still does not require openGL context but has dependency on a few other things.
    virtual void LoadSceneStage2(qt3ds::QT3DSU32 inSceneHandle, IPresentation &inPresentation,
                                 size_t inElementMemoryOffset, IScriptBridge &inBridge) = 0;
};
struct FacePositionPlanes
{
    enum Enum { XY = 0, YZ, XZ };
};

//==============================================================================
/**
*	@class	ISceneManager
*	@brief	Container class that creates and manages all the Scenes.
*/
class ISceneManager : public qt3ds::foundation::NVRefCounted
{
protected:
    virtual ~ISceneManager(){}

public: // Presentations
    //==============================================================================
    /**
     *	Load a new scene based on an existing presentation.  This will create a new
     *	CScene-based class from the factory method at "CreateScene".  To supply
     *	your own class, simply set "CreateScene" to the desired factory method.
     *	After creating the new scene, a load will be triggered.
     *	@param inPresentation		the current presentation loaded
     */
    virtual IScene *LoadScene(IPresentation *inPresentation, IUIPParser *inParser,
                              IScriptBridge &inBridge,
                              const qt3ds::Q3DSVariantConfig &variantConfig) = 0;
    virtual void LoadRenderPlugin(const CHAR *inAssetIDString, const CHAR *inPath,
                                  const CHAR *inArgs) = 0;

    virtual void LoadQmlStreamerPlugin(const CHAR *inAssetIDString) = 0;

    virtual void BinarySaveManagerData(qt3ds::foundation::IOutStream &inStream,
                                       const char *inBinaryDir) = 0;
    // Loading flow data needs to return the string table memory block.  This allows
    // other systems to use remap their string table strings assuming they had mapped them before
    // serialization.  In general this is a horrible hack and abstraction leak but assuming you have
    // a string
    // table I guess it isn't too bad.
    virtual qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8>
    BinaryLoadManagerData(qt3ds::foundation::IInStream &inStream, const char *inBinaryDir) = 0;

    virtual void BinarySave(Q3DStudio::IScene &inScene) = 0;

public: // Update Cycle
    virtual BOOL Update() = 0;
    virtual BOOL RenderPresentation(IPresentation *inPresentation, bool firstFrame) = 0;
    virtual void OnViewResize(INT32 inViewWidth, INT32 inViewHeight) = 0;
    virtual void GetViewSize(INT32 &outWidth, INT32 &outHeight) = 0;
    virtual void GetActiveSubPresentations(QVector<qt3ds::foundation::CRegisteredString> &subs) = 0;

    virtual STextSizes GetDisplayDimensions(Q3DStudio::IPresentation *inPrimaryPresentation) = 0;

public: // Picking
    virtual Q3DStudio::TElement *UserPick(float mouseX, float mouseY) = 0;
    virtual SPickFrame AdvancePickFrame(const SInputFrame &inInputFrame) = 0;
    // Get the relative UV coord position of an element.  This matches how mouse picking works where
    // we pick out
    virtual qt3ds::foundation::Option<qt3ds::QT3DSVec2>
    FacePosition(Q3DStudio::TElement &inElement, float mouseX, float mouseY,
                 qt3ds::foundation::NVDataRef<Q3DStudio::TElement *> inMapperElements,
                 FacePositionPlanes::Enum inPlane) = 0;

    virtual void Release() = 0;

    static ISceneManager &Create(IRuntimeFactory &inFactory, CRenderEngine &inRenderEngine);
};

} // namespace Q3DStudio
