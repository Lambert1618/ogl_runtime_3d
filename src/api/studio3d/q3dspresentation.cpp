/****************************************************************************
**
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

#include "q3dspresentation_p.h"
#include "q3dssceneelement_p.h"
#include "q3dscommandqueue_p.h"
#include "viewerqmlstreamproxy_p.h"
#include "q3dsdatainput_p.h"
#include "q3dsdataoutput_p.h"
#include "q3dsgeometry_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

/*!
    \class Q3DSPresentation
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0

    \brief Represents a Qt 3D Studio presentation.

    This class provides properties and methods for controlling a
    presentation.

    Qt 3D Studio supports multiple presentations in one project. There
    is always a main presentation and zero or more
    subpresentations. The subpresentations are composed into the
    main presentations either as contents of Qt 3D Studio layers or as
    texture maps.

    In the filesystem each presentation corresponds to one \c{.uip}
    presentation file. When present, the \c{.uia} project file ties
    these together by specifying a name for each of the
    (sub-)presentations and specifies which one is the main one.

    The \c{.uia} project also defines \l{DataInput}s and
    \l{DataOutput}s that are exported by the presentations.
    \l{DataInput}s provide a way to provide input to the presentation
    to e.g. control a timeline of a subpresentation from code.
    \c{DataOutput}s provide a way to get notified when an attribute
    is changed in the presentation by animation timeline,
    by behavior scripts or by a \l{DataInput}.

    From the API point of view Q3DSPresentation corresponds to the
    main presentation. The source property can refer either to a
    \c{.uia} or \c{.uip} file. When specifying a file with \c{.uip}
    extension and a \c{.uia} is present with the same name, the
    \c{.uia} is loaded automatically and thus sub-presentation
    information is available regardless.

    \note This class should not be instantiated directly when working with the
    C++ APIs. Q3DSSurfaceViewer creates a Q3DSPresentation
    instance implicitly. This can be queried via
    Q3DSSurfaceViewer::presentation().
 */

/*!
    \qmltype Presentation
    \instantiates Q3DSPresentationItem
    \inqmlmodule QtStudio3D.OpenGL
    \ingroup OpenGLRuntime
    \inherits Q3DSPresentation
    \keyword Studio3D

    \brief Represents a Qt 3D Studio presentation.

    This item provides properties and methods for controlling a
    presentation.

    Qt 3D Studio supports multiple presentations in one project. There
    is always a main presentation and zero or more
    sub-presentations. The sub-presentations are composed into the
    main presentations either as contents of Qt 3D Studio layers or as
    texture maps.

    In the filesystem each presentation corresponds to one \c{.uip}
    presentation file. When present, the \c{.uia} project file ties
    these together by specifying a name for each of the
    (sub-)presentations and specifies which one is the main one.

    The \c{.uia} project also defines \l{DataInput}s and
    \l{DataOutput}s that are exported by the presentations.
    \l{DataInput}s provide a way to provide input to the presentation
    to e.g. control a timeline of a subpresentation from code.
    \l{DataOutput}s provide a way to get notified when an attribute
    is changed in the presentation by animation timeline,
    by behavior scripts or by a \l{DataInput}.

    The Presentation type handles child objects of the types \l Element, \l
    SceneElement, \l DataInput, \l DataOutput, and \l SubPresentationSettings specially. These
    will get automatically associated with the presentation and can control
    certain aspects of it from that point on.

    From the API point of view Presentation corresponds to the
    main presentation. The source property can refer either to a
    \c{.uia} or \c{.uip} file. When specifying a file with \c{.uip}
    extension and a \c{.uia} is present with the same name, the
    \c{.uia} is loaded automatically and thus sub-presentation
    information is available regardless.
 */

/*!
    Constructs a new Q3DSPresentation with the given \a parent.
 */
Q3DSPresentation::Q3DSPresentation(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSPresentationPrivate(this))
{
}

/*!
    Destructor.
 */
Q3DSPresentation::~Q3DSPresentation()
{
}

/*!
    \qmlproperty string Presentation::source

    Holds the name of the main presentation file (\c{*.uia} or
    \c{*.uip}). This may be either a local file or qrc URL.

    The names of all further assets (image files for texture maps, qml
    behavior scripts, mesh files) will be resolved relative to the
    location of the presentation, unless they use absolute paths. This
    allows bundling all assets next to the presentation in the Qt
    resource system.

    Currently set \c{variantList} property will modify which variant groups
    and tags are loaded from the presentations. See
    Q3DSPresentation::variantList property.
*/

/*!
    \property Q3DSPresentation::source

    Holds the name of the main presentation file (\c{*.uia} or
    \c{*.uip}). This may be either a local file or qrc URL.

    The names of all further assets (image files for texture maps, qml
    behavior scripts, mesh files) will be resolved relative to the
    location of the presentation, unless they use absolute paths. This
    allows bundling all assets next to the presentation in the Qt
    resource system.

    Currently set variantList will modify which variant groups
    and tags are loaded from the presentations. See
    Q3DSPresentation::variantList property.
*/
QUrl Q3DSPresentation::source() const
{
    return d_ptr->m_source;
}

void Q3DSPresentation::setSource(const QUrl &source)
{
    if (d_ptr->m_source != source) {
        d_ptr->setSource(source);
        Q_EMIT sourceChanged(source);
    }
}

/*!
    \qmlproperty list<string> Presentation::variantList

    Holds a list of (variant group):(variant) tags that are loaded when the
    \c{source} property is set. If this list is left empty (default), no variant
    filtering is applied and all items are loaded regardless of variant tags in
    the presentation. Variant mechanism allows one presentation project to
    contain multiple variants of the presentation and the decision which variant
    set is loaded is determined during runtime based on the \c{variantList}.

    Variants are divided to variant groups, e.g. one variant group could be
    \c{region} and the variants within that group could be e.g. \c{US, EU, CH}.
    Another variant group could be e.g. \c{power} and variants within that could
    be e.g. \c{gas, electric, diesel}. To filter in this example an electric
    variant for the EU region, the variantList needs to contain two strings
    "region:EU" and "power:electric". Also of course the presentation project
    needs to contain these variant groups and tags applied appropriately to the
    presentation content.

    When variant filters are used, the decision what gets loaded and what is not
    loaded is based on checking every item in the presentation:
    \list
    \li If the item has no variant tags, it will be loaded.
    \li If the item has no tags defined for the checked variant group(s),
        it will be loaded.
    \li If the item has tag(s) for the variant group, any of those tags must
        match any of the variants defined in the filter for that group.
    \endlist

    If the item doesn't fulfill the above rules it will not be loaded.
*/

/*!
    \property Q3DSPresentation::variantList

    Holds a list of (variant group):(variant) tags that are loaded when the
    \c{source} property is set. If this list is left empty (default), no variant
    filtering is applied and all items are loaded regardless of variant tags in
    the presentation. Variant mechanism allows one presentation project to
    contain multiple variants of the presentation and the decision which variant
    set is loaded is determined during runtime based on the \c{variantList}.

    Variants are divided to variant groups, e.g. one variant group could be
    \c{region} and the variants within that group could be e.g. \c{US, EU, CH}.
    Another variant group could be e.g. \c{power} and variants within that could
    be e.g. \c{gas, electric, diesel}. To filter in this example an electric
    variant for the EU region, the variantList needs to contain two strings
    "region:EU" and "power:electric". Also of course the presentation project
    needs to contain these variant groups and tags applied appropriately to the
    presentation content.

    When variant filters are used, the decision what gets loaded and what is not
    loaded is based on checking every item in the presentation:
    \list
    \li If the item has no variant tags, it will be loaded.
    \li If the item has no tags defined for the checked variant group(s),
        it will be loaded.
    \li If the item has tag(s) for the variant group, any of those tags must
        match any of the variants defined in the filter for that group.
    \endlist

    If the item doesn't fulfill the above rules it will not be loaded.
*/
QStringList Q3DSPresentation::variantList() const
{
    return d_ptr->m_variantList;
}

void Q3DSPresentation::setVariantList(const QStringList &variantList)
{
    if (d_ptr->m_variantList != variantList) {
        d_ptr->setVariantList(variantList);
        Q_EMIT variantListChanged(variantList);
    }
}

/*!
    \internal
 */
void Q3DSPresentation::registerElement(Q3DSElement *element)
{
    d_ptr->registerElement(element);
}

/*!
    \internal
 */
void Q3DSPresentation::unregisterElement(Q3DSElement *element)
{
    d_ptr->unregisterElement(element);
}

/*!
    \internal
 */
Q3DSElement *Q3DSPresentation::registeredElement(const QString &elementPath) const
{
    return d_ptr->m_elements.value(elementPath, nullptr);
}

/*!
    \internal
 */
void Q3DSPresentation::registerDataInput(Q3DSDataInput *dataInput)
{
    d_ptr->registerDataInput(dataInput);
}

/*!
    \internal
 */
void Q3DSPresentation::unregisterDataInput(Q3DSDataInput *dataInput)
{
    d_ptr->unregisterDataInput(dataInput);
}

