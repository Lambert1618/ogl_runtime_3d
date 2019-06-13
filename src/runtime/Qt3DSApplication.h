/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef QT3DS_APPLICATION_H
#define QT3DS_APPLICATION_H
#include "foundation/Qt3DSRefCounted.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Utils.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSMetadata.h"
#include "QtQml/qjsengine.h"

namespace Q3DStudio {
class IRuntimeFactory;
class IRuntimeFactoryCore;
class CRuntime;
class CPresentation;
class CInputEngine;
class IAudioPlayer;
}

namespace qt3ds {
namespace state {
namespace debugger {
class IDebugger;
class ISceneGraphRuntimeDebugger;
}
}
}

namespace qt3ds {
class Qt3DSAssetVisitor;
namespace runtime {
using namespace qt3ds::foundation;
using namespace qt3ds;

class IElementAllocator;
class IActivityZoneManager;

class CAppStr : public eastl::basic_string<char8_t, ForwardingAllocator>
{
    typedef eastl::basic_string<char8_t, ForwardingAllocator> TBase;

public:
    CAppStr(NVAllocatorCallback &alloc, const char8_t *inStr = NULL);
    CAppStr(const CAppStr &inOther);
    CAppStr();
    CAppStr &operator=(const CAppStr &inOther);
};

class IApplication;

class IAppRunnable : public NVRefCounted
{
public:
    virtual void Run() = 0;
};

struct DataInOutAttribute
{
    QByteArray elementPath;
    QVector<QByteArray> attributeName;
    Q3DStudio::EAttributeType propertyType = Q3DStudio::ATTRIBUTETYPE_NONE;

    bool operator==(const DataInOutAttribute &inOther) const {
        return (inOther.elementPath == elementPath && inOther.attributeName == attributeName
                && inOther.propertyType == propertyType);
    }
};

enum DataInOutType {
    DataInOutTypeInvalid = 0,
    DataInOutTypeRangedNumber,
    DataInOutTypeString,
    DataInOutTypeFloat,
    DataInOutTypeEvaluator,
    DataInOutTypeBoolean,
    DataInOutTypeVector4,
    DataInOutTypeVector3,
    DataInOutTypeVector2,
    DataInOutTypeVariant
};

// Duplicated from Q3DSDataInput class on viewer side
enum class DataInputValueRole {
    Value = 0,
    Min = 1,
    Max = 2
};

struct DataInputDef
{
    QVector<DataInOutAttribute> controlledAttributes;
    DataInOutType type = DataInOutTypeInvalid;
    float min = 0.0f;
    float max = 0.0f;
    QString evaluator;
    QJSValue evalFunc;  // keep both evaluator string and JS function
                        // to avoid having to evaluate string several times
    QVariant value;     // most recently set value
    // evaluator datainputs that need to re-evaluate when this datainput changes value
    QVector<QString> dependents;
    QHash<QString, QString> metadata;
};

struct DataOutputDef
{
    DataInOutAttribute observedAttribute;
    DataInOutType type = DataInOutTypeInvalid;
    QVariant value;     // most recently notified value
    QString name;
    QT3DSU32 observedHandle;
    float min = 0.0f;
    float max = 0.0f;
    Q3DStudio::TComponent *timelineComponent;  // Valid only for @timeline
};

typedef QMap<QString, DataInputDef> DataInputMap;
typedef QMap<QString, DataOutputDef> DataOutputMap;

class QT3DS_AUTOTEST_EXPORT IApplication : public NVRefCounted
{
public:
    virtual Q3DStudio::IRuntimeFactory &GetRuntimeFactory() const = 0;

    virtual void SetFrameCount(Q3DStudio::INT32 inFrameCount) = 0;
    virtual Q3DStudio::INT32 GetFrameCount() = 0;

    // Allow overriding the global wall time. This way animations can be
    // advanced at an arbitrary rate when rendering offscreen. Set to 0 to
    // disable (the default).
    virtual void SetTimeMilliSecs(Q3DStudio::INT64 inMilliSecs) = 0;

    virtual Q3DStudio::INT64 GetTimeMilliSecs() = 0;
    virtual void ResetTime() = 0;

    // Setup during UpdateAndRender.  Returns 0 for first frame.
    virtual double GetMillisecondsSinceLastFrame() = 0;

    virtual Q3DStudio::CInputEngine &GetInputEngine() = 0;

    virtual Q3DStudio::CPresentation *GetPrimaryPresentation() = 0;

