/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#ifndef QT3DS_VIEWER_H
#define QT3DS_VIEWER_H

#include "qt3dsruntimeglobal.h"
#include "Qt3DSApplication.h"
#include "Qt3DSInputDefs.h"

#include <string>
#include <vector>
#include <QObject>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE
class QQmlImageProviderBase;
QT_END_NAMESPACE

namespace Q3DStudio {
class IRuntimeView;
class IWindowSystem;
class IAudioPlayer;
}

namespace qt3ds {
class Qt3DSAssetVisitor
{
public:
    virtual void visit(const char *path) = 0;
    virtual void visit(const char *type, const char *id, const char *src, const char *args) = 0;
};
}

namespace Q3DSViewer {

typedef void (*qml_Function)(void *inUserData);

// not must match the NDDRuntime see
// UICQmlEngine.h
struct ViewerCallbackType
{
    enum Enum {
        CALLBACK_ON_INIT = 1,
        CALLBACK_ON_UPDATE = 2,
    };
};

struct ViewerContextType
{
    enum Enum { GL_ES2 = 0, GL_ES3, GL_ES31, GL_ES32, GL_2, GL_3_3, GL_4 };
};

struct ViewerScaleModes
{
    enum Enum {
        ExactSize = 0, // Ensure the viewport is exactly same size as application
        ScaleToFit = 1, // Resize viewport keeping aspect ratio
        ScaleToFill = 2, // Resize viewport to entire window
    };
};

struct ViewerStereoModes
{
    enum Enum {
        Mono,
        TopBottom,
        LeftRight,
        AnaglyphRedCyan,
        AnaglyphGreenMagenta
    };
};

struct ViewerShadeModes
{
    enum Enum {
        Shaded = 0, // Geometry is shaded only
        ShadedWireframe = 1, // Wireframe is drawn on top shaded geometry
        Wireframe = 2, // Wireframe only
    };
};

// Intermediate struct to transfer mesh data between studio3d module and runtime
struct MeshData
{
    // All enums must match the ones defined by Q3DSGeometry class in studio3d module
    enum PrimitiveType { // Must match also internal NVRenderDrawMode
        UnknownType = 0,
        Points,
        LineStrip,
        LineLoop,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles, // Default primitive type
        Patches
    };

    struct Attribute {
        enum Semantic {
            UnknownSemantic = 0,
            IndexSemantic,
            PositionSemantic, // attr_pos
            NormalSemantic,   // attr_norm
            TexCoordSemantic, // attr_uv0
            TangentSemantic,  // attr_textan
            BinormalSemantic  // attr_binormal
        };
        enum ComponentType { // Must match also internal NVRenderComponentTypes
            DefaultType = 0,
            U8Type,
            I8Type,
            U16Type,
            I16Type,
            U32Type, // Default for IndexSemantic
            I32Type,
            U64Type,
            I64Type,
            F16Type,
            F32Type, // Default for other semantics
            F64Type
        };

        int typeSize() const
        {
            switch (componentType) {
            case U8Type:  return 1;
            case I8Type:  return 1;
            case U16Type: return 2;
            case I16Type: return 2;
            case U32Type: return 4;
            case I32Type: return 4;
            case U64Type: return 8;
            case I64Type: return 8;
            case F16Type: return 2;
            case F32Type: return 4;
            case F64Type: return 8;
            default:
                Q_ASSERT(false);
                return 0;
            }
        }

        int componentCount() const
        {
            switch (semantic) {
            case IndexSemantic:    return 1;
            case PositionSemantic: return 3;
            case NormalSemantic:   return 3;
            case TexCoordSemantic: return 2;
            case TangentSemantic:  return 3;
            case BinormalSemantic: return 3;
            default:
                Q_ASSERT(false);
                return 0;
            }
        }

        Semantic semantic = PositionSemantic;
        ComponentType componentType = F32Type;
        int offset = 0;
    };

    static const int MAX_ATTRIBUTES = 6;

    void clear()
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_attributeCount = 0;
        m_primitiveType = Triangles;
    }

    QByteArray m_vertexBuffer;
    QByteArray m_indexBuffer;

    Attribute m_attributes[MAX_ATTRIBUTES];
    int m_attributeCount = 0;
    PrimitiveType m_primitiveType = Triangles;
    int m_stride = 0;
};