/*!
    \internal
 */
Q3DSDataInput *Q3DSPresentation::registeredDataInput(const QString &name) const
{
    return d_ptr->m_dataInputs.value(name, nullptr);
}

/*!
    \internal
 */
void Q3DSPresentation::registerDataOutput(Q3DSDataOutput *dataOutput)
{
    d_ptr->registerDataOutput(dataOutput);
}

/*!
    \internal
 */
void Q3DSPresentation::unregisterDataOutput(Q3DSDataOutput *dataOutput)
{
    d_ptr->unregisterDataOutput(dataOutput);
}

/*!
    \internal
 */
Q3DSDataOutput *Q3DSPresentation::registeredDataOutput(const QString &name) const
{
    return d_ptr->m_dataOutputs.value(name, nullptr);
}

/*!
    Returns a list of datainputs defined for this presentation. Use setDataInputValue()
    interface to set a datainput value using datainput name, or call Q3DSDataInput::setValue
    directly for a specific datainput.

    \sa setDataInputValue()
    \sa Q3DSDataInput
 */
QVector<Q3DSDataInput *> Q3DSPresentation::dataInputs() const
{
    QVector<Q3DSDataInput *> ret;
    // Just return local datainput list
    const auto datainputs = d_ptr->m_dataInputs;
    for (const auto &it : datainputs)
        ret.append(it);

    return ret;
}

/*!
    \qmlmethod var Presentation::getDataInputs
    Returns a list of datainputs defined for this presentation. Use setDataInputValue()
    interface to set a datainput value using datainput name, or call Q3DSDataInput::setValue
    directly for a specific datainput.

    \sa DataInput
 */

/*!
    Returns a list of datainputs defined for this presentation. Use setDataInputValue()
    interface to set a datainput value using datainput name, or call Q3DSDataInput::setValue
    directly for a specific datainput.

    \sa setDataInputValue()
    \sa Q3DSDataInput
 */
QVariantList Q3DSPresentation::getDataInputs() const
{
    QVariantList ret;
    const auto datainputs = dataInputs();

    for (const auto &it : datainputs)
        ret.append(QVariant::fromValue(it));

    return ret;
}

/*!
    \qmlmethod var Presentation::getDataInputs(string metadataKey)
    Returns a list of datainputs defined for this presentation that have the specified
    \a metadataKey.

    \sa DataInput
 */

/*!
    Returns a list of datainputs defined for this presentation that have the specified
    \a metadataKey.

    \sa setDataInputValue()
    \sa Q3DSDataInput
 */
QVariantList Q3DSPresentation::getDataInputs(const QString &metadataKey) const
{
    QVariantList ret;
    const auto datainputs = dataInputs(metadataKey);

    for (const auto &it : datainputs)
        ret.append(QVariant::fromValue(it));

    return ret;
}

/*!
    Returns a list of datainputs defined for this presentation that have the specified
    \a metadataKey.

    \sa setDataInputValue()
    \sa Q3DSDataInput
 */
QVector<Q3DSDataInput *> Q3DSPresentation::dataInputs(const QString &metadataKey) const
{
    // Defer to presentation item as we want to read metadata from viewer app whenever
    // possible.
    return d_ptr->dataInputs(metadataKey);
}

/*!
    Returns a list of dataoutputs defined for this presentation. Use Qt's connect() method
    to connect slots to the valueChanged() signal in the required \l{DataOutput}s to get notified
    when the value tracked by the DataOutput is changed.

    \sa Q3DSDataOutput
 */
QVector<Q3DSDataOutput *> Q3DSPresentation::dataOutputs() const
{
    QVector<Q3DSDataOutput *> ret;
    const auto datainputs = d_ptr->m_dataOutputs;
    for (const auto &it : datainputs)
        ret.append(it);

    return ret;
}

/*!
    \qmlmethod var Presentation::getDataOutputs

    Returns a list of dataoutputs defined for this presentation. Connect slots to the
    \c{valueChanged()} signal in the required \l{DataOutput}s to get notified
    when the value tracked by the DataOutput is changed.

    \sa Q3DSDataOutput
 */
/*!
 * \brief Q3DSPresentation::getDataOutputs Returns \l{DataOutput}s.
    Returns a list of dataoutputs defined for this presentation. Use Qt's connect() method
    to connect slots to the valueChanged() signal in the required \l{DataOutput}s to get notified
    when the value tracked by the DataOutput is changed.

    \sa Q3DSDataOutput
 */
QVariantList Q3DSPresentation::getDataOutputs() const
{
    QVariantList ret;
    const auto dataoutputs = dataOutputs();

    for (const auto &it : dataoutputs)
        ret.append(QVariant::fromValue(it));

    return ret;
}

/*!
    \qmlproperty bool Presentation::delayedLoading

    This property controls whether the presentation resources are loaded while loading
    the presentation(false) or afterwards when they are actually used in the presentation(true).
    The resources are loaded per slide basis so that all resources required by a slide will be
    loaded at once.

    The resources can be images, subpresentations, materials, effects and meshes.

    Default is \c{false}.
  */

/*!
    \property Q3DSPresentation::delayedLoading

    This property controls whether the presentation resources are loaded while loading
    the presentation(false) or afterwards when they are actually used in the presentation(true).
    The resources are loaded per slide basis so that all resources required by a slide will be
    loaded at once.

    The resources can be images, subpresentations, materials, effects and meshes.

    Default is \c{false}.
  */
bool Q3DSPresentation::delayedLoading() const
{
    return d_ptr->m_delayedLoading;
}

void Q3DSPresentation::setDelayedLoading(bool enable)
{
    if (d_ptr->m_delayedLoading != enable) {
        d_ptr->setDelayedLoading(enable);
        Q_EMIT delayedLoadingChanged(enable);
    }
}

/*!
    \qmlmethod Presentation::preloadSlide(string elementPath)

    Preloads slide resources identified by \a elementPath to memory. All resources required
    by the given slide will load in the background.

    \a elementPath is the identifier of a slide in a presentation and component.

    The presentation, component and slide are separated by ":", for example
    My-presentation:Scene.Layer.Speedometer:Main-slide.

    If \a elementPath does not contain any ":", then it specifies the name of a slide in the
    main presentation and main component. If it contains one ":", it specifies component:slide
    in the main presentation, and if it contains two ":", it
    specifies presentation:component:slide.

    If you want to indicate the main slide in a sub-presentation, you will need to specify the
    full path. For example My-sub-presentation:Scene:Main-slide.

    This function has effect only when delayed loading is enabled.
 */
/*!
    Preloads slide resources identified by \a elementPath to memory. All resources required
    by the given slide will load in the background.

    \a elementPath is the identifier of a slide in a presentation and component.

    The presentation, component and slide are separated by ":", for example
    My-presentation:Scene.Layer.Speedometer:Main-slide.

    If \a elementPath does not contain any ":", then it specifies the name of a slide in the
    main presentation and main component. If it contains one ":", it specifies component:slide
    in the main presentation, and if it contains two ":", it
    specifies presentation:component:slide.

    If you want to indicate the main slide in a sub-presentation, you will need to specify the
    full path. For example My-sub-presentation:Scene:Main-slide.

    This function has effect only when delayed loading is enabled.
 */
void Q3DSPresentation::preloadSlide(const QString &elementPath)
{
    if (d_ptr->m_viewerApp)
        d_ptr->m_viewerApp->preloadSlide(elementPath);
    else if (d_ptr->m_commandQueue)
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_PreloadSlide);
}

/*!
    \qmlmethod Presentation::unloadSlide(string elementPath)

    Unloads slide resources identified by \a elementPath from memory. If the
    slide is current, the resources are unloaded when the slide is changed.

    \a elementPath is the identifier of a slide in a presentation and component.

    The presentation, component and slide are separated by ":", for example
    my-presentation:Scene.Layer.Speedometer:Main-slide.

    If \a elementPath does not contain any ":", then it specifies the name of a slide in the
    main presentation and main component. If it contains one ":", it specifies component:slide
    in the main presentation, and if it contains two ":", it
    specifies presentation:component:slide.

    If you want to indicate the main slide in a sub-presentation, you will need to specify the
    full path. For example My-sub-presentation:Scene:Main-slide.

    This function has effect only when delayed loading is enabled.
 */

/*!
    Unloads slide resources identified by \a elementPath from memory. If the
    slide is current, the resources are unloaded when the slide is changed.

    \a elementPath is the identifier of a slide in a presentation and component.

    The presentation, component and slide are separated by ":", for example
    My-presentation:speedometer:main-slide.

    If \a elementPath does not contain any ":", then it specifies the name of a slide in the
    main presentation and main component. If it contains one ":", it specifies component:slide
    in the main presentation, and if it contains two ":", it
    specifies presentation:Scene.Layer.Speedometer:Main-slide.

    If you want to indicate the main slide in a sub-presentation, you will need to specify the
    full path. For example My-sub-presentation:Scene:Main-slide.

    This function has effect only when delayed loading is enabled.
 */