    virtual Q3DStudio::CPresentation *GetPresentationById(const char8_t *inId) = 0;

    virtual QList<Q3DStudio::CPresentation *> GetPresentationList() = 0;

    // Update all the presentations and render them.  Called exactly once per frame.
    virtual void UpdateAndRender() = 0;

    virtual bool IsApplicationDirty() = 0;

    virtual void MarkApplicationDirty() = 0;

    virtual Q3DStudio::IAudioPlayer &GetAudioPlayer() = 0;

    virtual bool createSuccessful() = 0;

    virtual DataInputMap &dataInputMap() = 0;
    virtual DataOutputMap &dataOutputMap() = 0;

    virtual QList<QString> dataInputs() const = 0;
    virtual QList<QString> dataOutputs() const = 0;

    virtual float dataInputMax(const QString &name) const = 0;
    virtual float dataInputMin(const QString &name) const = 0;
    virtual QHash<QString, QString> dataInputMetadata(const QString &name) const = 0;

    virtual void setPresentationId(const QString &id) = 0;

    virtual void preloadSlide(const QString &slide) = 0;
    virtual void unloadSlide(const QString &slide) = 0;
    virtual void setDelayedLoading(bool enable) = 0;

    // threadsafe call.
    virtual void QueueForMainThread(IAppRunnable &inRunnable) = 0;

    // The directory that contains the executable and the root resource path
    virtual CRegisteredString GetApplicationDirectory() const = 0;
    // Directory that contained the UIA file.
    virtual CRegisteredString GetProjectDirectory() const = 0;
    // Directory where we will copy shared object files to before we load them.
    // This is specifically for android and the case where the place where the project exists
    // is not always the place where we can load the project.
    virtual CRegisteredString GetDllDir() const = 0;
    virtual void SetDllDir(const char *inDllDir) = 0;
    virtual Q3DStudio::THashValue HashString(const char *inStr) = 0;
    virtual const char *ReverseHash(Q3DStudio::THashValue theValue) = 0;

    virtual Q3DStudio::IRuntimeMetaData &GetMetaData() = 0;
    // Element handles are unique across all presentations.
    virtual Q3DStudio::TElement *GetElementByHandle(Q3DStudio::UINT32 inHandle) = 0;
    // Passing in NULL gets you zero as the return handle.
    virtual Q3DStudio::UINT32 GetHandleForElement(Q3DStudio::TElement *inElement) = 0;

    virtual Q3DStudio::IRuntimeFactoryCore &GetRuntimeFactoryCore() = 0;

    virtual IActivityZoneManager &GetActivityZoneManager() = 0;
    virtual IElementAllocator &GetElementAllocator() = 0;

    // nonblocking call to begin loading, loads uia file alone and returns.
    virtual bool BeginLoad(const QString &sourcePath, const QStringList &variantList) = 0;
    // blocking call to end all loading threads and such/wait till finished
    virtual void EndLoad() = 0;
    // Will EndLoad cause nontrivial blocking.
    // Runs any queued runnables.
    virtual bool HasCompletedLoading() = 0;

    virtual void setAssetVisitor(qt3ds::Qt3DSAssetVisitor *) = 0;

    virtual void ComponentSlideEntered(Q3DStudio::CPresentation *presentation,
                                       Q3DStudio::TElement *component,
                                       const QString &elementPath, int slideIndex,
                                       const QString &slideName) = 0;
    virtual void ComponentSlideExited(Q3DStudio::CPresentation *presentation,
                                      Q3DStudio::TElement *component,
                                      const QString &elementPath, int slideIndex,
                                      const QString &slideName) = 0;

    // will force loading to end if endLoad hasn't been called yet.  Will fire off loading
    // of resources that need to be uploaded to opengl.  Maintains reference to runtime factory
    virtual IApplication &CreateApplication(Q3DStudio::CInputEngine &inInputEngine,
                                            Q3DStudio::IAudioPlayer *inAudioPlayer,
                                            Q3DStudio::IRuntimeFactory &inFactory) = 0;

    // maintains reference to runtime factory core.  AppDir is where the executable is located;
    // the system will expect res directory
    // next to executable.
    static IApplication &CreateApplicationCore(Q3DStudio::IRuntimeFactoryCore &inFactory,
                                               const char8_t *inApplicationDirectory);
    static bool isPickingEvent(Q3DStudio::TEventCommandHash event);
};

bool isImagePath(const QString &path);
}
}

#endif