class Q3DSViewerAppImpl;
class QT3DS_RUNTIME_API Q3DSViewerApp : public QObject
{
    Q_OBJECT
private:
    Q3DSViewerApp(void *glContext, Q3DStudio::IAudioPlayer *inAudioPlayer,
                  QElapsedTimer *startupTimer);
    ~Q3DSViewerApp();

public:
    /**
     * @brief Initialize viewer app
     *
     * @param[in] winWidth              window width
     * @param[in] winHeight             window height
     * @param[in] format                OpenGL API version
     * @param[in] offscreenID           ID of an OpenGL FBO we should render to by  default
     * @param[in] source                string to presentation source
     *
     * @return true on success
     */
    bool InitializeApp(int winWidth, int winHeight, const QSurfaceFormat& format,
                       int offscreenID, const QString &source,
                       const QStringList &variantList,
                       bool delayedLoading, bool initInRenderThread,
                       const QByteArray &shaderCache,
                       qt3ds::Qt3DSAssetVisitor *assetVisitor = nullptr);

    void connectSignals();
    void finishAsyncInit();
    bool IsInitialised(void);
    void setOffscreenId(int offscreenID);


    /*
     * @brief handle window resize
     *
     * @param[in] width		new width
     * @param[in] height	new size
     *
     * @return no return
     */
    void Resize(int width, int height);

    /*
     * @brief does the actual scene rendering
     *
     * @return true if the scene needs to be rendered again in next frame
     */
    bool Render();

    /*
     * @brief handle keyboard input
     *
     * @param[in] keyEvent		event
     * @param[in] isPressed		true if key pressed
     *
     * @return no return
     */
    void HandleKeyInput(Q3DStudio::EKeyCode inKeyCode, bool isPressed);

    /*
     * @brief handle mouse move input
     *
     * @param[in] x				mouse pos x
     * @param[in] y				mouse pos y
     * @param[in] mouseButton	mouse button id
     * @param[in] isPressed		true if key pressed
     *
     * @return no return
     */
    void HandleMouseMove(int x, int y, bool isPressed);

    /*
     * @brief handle mouse press events
     *
     * @param[in] x				mouse pos x
     * @param[in] y				mouse pos y
     * @param[in] mouseButton	mouse button id
     * @param[in] isPressed		true if key pressed
     *
     * @return no return
     */
    void HandleMousePress(int x, int y, int mouseButton, bool isPressed);

    /*
     * @brief handle mouse wheel events
     *
     * @param[in] x				mouse pos x
     * @param[in] y				mouse pos y
     * @param[in] orientation	0 vertical, 1 horizontal
     * @param[in] numSteps		how much the wheel has turned
     *
     * @return no return
     */
    void HandleMouseWheel(int x, int y, int orientation, int numSteps);

    /*
     * @brief Scale scene to adjsut to window size regarding width and height
     *
     * @param[in] inScale	true if scale false if unscale
     *
     * @return no return
     */
    void SetScaleMode(ViewerScaleModes::Enum inScaleMode);

    ViewerScaleModes::Enum GetScaleMode();

    void SetStereoMode(ViewerStereoModes::Enum inStereoMode);
    ViewerStereoModes::Enum GetStereoMode() const;

    void SetStereoEyeSeparation(double separation);
    double GetStereoEyeSeparation() const;

    void setMatteColor(const QColor &color);
    void setShowOnScreenStats(bool s);

    /*
     * @brief Set the shading mode
     *
     * @param[in] inShadeMode	the selected shade mode
     *
     * @return no return
     */
    void SetShadeMode(ViewerShadeModes::Enum inShadeMode);

    /*
     * @brief Save current render state
     *
     * @return no return
     */
    void SaveState();
    /*
     * @brief Restore current render state
     *
     * @return no return
     */
    void RestoreState();

    /// event call back slots

    /*
    * @brief Go to a slide by name
    *
    * @param[in] elementPath	where to find the element
    * @param[in] slideName		slide name
    *
    * @return no return
    */
    void GoToSlideByName(const char *elementPath, const char *slideName);

    /*
    * @brief Go to a slide by index
    *
    * @param[in] elementPath	where to find the element
    * @param[in] slideIndex		slide index
    *
    * @return no return
    */
    void GoToSlideByIndex(const char *elementPath, const int slideIndex);