void Q3DSPresentation::unloadSlide(const QString &elementPath)
{
    if (d_ptr->m_viewerApp)
        d_ptr->m_viewerApp->unloadSlide(elementPath);
    else if (d_ptr->m_commandQueue)
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_UnloadSlide);
}

/*!
    This function is for backwards compatibility. We recommend using \l{DataInput}s to control
    slide changes. \l{DataInput} provides stronger contract between the design and
    code as it avoids use of elementPath (a reference to design's internal structure).

    Requests a time context (a Scene or a Component object) to change
    to a specific slide by \a index. If the context is already on that
    slide, playback will start over.

    If \a elementPath points to a time context, that element is
    controlled. For all other element types the time context owning
    that element is controlled instead.  You can target the command to
    a specific sub-presentation by adding "SubPresentationId:" in
    front of the element path, for example \c{"SubPresentationOne:Scene"}.
 */
void Q3DSPresentation::goToSlide(const QString &elementPath, unsigned int index)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray path(elementPath.toUtf8());
        d_ptr->m_viewerApp->GoToSlideByIndex(path, index);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_GoToSlide, int(index));
    }
}

/*!
    This function is for backwards compatibility. We recommend using \l{DataInput}s to control
    slide changes. \l{DataInput} provides stronger contract between the design and
    code as it avoids use of elementPath (a reference to design's internal structure).

    Requests a time context (a Scene or a Component object) to change
    to a specific slide by \a name. If the context is already on that
    slide, playback will start over.

    If \a elementPath points to a time context, that element is
    controlled. For all other element types the time context owning
    that element is controlled instead.  You can target the command to
    a specific sub-presentation by adding "SubPresentationId:" in
    front of the element path, for example \c{"SubPresentationOne:Scene"}.
 */
void Q3DSPresentation::goToSlide(const QString &elementPath, const QString &name)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray path(elementPath.toUtf8());
        const QByteArray byteName(name.toUtf8());
        d_ptr->m_viewerApp->GoToSlideByName(path, byteName);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_GoToSlideByName, name);
    }
}

/*!
    This function is for backwards compatibility. We recommend using \l{DataInput}s to control
    slide changes. \l{DataInput} provides stronger contract between the design and
    code as it avoids use of elementPath (a reference to design's internal structure).

    Requests a time context (a Scene or a Component object) to change to the
    next or previous slide, depending on the value of \a next. If the context
    is already at the last or first slide, \a wrap defines if wrapping over to
    the first or last slide, respectively, occurs.

    If \a elementPath points to a time context, that element is controlled. For
    all other element types the time context owning that element is controlled
    instead. You can target the command to a specific sub-presentation by
    adding "SubPresentationId:" in front of the element path, for example
    \c{"SubPresentationOne:Scene"}.
 */
void Q3DSPresentation::goToSlide(const QString &elementPath, bool next, bool wrap)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray path(elementPath.toUtf8());
        d_ptr->m_viewerApp->GoToSlideRelative(path, next, wrap);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_GoToSlideRelative,
                                            int(next), int(wrap));
    }
}

/*!
    This function is for backwards compatibility. We recommend using \l{DataInput}s to control
    slide changes. \l{DataInput} provides stronger contract between the design and
    code as it avoids use of elementPath (a reference to design's internal structure).

    Moves the timeline for a time context (a Scene or a Component element) to a
    specific position. The position is given in seconds in \a time.

    If \a elementPath points to a time context, that element is
    controlled. For all other element types the time context owning
    that element is controlled instead.  You can target the command to
    a specific sub-presentation by adding "SubPresentationId:" in
    front of the element path, for example
    \c{"SubPresentationOne:Scene"}.

    The behavior when specifying a time before 0 or after the end time
    for the current slide depends on the play mode of the slide:

    \list
    \li \c{Stop at End} - values outside the valid time range instead clamp to the boundaries.
    For example, going to time -5 is the same as going to time 0.
    \li \c{Looping} - values outside the valid time range mod into the valid range. For example,
    going to time -4 on a 10 second slide is the same as going to time 6.
    \li \c{Ping Pong} - values outside the valid time range bounce off the ends. For example,
    going to time -4 is the same as going to time 4 (assuming the time context is at least 4 seconds
    long), while going to time 12 on a 10 second slide is the same as going to time 8.
    \li \c{Ping} - values less than 0 are treated as time 0, while values greater than the endtime
    bounce off the end (eventually hitting 0.)
    \endlist
 */
void Q3DSPresentation::goToTime(const QString &elementPath, float time)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray path(elementPath.toUtf8());
        d_ptr->m_viewerApp->GoToTime(path, time);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_GoToTime, time);
    }
}

/*!
    This function is for backwards compatibility. We recommend using \l{DataInput}s to control
    attributes in the presentation. \l{DataInput} provides stronger contract between the
    design and code as it avoids use of elementPath (a reference to design's
    internal structure).

    Sets the \a value of an attribute (property) on the object specified by
    \a elementPath. The \a attributeName is the \l{Attribute Names}{scripting
    name} of the attribute.

    An element path refers to an object in the scene by name, for example,
    \c{Scene.Layer.Camera}. Here the right camera object gets chosen even if
    the scene contains other layers with the default camera names (for instance
    \c{Scene.Layer2.Camera}).

    To reference an object stored in a property of another object, the dot
    syntax can be used. The most typical example of this is changing the source
    of a texture map by changing the \c sourcepath property on the object
    selected by \c{SomeMaterial.diffusemap}.

    To access an object in a sub-presentation, prepend the name of the
    sub-presentation followed by a colon, for example,
    \c{SubPresentationOne:Scene.Layer.Camera}.
 */
void Q3DSPresentation::setAttribute(const QString &elementPath, const QString &attributeName,
                                    const QVariant &value)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray path(elementPath.toUtf8());
        const QByteArray name(attributeName.toUtf8());

        QByteArray valueStr;
        float valueFloat;

        const void *theValue = nullptr;
        switch (static_cast<QMetaType::Type>(value.type())) {
        case QMetaType::Bool:
        case QMetaType::Int:
        case QMetaType::Double:
        case QMetaType::Float:
            valueFloat = value.toFloat();
            theValue = &valueFloat;
            break;
        case QMetaType::QString:
        default: // Try string for other types
            valueStr = value.toString().toUtf8();
            theValue = valueStr.constData();
            break;
        }
        d_ptr->m_viewerApp->SetAttribute(path, name, (char *)theValue);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_SetAttribute,
                                            attributeName, value);
    }
}

/*!
    Activate or deactivate the presentation identified by \a id depending
    on the value of \a active.
 */
void Q3DSPresentation::setPresentationActive(const QString &id, bool active)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray presId(id.toUtf8());
        d_ptr->m_viewerApp->SetPresentationActive(presId, active);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(id, CommandType_SetPresentationActive, active);
    }
}

/*!
    Dispatches a Qt 3D Studio presentation event with \a eventName on
    scene object specified by \a elementPath. These events provide a
    way to communicate with the \c .qml based \c{behavior scripts}
    attached to scene objects since they can register to be notified
    via Behavior::registerForEvent().

    See setAttribute() for a description of \a elementPath.
 */
void Q3DSPresentation::fireEvent(const QString &elementPath, const QString &eventName)
{
    if (d_ptr->m_viewerApp) {
        const QByteArray path(elementPath.toUtf8());
        const QByteArray name(eventName.toUtf8());
        d_ptr->m_viewerApp->FireEvent(path, name);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(elementPath, CommandType_FireEvent, eventName);
    }
}

/*!
    Set global animation time to manual value specified by \a milliseconds
    (if non-zero) or resume normal timer (if zero).
 */
void Q3DSPresentation::setGlobalAnimationTime(qint64 milliseconds)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->SetGlobalAnimationTime(milliseconds);
    } else {
        d_ptr->m_commandQueue->m_globalAnimationTimeChanged = true;
        d_ptr->m_commandQueue->m_globalAnimationTime = milliseconds;
    }
}

/*!
    Sets the \a value of a data input element \a name in the presentation.

    Data input provides a higher level, designer-driven alternative to
    Q3DSElement and setAttribute(). Instead of exposing a large set of
    properties with their internal engine names, data input allows designers to
    decide which properties should be writable by the application, and can
    assign custom names to these data input entries, thus forming a
    well-defined contract between the designer and the developer.

    In addition, data input also allows controlling the time line and the
    current slide for time context objects (Scene or Component). Therefore it
    is also an alternative to the goToSlide() and goToTime() family of APIs and
    to Q3DSSceneElement.

    \sa DataInput
 */
void Q3DSPresentation::setDataInputValue(const QString &name, const QVariant &value,
                                         Q3DSDataInput::ValueRole valueRole)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->SetDataInputValue(name, value,
                                              (qt3ds::runtime::DataInputValueRole)valueRole);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(QString(), CommandType_SetDataInputValue,
                                            name, value, static_cast<int>(valueRole));
    }
}

