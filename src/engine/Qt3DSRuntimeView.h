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

#ifndef QT3DS_RUNTIME_VIEW_H
#define QT3DS_RUNTIME_VIEW_H

#include "EnginePrefix.h"
#include "Qt3DSIStateful.h"
#include "foundation/Qt3DSVec4.h"
#include "foundation/Qt3DSOption.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSApplication.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSTimer.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSRenderRuntimeBinding.h"
#include <QtCore/qobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qsurfaceformat.h>

typedef void (*qml_Function)(void *inUserData);

namespace qt3dsimp {
    struct Mesh;
}

class QRuntimeViewSignalProxy : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void SigSlideEntered(const QString &elementPath, unsigned int index, const QString &name);
    void SigSlideExited(const QString &elementPath, unsigned int index, const QString &name);
    void SigCustomSignal(const QString &elementPath, const QString &name);
    void SigPresentationReady();
    void SigElementsCreated(const QStringList &elementPaths, const QString &error);
    void SigMaterialsCreated(const QStringList &materialNames, const QString &error);
    void SigDataOutputValueUpdated(const QString &name, const QVariant &value);
};

namespace qt3ds {
class Qt3DSAssetVisitor;
}

namespace qt3ds {
namespace render {
class NVRenderContext;
}
}

namespace Q3DStudio {

class CTegraInputEngine;
class CTegraRenderEngine;
class IScene;
class CInputEngine;
class IAudioPlayer;

class ITegraRenderStateManager : public IStatefulStackBase
{
protected:
    virtual ~ITegraRenderStateManager() {}
public:
    virtual void SetViewport(INT32 inX, INT32 inY, INT32 inWidth, INT32 inHeight) = 0;
    virtual void SetScissorTestEnabled(bool inValue) = 0;
    virtual void SaveAllState() = 0;
    virtual void RestoreAllState() = 0;
};

struct TegraRenderScaleModes
{
    enum Enum {
        ExactSize = 0, // Ensure the viewport is exactly same size as application
        ScaleToFit = 1, // Resize viewport keeping aspect ratio
        ScaleToFill = 2, // Resize viewport to entire window
    };
};

struct TegraRenderStereoModes
{
    enum Enum {
        Mono,
        TopBottom,
        LeftRight
    };
};

struct TegraRenderShadeModes
{
    enum Enum {
        Shaded = 0, // Geometry is shaded only
        ShadedWireframe = 1, // Wireframe is drawn on top shaded geometry
        Wireframe = 2, // Wireframe only
    };
};

class ITegraApplicationRenderEngine
{
protected:
    virtual ~ITegraApplicationRenderEngine() {}

public:
    virtual void SetViewport(INT32 inX, INT32 inY, INT32 inWidth, INT32 inHeight) = 0;
    virtual void SetApplicationViewport(const qt3ds::render::NVRenderRect &inViewport) = 0;
    virtual void ensureRenderTarget() = 0;
    virtual void CheckResize(bool inForce, IPresentation &inActivePresentation) = 0;
    virtual BOOL LoadShaderCache(const CHAR *inFilePath) = 0;
    virtual void AbandonLoadingImages(IScene &inScene) = 0;
    virtual BOOL IsPickValid(FLOAT &outX, FLOAT &outY,
                             const IPresentation &inPresentation) const = 0;
    virtual void SetScaleMode(TegraRenderScaleModes::Enum inScale) = 0;
    virtual void SetShadeMode(TegraRenderShadeModes::Enum inShade) = 0;
    virtual TegraRenderScaleModes::Enum GetScaleMode() const = 0;

    virtual void SetStereoMode(TegraRenderStereoModes::Enum inMode) = 0;
    virtual TegraRenderStereoModes::Enum GetStereoMode() const = 0;
    virtual void SetStereoEyeSeparation(double separation) = 0;
    virtual double GetStereoEyeSeparation() const = 0;

    // TODO: To be removed, not used anywhere anymore
    void CycleScaleMode()
    {
        int mode = (int)GetScaleMode();
        mode = (mode + 1) % 3;
        SetScaleMode((TegraRenderScaleModes::Enum)mode);
    }