    /*
    * @brief Go to relative slide
    *
    * @param[in] elementPath    where to find the element
    * @param[in] next           next or previous
    * @param[in] wrap           wrap to beginning if end is reached
    *
    * @return no return
    */
    void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap);

    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName);

    /*
    * @brief Set presentation active or inactive
    *
    * @param[in] presId         presentation id (name without .uip)
    * @param[in] active         set active or inactive
    *
    * @return no return
    */
    void SetPresentationActive(const char *presId, const bool active);

    /*
    * @brief Go to a certain time
    *
    * @param[in] elementPath	where to find the element
    * @param[in] time			time to move
    *
    * @return no return
    */
    void GoToTime(const char *elementPath, const float time);

    /*
    * @brief Play a sound
    *
    * @param[in] soundPath      sound file to play
    *
    * @return no return
    */
    void PlaySoundFile(const char *soundPath);

    /*
    * @brief Set attribute values
    *
    * @param[in] elementPath	where to find the element
    * @param[in] attributeName	attribute name
    * @param[in] value			new value of the attribute
    *
    * @return no return
    */
    void SetAttribute(const char *elementPath, const char *attributeName, const char *value);

    /*
    * @brief Get attribute values
    *
    * @param[in] elementPath	where to find the element
    * @param[in] attributeName	attribute name
    * @param[out] value			current attribute values
    *
    * @return true if successful
    */
    bool GetAttribute(const char *elementPath, const char *attributeName, void *value);

    /*
    * @brief Fire an event
    *
    * @param[in] elementPath    where to find the element to fire the event on
    * @param[in] evtName        the string that identifies and defines the event
    *
    * @return no return
    */
    void FireEvent(const char *elementPath, const char *evtName);

    /*
    * @brief Peek a custom action from the queue
    *		 Note the action is remvoved from the queue
    *
    * @param[out] outElementPath	element path
    * @param[out] actionName		action name
    *
    * @return true if action available
    */
    bool PeekCustomAction(std::string &outElementPath, std::string &actionName);

    /**
    * @brief Register a callback
    *
    * @param[in] callbackType		callback type
    * @param[in] inCallback		    pointer to callback
    * @param[in] inCallback		    pointer to user data
    *
    * @return  true on success
    */
    bool RegisterScriptCallback(ViewerCallbackType::Enum callbackType,
                                const qml_Function inCallback, void *inUserData);

    bool WasLastFrameDirty();

    int GetWindowHeight();
    int GetWindowWidth();

    void SetGlobalAnimationTime(qint64 inMilliSecs);

    void SetDataInputValue(const QString &name, const QVariant &value,
                           qt3ds::runtime::DataInputValueRole valueRole
                           = qt3ds::runtime::DataInputValueRole::Value);

    QList<QString> dataInputs() const;
    QList<QString> dataOutputs() const;

    float dataInputMax(const QString &name) const;
    float dataInputMin(const QString &name) const;
    QHash<QString, QString> dataInputMetadata(const QString &name) const;

    void createElements(const QString &parentElementPath, const QString &slideName,
                        const QVector<QHash<QString, QVariant>> &properties);
    void deleteElements(const QStringList &elementPaths);
    void createMaterials(const QString &subPresId, const QStringList &materialDefinitions);
    void deleteMaterials(const QStringList &materialNames);
    void createMeshes(const QHash<QString, Q3DSViewer::MeshData> &meshData);
    void deleteMeshes(const QStringList &meshNames);

    void addImageProvider(const QString &providerId, QQmlImageProviderBase *provider);

    uint textureId(const QString &elementPath);
    uint textureId(const QString &elementPath, QSize &size, GLenum &format);

    QString error();

    void setPresentationId(const QString &id);
    void preloadSlide(const QString &slide);
    void unloadSlide(const QString &slide);
    void setDelayedLoading(bool enable);
    void setMatteEnabled(bool enabled);

    QByteArray exportShaderCache(bool binaryShaders);

private:
    /*
     * @brief parse command line arguments this fills in our
     *		  global command line variables
     *
     * @param[in] cmdLineArgs	string to command line parameter
     *
     * @return no return
     */
    void handleCmdLineArguments(std::vector<std::string> &cmdLineArgs);
    /*
     * @brief setup search path.
     *		  This functions setup additonal search path for resoruces
     *
     * @param[in] cmdLineArgs	string to command line parameter
     *
     * @return no return
     */
    void setupSearchPath(std::vector<std::string> &cmdLineArgs);

private:
    Q3DSViewerAppImpl &m_Impl;
    QElapsedTimer *m_startupTimer;
    bool m_presentationReady = false;

public:
    static Q3DSViewerApp &Create(void *glContext, Q3DStudio::IAudioPlayer *inAudioPlayer = nullptr,
                                 QElapsedTimer *startupTimer = nullptr);
    void Release();

Q_SIGNALS:
    void SigSlideEntered(const QString &elementPath, unsigned int index, const QString &name);
    void SigSlideExited(const QString &elementPath, unsigned int index, const QString &name);
    void SigCustomSignal(const QString &elementPath, const QString &name);
    void SigElementsCreated(const QStringList &elementPaths, const QString &error);
    void SigMaterialsCreated(const QStringList &materialNames, const QString &error);
    void SigMeshesCreated(const QStringList &meshNames, const QString &error);
    void SigDataOutputValueUpdated(const QString &name, const QVariant &newValue);
    void SigPresentationReady();
    void SigPresentationLoaded();
    void SigLoadShaderCacheErrors(const QString &errors);
};

} // end namespace

#endif // QT3DS_VIEWER_H