/*!
    Adds a new child element for the element specified by \a parentElementPath to the slide
    specified with \a slideName. Only model and group element creation is currently supported.
    The \a properties hash table specifies the name-value pairs of the properties of the new
    element. The property names are the same the setAttribute() recognizes.

    A referenced material element is also created for the new model element. The source material
    name can be specified with custom "material" attribute in the \a properties hash.
    The source material must exist in the same presentation where the element is created.

    The mesh for a model is specified with the \c sourcepath property. This can be a local file
    path to \c .mesh file, a studio mesh primitive (e.g. \c{#Cube}), or the name of a mesh created
    dynamically with createMesh().

    A property/properties of the element can be bound to be controlled by an existing datainput.
    Control bindings can be indicated with custom "controlledproperty" attribute in the
    \a properties hash. The format for attribute value is "$<datainputname> <attributename>", i.e.
    "$Datainput_1 rotation $Datainput_2 diffusecolor". If datainput name does not match with
    any of the datainputs defined in UIA file, binding has no impact.

    The element is ready for use once elementsCreated() signal is received for it.

    \note If your application is creating and deleting a lot of elements, it is recommended that
    you reuse previously deleted element names when creating new elements.
    This is because the internal string table implementation of Qt 3D Studio ogl-runtime doesn't
    support removing strings for performance reasons, so always using new unique names
    will leak memory.

    \sa createElements()
    \sa createMaterial()
    \sa createMesh()
    \sa elementsCreated()
    \sa setAttribute()
    \sa dataInputs()
 */
void Q3DSPresentation::createElement(const QString &parentElementPath, const QString &slideName,
                                     const QHash<QString, QVariant> &properties)
{
    QVector<QHash<QString, QVariant>> theProperties;
    theProperties << properties;
    createElements(parentElementPath, slideName, theProperties);
}

/*!
    Adds multiple new child elements for the element specified by \a parentElementPath to the slide
    specified with \a slideName. Element properties are specified in \a properties.
    For more details, see createElement().

    \sa createElement()
    \sa elementsCreated()
 */
void Q3DSPresentation::createElements(const QString &parentElementPath, const QString &slideName,
                                      const QVector<QHash<QString, QVariant>> &properties)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->createElements(parentElementPath, slideName, properties);
    } else if (d_ptr->m_commandQueue) {
        // We need to copy the properties map as queue takes ownership of it
        QVector<QHash<QString, QVariant>> *theProperties
                = new QVector<QHash<QString, QVariant>>(properties);
        d_ptr->m_commandQueue->queueCommand(parentElementPath, CommandType_CreateElements,
                                            slideName, theProperties);
    }
}

/*!
    Deletes the element specified by \a elementPath and all its child elements.
    Deleting elements is supported only for elements that have been dynamically created with
    createElement() or createElements().

    \sa deleteElements()
    \sa createElement()
 */
void Q3DSPresentation::deleteElement(const QString &elementPath)
{
    QStringList elementPaths;
    elementPaths << elementPath;
    deleteElements(elementPaths);
}

/*!
    Deletes multiple elements specified by \a elementPaths and all their child elements.
    Deleting elements is supported only for elements that have been dynamically created with
    createElement() or createElements().

    \sa deleteElement()
 */
void Q3DSPresentation::deleteElements(const QStringList &elementPaths)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->deleteElements(elementPaths);
    } else if (d_ptr->m_commandQueue) {
        // We need to copy the list as queue takes ownership of it
        QStringList *theElementPaths = new QStringList(elementPaths);
        d_ptr->m_commandQueue->queueCommand(CommandType_DeleteElements, theElementPaths);
    }
    for (const auto &elementPath : elementPaths)
        d_ptr->m_createdElements.removeAll(elementPath);
}

/*!
    \qmlproperty list<string> Presentation::createdElements
    \readonly

    This property contains a list of all dynamically created elements on this presentation.

    This property is read-only.

    \note Elements can only be dynamically created via C++ API.

    \sa createElement()
    \sa createElements()
*/

/*!
    \property Q3DSPresentation::createdElements

    This property contains a list of all dynamically created elements on this presentation.

    This property is read-only.

    \sa createElement()
    \sa createElements()
*/
QStringList Q3DSPresentation::createdElements() const
{
    return d_ptr->m_createdElements;
}

/*!
    Creates a material specified by the \a materialDefinition parameter into the subpresentation
    specified by the \a subPresId parameter. If \a subPresId is empty, the material
    is created into the main presentation.

    The \a materialDefinition parameter can contain either the file path to a Qt 3D Studio
    material definition file or the actual material definition in the
    Qt 3D Studio material definition format. The material definition is a XML file that specifies
    the material properties. They are not meant to be hand crafted - to create one, simply create
    a new basic material in the Qt 3D Studio editor and edit the material properties in the
    inspector. The properties are stored into \c{materialName.materialdef} file in \c materials
    folder of the project, which you can include into the resources of you application and
    use with this method.

    After creation, the material can be used for new elements created via createElement() by
    setting \c material property of the new element to the name of the created material.
    The material name is specified by the \c name property of the material definition.

    The material is ready for use once materialsCreated() signal is received for it.

    \note If your application is creating and deleting a lot of materials, it is recommended that
    you reuse previously deleted material names when creating new materials.
    This is because the internal string table implementation of Qt 3D Studio ogl-runtime doesn't
    support removing strings for performance reasons, so always using new unique names
    will leak memory.

    \note Creating materials that utilise custom shaders with mipmapped textures can in some cases
    corrupt the textures on other elements if the same textures are already used by existing basic
    materials in the scene, as basic materials do not create mipmaps for their textures.
    Typical symptom of this is black texture on another element after creating a new element using
    the custom material.

    \sa createMaterials()
    \sa createElement()
    \sa materialsCreated()
 */
void Q3DSPresentation::createMaterial(const QString &materialDefinition,
                                      const QString &subPresId)
{
    QStringList materialDefinitions;
    materialDefinitions << materialDefinition;
    createMaterials(materialDefinitions, subPresId);
}

/*!
    Creates multiple materials specified by the \a materialDefinitions parameter into the
    subpresentation specified by the \a subPresId parameter. If \a subPresId is empty,
    the materials are created into the main presentation.

    For more details, see createMaterial().

    \sa createMaterial()
    \sa materialsCreated()
 */
void Q3DSPresentation::createMaterials(const QStringList &materialDefinitions,
                                       const QString &subPresId)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->createMaterials(subPresId, materialDefinitions);
    } else if (d_ptr->m_commandQueue) {
        // We need to copy the list as queue takes ownership of it
        QStringList *theMaterialDefinitions = new QStringList(materialDefinitions);
        d_ptr->m_commandQueue->queueCommand(subPresId, CommandType_CreateMaterials,
                                            theMaterialDefinitions);
    }
}

/*!
    Deletes the material specified by \a materialName from the presentation.
    To delete material from a subpresentation, prefix \a materialName with the subpresentation ID
    similarly to the element paths. For example: \c{"SubPresentationOne:MyMaterial"}.

    Deleting materials is supported only for materials that have been dynamically created with
    createMaterial() or createMaterials().

    \sa deleteMaterials()
    \sa createMaterial()
 */
void Q3DSPresentation::deleteMaterial(const QString &materialName)
{
    QStringList materialNames;
    materialNames << materialName;
    deleteMaterials(materialNames);
}

/*!
    Deletes materials specified by \a materialNames from the presentation.
    To delete material from a subpresentation, prefix the material name with the subpresentation ID
    similarly to the element paths. For example: \c{"SubPresentationOne:MyMaterial"}.

    Deleting materials is supported only for materials that have been dynamically created with
    createMaterial() or createMaterials().

    \sa deleteMaterial()
 */
void Q3DSPresentation::deleteMaterials(const QStringList &materialNames)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->deleteMaterials(materialNames);
    } else if (d_ptr->m_commandQueue) {
        // We need to copy the list as queue takes ownership of it
        QStringList *theMaterialNames = new QStringList(materialNames);
        d_ptr->m_commandQueue->queueCommand(CommandType_DeleteMaterials, theMaterialNames);
    }
    for (const auto &name : materialNames)
        d_ptr->m_createdMaterials.removeAll(name);
}

/*!
    \qmlproperty list<string> Presentation::createdMaterials
    \readonly

    This property contains a list of all dynamically created materials on this presentation.

    This property is read-only.

    \note Materials can only be dynamically created via C++ API.

    \sa createMaterial()
    \sa createMaterials()
*/

/*!
    \property Q3DSPresentation::createdMaterials

    This property contains a list of all dynamically created materials on this presentation.

    This property is read-only.

    \sa createMaterial()
    \sa createMaterials()
*/
QStringList Q3DSPresentation::createdMaterials() const
{
    return d_ptr->m_createdMaterials;
}

/*!
    Creates a mesh specified by given \a geometry. The given \a meshName can be used as
    \c sourcepath property value for model elements created with future createElement() calls.

    The mesh is ready for use once meshesCreated() signal is received for it.

    \sa createElement()
    \sa createMeshes()
    \sa meshesCreated()
*/
void Q3DSPresentation::createMesh(const QString &meshName, const Q3DSGeometry &geometry)
{
    QHash<QString, const Q3DSGeometry *> meshData;
    meshData.insert(meshName, &geometry);
    createMeshes(meshData);
}