    virtual ITegraRenderStateManager &GetTegraRenderStateManager() = 0;
    virtual qt3ds::render::NVRenderContext &GetRenderContext() = 0;
    virtual void SetMatteColor(qt3ds::foundation::Option<qt3ds::QT3DSVec4> inColor) = 0;
    virtual void setMatteEnabled(bool enable) = 0;
    virtual void EnableRenderRotation(bool inEnable) = 0;
    virtual void SetWriteOutShaderCache(bool inWriteOutShaderCache) = 0;
    virtual void Release() = 0;
    virtual void RenderText2D(FLOAT x, FLOAT y, qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                              const char *text) = 0;
    virtual void RenderGpuProfilerStats(FLOAT x, FLOAT y,
                                        qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) = 0;
};

class IRuntimeView : public qt3ds::foundation::NVRefCounted
{
public:
    virtual ~IRuntimeView(){}

public: // loading
    virtual bool BeginLoad(const QString &sourcePath, const QStringList &variantList) = 0;
    virtual bool HasOfflineLoadingCompleted() = 0;
    virtual bool InitializeGraphics(const QSurfaceFormat &format, bool delayedLoading,
                                    bool initInRenderThread) = 0;
    virtual void connectSignals() = 0;
    virtual void finishAsyncInit() = 0;

    virtual void Cleanup() = 0;

    virtual bool CanRender() = 0;

    virtual void Render() = 0;

    virtual bool WasLastFrameDirty() = 0;

    virtual bool HandleMessage(const QEvent *inEvent) = 0;

    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual bool IsPaused() = 0;
    virtual INT32 GetFrameCount() = 0;
    virtual void showOnScreenStats(bool) = 0;

    virtual void setPresentationId(const QString &id) = 0;

public: // Input engine access
    virtual CInputEngine *GetInputEngine() = 0;
    // Only valid after InitializeGraphics
    virtual ITegraApplicationRenderEngine *GetTegraRenderEngine() = 0;

public:
    virtual void GoToSlideByName(const char *elementPath, const char *slideName) = 0;
    virtual void GoToSlideByIndex(const char *elementPath, const int slideIndex) = 0;
    virtual void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap) = 0;
    virtual bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                              QString &currentName, QString &previousName) = 0;
    virtual void SetPresentationAttribute(const char *presId, const char *, const char *value) = 0;
    virtual void GoToTime(const char *elementPath, const float time) = 0;
    virtual void SetGlobalAnimationTime(qint64 inMilliSecs) = 0;
    virtual void SetDataInputValue(const QString &name, const QVariant &value,
                                   int property) = 0;
    virtual QList<QString> dataInputs() const = 0;
    virtual QList<QString> dataOutputs() const = 0;
    virtual float dataInputMax(const QString &name) const = 0;
    virtual float dataInputMin(const QString &name) const = 0;
    virtual QHash<QString, QString> dataInputMetadata(const QString &name) const = 0;
    virtual void createElements(const QString &parentElementPath, const QString &slideName,
                                const QVector<QHash<QString, QVariant>> &properties) = 0;
    virtual void deleteElements(const QStringList &elementPaths) = 0;
    virtual void createMaterials(const QString &subPresId,
                                 const QStringList &materialDefinitions) = 0;
    virtual void deleteMaterials(const QStringList &materialNames) = 0;
    virtual void createMesh(const QString &name, qt3dsimp::Mesh *mesh) = 0;
    virtual void deleteMeshes(const QStringList &meshNames) = 0;
    virtual void SetAttribute(const char *elementPath, const char *attributeName,
                              const char *value) = 0;
    virtual bool GetAttribute(const char *elementPath, const char *attributeName, void *value) = 0;
    virtual void FireEvent(const char *element, const char *evtName) = 0;
    virtual bool PeekCustomAction(char *&outElementPath, char *&outActionName) = 0;
    virtual bool RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData) = 0;
    virtual void FireEvent(const TEventCommandHash inEventType, eastl::string inArgument) = 0;
    virtual qt3ds::foundation::Option<SPresentationSize> GetPresentationSize() = 0;
    virtual void setAssetVisitor(qt3ds::Qt3DSAssetVisitor *) = 0;
    virtual void preloadSlide(const QString &slide) = 0;
    virtual void unloadSlide(const QString &slide) = 0;
    virtual void setDelayedLoading(bool enable) = 0;

public:
    static IRuntimeView &Create(ITimeProvider &inProvider, IWindowSystem &inWindowSystem,
                                IAudioPlayer *inAudioPlayer = nullptr,
                                QElapsedTimer *startupTimer = nullptr);

public:
    QRuntimeViewSignalProxy *signalProxy();
private:
    QRuntimeViewSignalProxy m_SignalProxy;
};

} // namespace Q3DStudio

#endif // QT3DS_RUNTIME_VIEW_H