/*!
    Creates multiple meshes specified by given \a meshData. The data is mesh name and geometry
    pairs. For more details, see createMesh().

    The ownership of supplied geometries stays with the caller.

    \note If your application is creating and deleting a lot of meshes, it is recommended that
    you reuse previously deleted mesh names when creating new materials.
    This is because the internal string table implementation of \RUNTIME doesn't
    support removing strings for performance reasons, so always using new unique names
    will leak memory.

    \sa createMesh()
    \sa meshesCreated()
*/
void Q3DSPresentation::createMeshes(const QHash<QString, const Q3DSGeometry *> &meshData)
{
    // We can't refer to API class Q3DSGeometry on the runtime side, so let's grab the meshdata
    // from Q3DSGeometryPrivate that is in runtime approved format and pass that on instead
    auto theMeshData = new QHash<QString, Q3DSViewer::MeshData>;
    QHashIterator<QString, const Q3DSGeometry *> it(meshData);
    while (it.hasNext()) {
        it.next();
        theMeshData->insert(it.key(), it.value()->d_ptr->meshData());
    }

    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->createMeshes(*theMeshData);
        delete theMeshData;
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(CommandType_CreateMeshes, theMeshData);
    }
}

/*!
    Deletes the mesh specified by \a meshName.
    Deleting meshes is supported only for meshes that have been dynamically created with
    createMesh() or createMeshes().

    \sa deleteMeshes()
    \sa createMesh()
 */
void Q3DSPresentation::deleteMesh(const QString &meshName)
{
    QStringList meshNames;
    meshNames << meshName;
    deleteMeshes(meshNames);
}

/*!
    Deletes meshes specified by \a meshNames.
    Deleting meshes is supported only for meshes that have been dynamically created with
    createMesh() or createMeshes().

    \sa deleteMesh()
 */
void Q3DSPresentation::deleteMeshes(const QStringList &meshNames)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->deleteMeshes(meshNames);
    } else if (d_ptr->m_commandQueue) {
        // We need to copy the list as queue takes ownership of it
        QStringList *theMeshNames = new QStringList(meshNames);
        d_ptr->m_commandQueue->queueCommand(CommandType_DeleteMeshes, theMeshNames);
    }
    for (const auto &name : meshNames)
        d_ptr->m_createdMeshes.removeAll(name);
}

/*!
    \qmlproperty list<string> Presentation::createdMeshes
    \readonly

    This property contains a list of all dynamically created meshes on this presentation.

    This property is read-only.

    \note Meshes can only be dynamically created via C++ API.

    \sa createMesh()
    \sa createMeshes()
*/

/*!
    \property Q3DSPresentation::createdMeshes

    This property contains a list of all dynamically created meshes on this presentation.

    This property is read-only.

    \sa createMesh()
    \sa createMeshes()
*/
QStringList Q3DSPresentation::createdMeshes() const
{
    return d_ptr->m_createdMeshes;
}

/*!
 * \internal
 */
void Q3DSPresentation::mousePressEvent(QMouseEvent *e)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->HandleMousePress(e->x(), e->y(), e->button(), true);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(QString(), CommandType_MousePress,
                                           e->x(), e->y(), int(e->button()));
    }
}

/*!
 * \internal
 */
void Q3DSPresentation::mouseReleaseEvent(QMouseEvent *e)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->HandleMousePress(e->x(), e->y(), e->button(), false);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(QString(), CommandType_MouseRelease,
                                           e->x(), e->y(), int(e->button()));
    }
}

/*!
 * \internal
 */
void Q3DSPresentation::mouseMoveEvent(QMouseEvent *e)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->HandleMouseMove(e->x(), e->y(), true);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(QString(), CommandType_MouseMove,
                                            e->x(), e->y());
    }
}

/*!
 * \internal
 */
void Q3DSPresentation::wheelEvent(QWheelEvent *e)
{
    QPoint pixelData = e->pixelDelta();
    int numSteps = 0;
    if (pixelData.isNull()) {
        if (e->orientation() == Qt::Vertical)
            numSteps = e->angleDelta().y() / 8;
        else
            numSteps = e->angleDelta().x() / 8;
    } else {
        // trackpad, pixel = one step in scroll wheel.
        if (e->orientation() == Qt::Vertical)
            numSteps = pixelData.y();
        else
            numSteps = pixelData.x();
    }
    if (numSteps != 0) {
        if (d_ptr->m_viewerApp) {
            d_ptr->m_viewerApp->HandleMouseWheel(e->x(), e->y(),
                                                 e->orientation() == Qt::Vertical ? 0 : 1,
                                                 numSteps);
        } else if (d_ptr->m_commandQueue) {
            d_ptr->m_commandQueue->queueCommand(QString(), CommandType_MouseWheel,
                                                e->x(), e->y(),
                                                int(e->orientation() == Qt::Vertical), numSteps);
        }
    }
}

/*!
 * \internal
 */
void Q3DSPresentation::keyPressEvent(QKeyEvent *e)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->HandleKeyInput(d_ptr->getScanCode(e), true);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(QString(), CommandType_KeyPress,
                                            d_ptr->getScanCode(e));
    }
}

/*!
 * \internal
 */
void Q3DSPresentation::keyReleaseEvent(QKeyEvent *e)
{
    if (d_ptr->m_viewerApp) {
        d_ptr->m_viewerApp->HandleKeyInput(d_ptr->getScanCode(e), false);
    } else if (d_ptr->m_commandQueue) {
        d_ptr->m_commandQueue->queueCommand(QString(), CommandType_KeyRelease,
                                            d_ptr->getScanCode(e));
    }
}

// #TODO: QT3DS-3562 Most Presentation signals missing documentation
/*!
    \qmlsignal Presentation::slideEntered(string elementPath, int index, string name)
    //! TODO
 */

/*!
    \fn Q3DSPresentation::slideEntered(const QString &elementPath, unsigned int index, const QString &name)
    //! TODO
 */

/*!
    \qmlsignal Presentation::slideExited(string elementPath, int index, string name)
    //! TODO
 */

/*!
    \fn Q3DSPresentation::slideExited(const QString &elementPath, unsigned int index, const QString &name)
    //! TODO
*/

/*!
    \fn Q3DSPresentation::dataInputsReady()

    Emitted when \l{DataInput}s in the Studio project have been parsed and data inputs are available
    through dataInputs() and getDataInputs() methods.
*/

/*!
    \fn Q3DSPresentation::dataOutputsReady()

    Emitted when \l{DataOutput}s in the Studio project have been parsed and data outputs are available
    through dataOutputs() and getDataOutputs() methods.
 */

/*!
    \qmlsignal Presentation::customSignalEmitted(string elementPath, string name)
    //! TODO
 */

/*!
    \fn Q3DSPresentation::customSignalEmitted(const QString &elementPath, const QString &name)
    //! TODO
 */

/*!
    \qmlsignal Presentation::elementsCreated(list<string> elementPaths, string error)

    Emitted when one or more elements have been created in response to createElement()
    or createElements() calls. The \a elementPaths list contains the element paths of the created
    elements.

    If creation failed, \a error string indicates the reason.

    \sa createElement()
    \sa createElements()
 */

/*!
    \fn Q3DSPresentation::elementsCreated(const QStringList &elementPaths, const QString &error)

    Emitted when one or more elements have been created in response to createElement()
    or createElements() calls. The \a elementPaths list contains the element paths of the created
    elements. If creation failed, \a error string indicates the reason.

    \sa createElement()
    \sa createElements()
 */

/*!
    \qmlsignal Presentation::materialsCreated(list<string> materialNames, string error)

    Emitted when one or more materials have been created in response to createMaterial()
    or createMaterials() calls. The \a materialNames list contains the names of the created
    materials. If the material is created into a subpresentation, the name is prefixed with
    subpresentation ID followed by a colon.

    If creation failed, \a error string indicates the reason.

    \sa createMaterial()
    \sa createMaterials()
 */

/*!
    \fn Q3DSPresentation::materialsCreated(const QStringList &materialNames, const QString &error)

    Emitted when one or more materials have been created in response to createMaterial()
    or createMaterials() calls. The \a materialNames list contains the names of the created
    materials.

    If creation failed, \a error string indicates the reason.

    \sa createMaterial()
    \sa createMaterials()
 */

/*!
    \qmlsignal Presentation::meshesCreated(list<string> meshNames, string error)

    Emitted when one or more meshes have been created in response to createMesh()
    or createMeshes() calls. The \a meshNames list contains the names of the created
    meshes.

    If creation failed, \a error string indicates the reason.

    \sa createMesh()
    \sa createMeshes()
 */

/*!
    \fn Q3DSPresentation::meshesCreated(const QStringList &meshNames, const QString &error)

    Emitted when one or more meshes have been created in response to createMesh()
    or createMeshes() calls. The \a meshNames list contains the names of the created
    meshes.

    If creation failed, \a error string indicates the reason.

    \sa createMesh()
    \sa createMeshes()
 */

/*!
 * \internal
 */
Q3DSPresentationPrivate::Q3DSPresentationPrivate(Q3DSPresentation *q)
    : QObject(q)
    , q_ptr(q)
    , m_viewerApp(nullptr)
    , m_commandQueue(nullptr)
    , m_streamProxy(nullptr)
    , m_delayedLoading(false)
{
}

Q3DSPresentationPrivate::~Q3DSPresentationPrivate()
{
    unregisterAllElements();
    unregisterAllDataInputs();
    unregisterAllDataOutputs();
    delete m_streamProxy;
}

void Q3DSPresentationPrivate::setSource(const QUrl &source)
{
    m_source = source;
    if (m_commandQueue) {
        m_commandQueue->m_sourceChanged = true;
        m_commandQueue->m_source = source;
    }
}

void Q3DSPresentationPrivate::setVariantList(const QStringList &variantList)
{
    m_variantList = variantList;
    if (m_commandQueue) {
        m_commandQueue->m_variantListChanged = true;
        m_commandQueue->m_variantList = variantList;
    }
}

void Q3DSPresentationPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app, bool connectApp)
{
    Q3DSViewer::Q3DSViewerApp *oldApp = m_viewerApp;
    m_viewerApp = app;

    const auto elements = m_elements.values();
    for (Q3DSElement *element : elements)
        element->d_ptr->setViewerApp(app);

    if (m_viewerApp) {
        const auto dataInputs = m_viewerApp->dataInputs();
        for (const auto &name : dataInputs) {
            if (!m_dataInputs.contains(name)) {
                // Name is sufficient for C++ side APIs, as other parameters
                // (max/min) are queried synchronously.
                auto *di = new Q3DSDataInput(name, nullptr);
                registerDataInput(di);
            }
        }
        Q_EMIT q_ptr->dataInputsReady();
    }

    if (connectApp) {
        if (app) {
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigSlideEntered,
                    this, &Q3DSPresentationPrivate::handleSlideEntered);
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigSlideExited,
                    q_ptr, &Q3DSPresentation::slideExited);
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigCustomSignal,
                    q_ptr, &Q3DSPresentation::customSignalEmitted);
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigDataOutputValueUpdated,
                    this, &Q3DSPresentationPrivate::handleDataOutputValueUpdate);
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigElementsCreated,
                    this, &Q3DSPresentationPrivate::handleElementsCreated);
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigMaterialsCreated,
                    this, &Q3DSPresentationPrivate::handleMaterialsCreated);
            connect(app, &Q3DSViewer::Q3DSViewerApp::SigMeshesCreated,
                    this, &Q3DSPresentationPrivate::handleMeshesCreated);
        }
        if (oldApp) {
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigSlideEntered,
                       this, &Q3DSPresentationPrivate::handleSlideEntered);
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigSlideExited,
                       q_ptr, &Q3DSPresentation::slideExited);
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigCustomSignal,
                       q_ptr, &Q3DSPresentation::customSignalEmitted);
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigDataOutputValueUpdated,
                       this, &Q3DSPresentationPrivate::handleDataOutputValueUpdate);
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigElementsCreated,
                       this, &Q3DSPresentationPrivate::handleElementsCreated);
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigMaterialsCreated,
                       this, &Q3DSPresentationPrivate::handleMaterialsCreated);
            disconnect(oldApp, &Q3DSViewer::Q3DSViewerApp::SigMeshesCreated,
                       this, &Q3DSPresentationPrivate::handleMeshesCreated);
        }
    }
}

void Q3DSPresentationPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;

    const auto elements = m_elements.values();
    const auto dataInputs = m_dataInputs.values();
    for (Q3DSElement *element : elements)
        element->d_ptr->setCommandQueue(queue);
    for (Q3DSDataInput *di : dataInputs)
        di->d_ptr->setCommandQueue(queue);

    if (m_commandQueue) {
        setDelayedLoading(m_delayedLoading);
        setVariantList(m_variantList);
        // Queue a request ASAP for datainputs and outputs defined in UIA file so that
        // getDataInputs has up-to-date info at the earliest and that data outputs
        // connect from source to destination
        m_commandQueue->queueCommand({}, CommandType_RequestDataInputs);
        m_commandQueue->queueCommand({}, CommandType_RequestDataOutputs);
        setSource(m_source);
    }
}

void Q3DSPresentationPrivate::setDelayedLoading(bool enable)
{
    m_delayedLoading = enable;
    if (m_commandQueue) {
        m_commandQueue->m_delayedLoading = enable;
        m_commandQueue->m_delayedLoadingChanged = true;
    }
}

void Q3DSPresentationPrivate::requestResponseHandler(CommandType commandType, void *requestData)
{
    switch (commandType) {
    case CommandType_RequestDataInputs: {
        QVariantList *response = reinterpret_cast<QVariantList *>(requestData);

        for (int i = 0; i < response->size(); ++i) {
            // Check and append to QML-side list if the (UIA) presentation has additional datainputs
            // that are not explicitly defined in QML code.
            auto receivedDI = response->at(i).value<Q3DSDataInput *>();
            // For QML behind async command queue, we cache min/max and metadata values in addition
            // to name, in order to be able to return values initially set in UIA file (in QML
            // getters).
            if (!m_dataInputs.contains(receivedDI->name())) {
                auto newDI = new Q3DSDataInput(receivedDI->name(), nullptr);
                newDI->d_ptr->m_min = receivedDI->d_ptr->m_min;
                newDI->d_ptr->m_max = receivedDI->d_ptr->m_max;
                newDI->d_ptr->m_metadata = receivedDI->d_ptr->m_metadata;
                registerDataInput(newDI);
            } else {
                m_dataInputs[receivedDI->name()]->d_ptr->m_min = receivedDI->d_ptr->m_min;
                m_dataInputs[receivedDI->name()]->d_ptr->m_max = receivedDI->d_ptr->m_max;
                m_dataInputs[receivedDI->name()]->d_ptr->m_metadata = receivedDI->d_ptr->m_metadata;
            }
        }
        delete response;
        Q_EMIT q_ptr->dataInputsReady();
        break;
    }
    case CommandType_RequestDataOutputs: {
        QVariantList *response = reinterpret_cast<QVariantList *>(requestData);

        for (int i = 0; i < response->size(); ++i) {
            // Check and append to QML-side list if the (UIA) presentation has additional
            // dataoutputs that are not explicitly defined in QML code.
            if (!m_dataOutputs.contains(response->at(i).value<QString>()))
                registerDataOutput(new Q3DSDataOutput(response->at(i).value<QString>(), nullptr));
        }
        delete response;
        Q_EMIT q_ptr->dataOutputsReady();
        break;
    }
    default:
        Q_ASSERT(false);
        break;
    }
}

// Doc note: The ownership of the registered scenes remains with the caller, who needs to
// ensure that registered scenes are alive as long as the presentation is alive.
void Q3DSPresentationPrivate::registerElement(Q3DSElement *element)
{
    Q_ASSERT(!element->elementPath().isEmpty());

    // Allow only single registration for each element path and scene object
    QMutableHashIterator<QString, Q3DSElement *> i(m_elements);
    while (i.hasNext()) {
        i.next();
        if (i.value() == element) {
            // If the same scene object is already registered with different path,
            // remove it from the map to avoid duplication.
            if (i.key() != element->elementPath())
                i.remove();
        } else if (i.key() == element->elementPath()) {
            // If the same element path is registered by another scene object, the old
            // scene object is unregistered.
            i.value()->d_ptr->setViewerApp(nullptr);
            i.value()->d_ptr->setPresentation(nullptr);
            i.remove();
        }
    }

    element->d_ptr->setViewerApp(m_viewerApp);
    element->d_ptr->setCommandQueue(m_commandQueue);
    element->d_ptr->setPresentation(this);

    m_elements.insert(element->elementPath(), element);
}

void Q3DSPresentationPrivate::unregisterElement(Q3DSElement *element)
{
    Q3DSElement *oldScene = m_elements.value(element->elementPath());
    if (oldScene == element) {
        element->d_ptr->setViewerApp(nullptr);
        element->d_ptr->setCommandQueue(nullptr);
        element->d_ptr->setPresentation(nullptr);
        m_elements.remove(element->elementPath());
    }
}

void Q3DSPresentationPrivate::unregisterAllElements()
{
    for (Q3DSElement *element : m_elements.values()) {
        element->d_ptr->setViewerApp(nullptr);
        element->d_ptr->setCommandQueue(nullptr);
        element->d_ptr->setPresentation(nullptr);
    }
    m_elements.clear();
}

void Q3DSPresentationPrivate::registerDataInput(Q3DSDataInput *dataInput)
{
    Q_ASSERT(!dataInput->name().isEmpty());

    // Allow only single registration for each DataInput
    QMutableHashIterator<QString, Q3DSDataInput *> i(m_dataInputs);
    while (i.hasNext()) {
        i.next();
        if (i.value() == dataInput) {
            // If the same DataInput object is already registered with different name,
            // remove it from the map to avoid duplication.
            if (i.key() != dataInput->name())
                i.remove();
        } else if (i.key() == dataInput->name()) {
            // If the same name is registered by another DataInput object, the old
            // DataInput object is unregistered.
            i.value()->d_ptr->setViewerApp(nullptr);
            i.value()->d_ptr->setPresentation(nullptr);
            i.remove();
        }
    }

    dataInput->d_ptr->setPresentation(q_ptr);
    dataInput->d_ptr->setViewerApp(m_viewerApp);
    dataInput->d_ptr->setCommandQueue(m_commandQueue);

    m_dataInputs.insert(dataInput->name(), dataInput);
}

void Q3DSPresentationPrivate::unregisterDataInput(Q3DSDataInput *dataInput)
{
    Q3DSDataInput *oldDi = m_dataInputs.value(dataInput->name());
    if (oldDi == dataInput) {
        dataInput->d_ptr->setCommandQueue(nullptr);
        dataInput->d_ptr->setViewerApp(nullptr);
        dataInput->d_ptr->setPresentation(nullptr);
        m_dataInputs.remove(dataInput->name());
    }
}

void Q3DSPresentationPrivate::unregisterAllDataInputs()
{
    for (Q3DSDataInput *di : m_dataInputs.values()) {
        di->d_ptr->setViewerApp(nullptr);
        di->d_ptr->setCommandQueue(nullptr);
        di->d_ptr->setPresentation(nullptr);
    }
    m_dataInputs.clear();
}
bool Q3DSPresentationPrivate::isValidDataInput(const Q3DSDataInput *dataInput) const
{
    // For QML instance separated from runtime engine by command queue,
    // check locally cached list for this datainput (initialised at presentation load).
    if (!m_viewerApp) {
        if (m_dataInputs.contains(dataInput->name()))
            return true;
        else
            return false;
    }

    return m_viewerApp->dataInputs().contains(dataInput->name());
}

float Q3DSPresentationPrivate::dataInputMin(const QString &name) const
{
    // For QML instance separated from runtime engine by command queue,
    // return locally cached value (initialised at presentation load).
    if (!m_viewerApp) {
        if (m_dataInputs.contains(name))
            return m_dataInputs[name]->d_ptr->m_min;
        else
            return 0.0f;
    }
    return m_viewerApp->dataInputMin(name);
}

float Q3DSPresentationPrivate::dataInputMax(const QString &name) const
{
    // For QML instance separated from runtime engine by command queue,
    // return locally cached value (initialised at presentation load).
    if (!m_viewerApp) {
        if (m_dataInputs.contains(name))
            return m_dataInputs[name]->d_ptr->m_max;
        else
            return 0.0f;
    }
    return m_viewerApp->dataInputMax(name);
}

QHash<QString, QString> Q3DSPresentationPrivate::dataInputMetadata(const QString &name) const
{
    // For QML instance separated from runtime engine by command queue,
    // return locally cached value (initialised at presentation load).
    if (!m_viewerApp) {
        if (m_dataInputs.contains(name))
            return m_dataInputs[name]->d_ptr->m_metadata;
        else
            return {};
    }
    return m_viewerApp->dataInputMetadata(name);
}

QVector<Q3DSDataInput *> Q3DSPresentationPrivate::dataInputs(const QString &key) const
{
    QVector<Q3DSDataInput *> ret;
    // For QML instance separated from runtime engine by command queue,
    // return locally cached value (initialised at presentation load).
    if (!m_viewerApp) {
        for (const auto &it : m_dataInputs) {
            if (it->metadataKeys().contains(key))
                ret.append(it);
        }
    } else {
        // Otherwise, defer to viewer app.
        const auto &diList = m_viewerApp->dataInputs();
        // We fetch the metadata(s) from the source (viewer app) but
        // return the corresponding datainput object(s) held by presentation item.
        for (const auto &it : diList) {
            if (m_viewerApp->dataInputMetadata(it).contains(key))
                ret.append(m_dataInputs[it]);
        }
    }

    return ret;
}

void Q3DSPresentationPrivate::registerDataOutput(Q3DSDataOutput *dataOutput)
{
    Q_ASSERT(!dataOutput->name().isEmpty());

    // Allow only single registration for each DataOutput
    QMutableHashIterator<QString, Q3DSDataOutput *> i(m_dataOutputs);
    while (i.hasNext()) {
        i.next();
        if (i.value() == dataOutput) {
            // If the same DataOutput object is already registered with different name,
            // remove it from the map to avoid duplication.
            if (i.key() != dataOutput->name())
                i.remove();
        } else if (i.key() == dataOutput->name()) {
            // If the same name is registered by another DataOutput object, the old
            // DataOutput object is unregistered.
            i.value()->d_ptr->setViewerApp(nullptr);
            i.value()->d_ptr->setPresentation(nullptr);
            i.value()->d_ptr->setCommandQueue(nullptr);
            i.remove();
        }
    }

    dataOutput->d_ptr->setPresentation(q_ptr);
    dataOutput->d_ptr->setViewerApp(m_viewerApp);
    dataOutput->d_ptr->setCommandQueue(m_commandQueue);

    m_dataOutputs.insert(dataOutput->name(), dataOutput);
}

void Q3DSPresentationPrivate::unregisterDataOutput(Q3DSDataOutput *dataOutput)
{
    Q3DSDataOutput *oldDout = m_dataOutputs.value(dataOutput->name());
    if (oldDout == dataOutput) {
        dataOutput->d_ptr->setCommandQueue(nullptr);
        dataOutput->d_ptr->setViewerApp(nullptr);
        dataOutput->d_ptr->setPresentation(nullptr);
        m_dataOutputs.remove(dataOutput->name());
    }
}

void Q3DSPresentationPrivate::unregisterAllDataOutputs()
{
    const auto values = m_dataOutputs.values();
    for (Q3DSDataOutput *dout : values) {
        dout->d_ptr->setViewerApp(nullptr);
        dout->d_ptr->setCommandQueue(nullptr);
        dout->d_ptr->setPresentation(nullptr);
    }
    m_dataOutputs.clear();
}

bool Q3DSPresentationPrivate::isValidDataOutput(const Q3DSDataOutput *dataOutput) const
{
    if (!m_viewerApp)
        return false;

    return m_viewerApp->dataOutputs().contains(dataOutput->name());
}

Q3DStudio::EKeyCode Q3DSPresentationPrivate::getScanCode(QKeyEvent *e)
{
    enum {
        RIGHT_SHIFT = 0x036,
        RIGHT_CTRL = 0x11d,
        RIGHT_ALT = 0x138,
    };

    Qt::Key keyScanCode = static_cast<Qt::Key>(e->key());

    Q3DStudio::EKeyCode newScanCode = Q3DStudio::KEY_NOKEY;
    switch (keyScanCode) {
    case Qt::Key_Down:
        newScanCode = Q3DStudio::KEY_DOWN;
        break;
    case Qt::Key_Up:
        newScanCode = Q3DStudio::KEY_UP;
        break;
    case Qt::Key_Left:
        newScanCode = Q3DStudio::KEY_LEFT;
        break;
    case Qt::Key_Right:
        newScanCode = Q3DStudio::KEY_RIGHT;
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        newScanCode = e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPADENTER
                                                           : Q3DStudio::KEY_RETURN;
        break;
    case Qt::Key_Backspace:
        newScanCode = Q3DStudio::KEY_BACK;
        break;
    case Qt::Key_Tab:
        newScanCode = Q3DStudio::KEY_TAB;
        break;
    case Qt::Key_Escape:
        newScanCode = Q3DStudio::KEY_ESCAPE;
        break;
    case Qt::Key_A:
        newScanCode = Q3DStudio::KEY_A;
        break;
    case Qt::Key_B:
        newScanCode = Q3DStudio::KEY_B;
        break;
    case Qt::Key_C:
        newScanCode = Q3DStudio::KEY_C;
        break;
    case Qt::Key_D:
        newScanCode = Q3DStudio::KEY_D;
        break;
    case Qt::Key_E:
        newScanCode = Q3DStudio::KEY_E;
        break;
    case Qt::Key_F:
        newScanCode = Q3DStudio::KEY_F;
        break;
    case Qt::Key_G:
        newScanCode = Q3DStudio::KEY_G;
        break;
    case Qt::Key_H:
        newScanCode = Q3DStudio::KEY_H;
        break;
    case Qt::Key_I:
        newScanCode = Q3DStudio::KEY_I;
        break;
    case Qt::Key_J:
        newScanCode = Q3DStudio::KEY_J;
        break;
    case Qt::Key_K:
        newScanCode = Q3DStudio::KEY_K;
        break;
    case Qt::Key_L:
        newScanCode = Q3DStudio::KEY_L;
        break;
    case Qt::Key_M:
        newScanCode = Q3DStudio::KEY_M;
        break;
    case Qt::Key_N:
        newScanCode = Q3DStudio::KEY_N;
        break;
    case Qt::Key_O:
        newScanCode = Q3DStudio::KEY_O;
        break;
    case Qt::Key_P:
        newScanCode = Q3DStudio::KEY_P;
        break;
    case Qt::Key_Q:
        newScanCode = Q3DStudio::KEY_Q;
        break;
    case Qt::Key_R:
        newScanCode = Q3DStudio::KEY_R;
        break;
    case Qt::Key_S:
        newScanCode = Q3DStudio::KEY_S;
        break;
    case Qt::Key_T:
        newScanCode = Q3DStudio::KEY_T;
        break;
    case Qt::Key_U:
        newScanCode = Q3DStudio::KEY_U;
        break;
    case Qt::Key_V:
        newScanCode = Q3DStudio::KEY_V;
        break;
    case Qt::Key_W:
        newScanCode = Q3DStudio::KEY_W;
        break;
    case Qt::Key_X:
        newScanCode = Q3DStudio::KEY_X;
        break;
    case Qt::Key_Y:
        newScanCode = Q3DStudio::KEY_Y;
        break;
    case Qt::Key_Z:
        newScanCode = Q3DStudio::KEY_Z;
        break;
    case Qt::Key_0:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD0 : Q3DStudio::KEY_0;
        break;
    case Qt::Key_1:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD1 : Q3DStudio::KEY_1;
        break;
    case Qt::Key_2:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD2 : Q3DStudio::KEY_2;
        break;
    case Qt::Key_3:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD3 : Q3DStudio::KEY_3;
        break;
    case Qt::Key_4:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD4 : Q3DStudio::KEY_4;
        break;
    case Qt::Key_5:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD5 : Q3DStudio::KEY_5;
        break;
    case Qt::Key_6:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD6 : Q3DStudio::KEY_6;
        break;
    case Qt::Key_7:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD7 : Q3DStudio::KEY_7;
        break;
    case Qt::Key_8:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD8 : Q3DStudio::KEY_8;
        break;
    case Qt::Key_9:
        newScanCode =
                e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPAD9 : Q3DStudio::KEY_9;
        break;
    case Qt::Key_Minus:
        newScanCode = e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPADSUBTRACT
                                                           : Q3DStudio::KEY_SUBTRACT;
        break;
    case Qt::Key_Plus:
        newScanCode = e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPADADD
                                                           : Q3DStudio::KEY_EQUALS;
        break;
    case Qt::Key_NumLock:
        newScanCode = Q3DStudio::KEY_NUMLOCK;
        break;
    case Qt::Key_ScrollLock:
        newScanCode = Q3DStudio::KEY_SCROLL;
        break;
    case Qt::Key_CapsLock:
        newScanCode = Q3DStudio::KEY_CAPITAL;
        break;
    case Qt::Key_Pause:
        newScanCode = Q3DStudio::KEY_PAUSE;
        break;
    case Qt::Key_Print:
        newScanCode = Q3DStudio::KEY_PRINTSCREEN;
        break;
    case Qt::Key_Insert:
        newScanCode = Q3DStudio::KEY_INSERT;
        break;
    case Qt::Key_Delete:
        newScanCode = Q3DStudio::KEY_DELETE;
        break;
    case Qt::Key_Home:
        newScanCode = Q3DStudio::KEY_HOME;
        break;
    case Qt::Key_End:
        newScanCode = Q3DStudio::KEY_END;
        break;
    case Qt::Key_PageUp:
        newScanCode = Q3DStudio::KEY_PGUP;
        break;
    case Qt::Key_PageDown:
        newScanCode = Q3DStudio::KEY_PGDN;
        break;
    case Qt::Key_F1:
        newScanCode = Q3DStudio::KEY_F1;
        break;
    case Qt::Key_F2:
        newScanCode = Q3DStudio::KEY_F2;
        break;
    case Qt::Key_F3:
        newScanCode = Q3DStudio::KEY_F3;
        break;
    case Qt::Key_F4:
        newScanCode = Q3DStudio::KEY_F4;
        break;
    case Qt::Key_F5:
        newScanCode = Q3DStudio::KEY_F5;
        break;
    case Qt::Key_F6:
        newScanCode = Q3DStudio::KEY_F6;
        break;
    case Qt::Key_F7:
        newScanCode = Q3DStudio::KEY_F7;
        break;
    case Qt::Key_F8:
        newScanCode = Q3DStudio::KEY_F8;
        break;
    case Qt::Key_F9:
        newScanCode = Q3DStudio::KEY_F9;
        break;
    case Qt::Key_F10:
        newScanCode = Q3DStudio::KEY_F10;
        break;
    case Qt::Key_F11:
        newScanCode = Q3DStudio::KEY_F11;
        break;
    case Qt::Key_F12:
        newScanCode = Q3DStudio::KEY_F12;
        break;
    case Qt::Key_F13:
        newScanCode = Q3DStudio::KEY_F13;
        break;
    case Qt::Key_F14:
        newScanCode = Q3DStudio::KEY_F14;
        break;
    case Qt::Key_QuoteLeft:
        newScanCode = Q3DStudio::KEY_GRAVE;
        break;
    case Qt::Key_Asterisk:
        newScanCode = Q3DStudio::KEY_MULTIPLY;
        break;
    case Qt::Key_BracketRight:
        newScanCode = Q3DStudio::KEY_RBRACKET;
        break;
    case Qt::Key_BracketLeft:
        newScanCode = Q3DStudio::KEY_LBRACKET;
        break;
    case Qt::Key_Semicolon:
        newScanCode = Q3DStudio::KEY_SEMICOLON;
        break;
    case Qt::Key_Comma:
        newScanCode = Q3DStudio::KEY_COMMA;
        break;
    case Qt::Key_Period:
        newScanCode = e->modifiers() == Qt::KeypadModifier ? Q3DStudio::KEY_NUMPADDECIMAL
                                                           : Q3DStudio::KEY_PERIOD;
        break;
    case Qt::Key_Apostrophe:
        newScanCode = Q3DStudio::KEY_APOSTROPHE;
        break;
    case Qt::Key_Slash:
        newScanCode = Q3DStudio::KEY_SLASH;
        break;
    case Qt::Key_Backslash:
        newScanCode = Q3DStudio::KEY_BACKSLASH;
        break;
    case Qt::Key_Equal:
        newScanCode = Q3DStudio::KEY_EQUALS;
        break;
    case Qt::Key_Space:
        newScanCode = Q3DStudio::KEY_SPACE;
        break;
    case Qt::Key_Shift:
        newScanCode =
                e->nativeScanCode() == RIGHT_SHIFT ? Q3DStudio::KEY_RSHIFT : Q3DStudio::KEY_LSHIFT;
        break;
    case Qt::Key_Control:
        newScanCode = e->nativeScanCode() == RIGHT_CTRL ? Q3DStudio::KEY_RCONTROL
                                                        : Q3DStudio::KEY_LCONTROL;
        break;
    case Qt::Key_Alt:
        newScanCode =
                e->nativeScanCode() == RIGHT_ALT ? Q3DStudio::KEY_RALT : Q3DStudio::KEY_LALT;
        break;
    default:
        break;
    }

    return newScanCode;
}

ViewerQmlStreamProxy *Q3DSPresentationPrivate::streamProxy()
{
    if (!m_streamProxy)
        m_streamProxy = new ViewerQmlStreamProxy();
    return m_streamProxy;
}

void Q3DSPresentationPrivate::handleSlideEntered(const QString &elementPath, unsigned int index,
                                                 const QString &name)
{
    Q3DSSceneElement *scene = qobject_cast<Q3DSSceneElement *>(m_elements.value(elementPath));
    if (scene)
        scene->d_func()->handleSlideEntered(index, name);
    Q_EMIT q_ptr->slideEntered(elementPath, index, name);
}

void Q3DSPresentationPrivate::handleDataOutputValueUpdate(const QString &name,
                                                          const QVariant &newValue)
{
    if (!m_dataOutputs.contains(name))
        return;

    Q3DSDataOutput *node = m_dataOutputs[name];
    node->setValue(newValue);
}

void Q3DSPresentationPrivate::handleElementsCreated(const QStringList &elementPaths,
                                                    const QString &error)
{
    if (error.isEmpty())
        m_createdElements << elementPaths;

    Q_EMIT q_ptr->elementsCreated(elementPaths, error);
}

void Q3DSPresentationPrivate::handleMaterialsCreated(const QStringList &materialNames,
                                                     const QString &error)
{
    if (error.isEmpty())
        m_createdMaterials << materialNames;

    Q_EMIT q_ptr->materialsCreated(materialNames, error);
}

void Q3DSPresentationPrivate::handleMeshesCreated(const QStringList &meshNames,
                                                  const QString &error)
{
    if (error.isEmpty())
        m_createdMeshes << meshNames;

    Q_EMIT q_ptr->meshesCreated(meshNames, error);
}

QT_END_NAMESPACE
