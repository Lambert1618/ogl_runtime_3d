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

#include "q3dsqmlscript.h"

#include "Qt3DSQmlEngine.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSApplication.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSInputFrame.h"
#include "Qt3DSQmlElementHelper.h"
#include "Qt3DSHash.h"
#include "Qt3DSEulerAngles.h"
#include "Qt3DSMathUtils.h"

#include <QMetaProperty>

using namespace Q3DStudio;
using namespace qt3ds::runtime;

QJSValue argToQJSValue(qt3ds::foundation::IStringTable &strTable,
                       Q3DStudio::UINT8 type, const UVariant &value) {
    switch (type) {
    case ATTRIBUTETYPE_INT32:
    case ATTRIBUTETYPE_HASH:
        return QJSValue(value.m_INT32);
    case ATTRIBUTETYPE_FLOAT:
        return QJSValue(value.m_FLOAT);
    case ATTRIBUTETYPE_BOOL:
        return QJSValue(value.m_INT32 != 0);
    case ATTRIBUTETYPE_STRING:
        return QJSValue(strTable.HandleToStr(value.m_StringHandle));
    case ATTRIBUTETYPE_NONE:
        return QJSValue();
    default:
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Pushed event argument value of unknown type: "<< type;
    }
    return QJSValue();
}

void eventCallback(void *contextData, SEventCommand &event)
{
    auto data = static_cast<Q3DSQmlScript::EventData *>(contextData);
    qt3ds::foundation::IStringTable &strTable(
                event.m_Target->GetBelongedPresentation()->GetStringTable());
    QJSValue arg1 = argToQJSValue(strTable, event.m_Arg1Type, event.m_Arg1);
    QJSValue arg2 = argToQJSValue(strTable, event.m_Arg2Type, event.m_Arg2);
    data->function.call({arg1, arg2});
}

Q3DSQmlScript::Q3DSQmlScript(CQmlEngine &api, Q3DSQmlBehavior &object,
                             TElement &behavior, TElement &owner)
    : QObject(nullptr)
    , m_api(api)
    , m_object(object)
    , m_behavior(behavior)
    , m_owner(owner)
    , m_initialized(false)
    , m_lastActivationState(false)
    , m_deltaTime(0.0f)
    , m_lastTime(0)
{
    qt3ds::foundation::IStringTable &strTable
            = m_behavior.GetBelongedPresentation()->GetStringTable();
    const QMetaObject *meta = object.metaObject();
    for (int i = 0; i < meta->propertyCount(); ++i) {
        QMetaProperty property = meta->property(i);
        QVariant::Type t = property.type();
        if (static_cast<QMetaType::Type>(t) == QMetaType::QVariant) {
            // detect type from behavior property
            auto nameHash = CHash::HashAttribute(property.name());
            Option<element::TPropertyDescAndValuePtr> value = behavior.FindProperty(nameHash);
            if (value.hasValue()) {
                switch (value->first.type()) {
                case ATTRIBUTETYPE_INT32:
                case ATTRIBUTETYPE_HASH:
                    t = QVariant::Int;
                    break;
                case ATTRIBUTETYPE_FLOAT:
                    t = QVariant::Double;
                    break;
                case ATTRIBUTETYPE_BOOL:
                    t = QVariant::Bool;
                    break;
                case ATTRIBUTETYPE_STRING:
                    t = QVariant::String;
                    break;
                case ATTRIBUTETYPE_FLOAT4:
                    t = QVariant::Vector4D;
                    break;
                case ATTRIBUTETYPE_FLOAT3:
                    t = QVariant::Vector3D;
                    break;
                case ATTRIBUTETYPE_FLOAT2:
                    t = QVariant::Vector2D;
                    break;
                default:
                    break;
                }
            } else {
                // detect vectors
                QByteArray name(property.name());
                QByteArray xname = name + QByteArrayLiteral(".x");
                Option<element::TPropertyDescAndValuePtr> xvalue
                        = behavior.FindProperty(CHash::HashAttribute(xname.data()));
                QByteArray rname = name + QByteArrayLiteral(".r");
                Option<element::TPropertyDescAndValuePtr> rvalue
                        = behavior.FindProperty(CHash::HashAttribute(rname.data()));
                if (xvalue.hasValue()) {
                    QByteArray yname = name + QByteArrayLiteral(".y");
                    QByteArray zname = name + QByteArrayLiteral(".z");
                    QByteArray wname = name + QByteArrayLiteral(".w");

                    Option<element::TPropertyDescAndValuePtr> yvalue
                            = behavior.FindProperty(CHash::HashAttribute(yname.data()));
                    Option<element::TPropertyDescAndValuePtr> zvalue
                            = behavior.FindProperty(CHash::HashAttribute(zname.data()));
                    Option<element::TPropertyDescAndValuePtr> wvalue
                            = behavior.FindProperty(CHash::HashAttribute(wname.data()));
                    int count = 1;
                    count += yvalue.hasValue() ? 1 : 0;
                    count += zvalue.hasValue() ? 1 : 0;
                    count += wvalue.hasValue() ? 1 : 0;
                    if (count == 2)
                        t = QVariant::Vector2D;
                    else if (count == 3)
                        t = QVariant::Vector3D;
                    else if (count == 4)
                        t = QVariant::Vector4D;
                } else if (rvalue.hasValue()) {
                    QByteArray gname = name + QByteArrayLiteral(".g");
                    QByteArray bname = name + QByteArrayLiteral(".b");
                    QByteArray aname = name + QByteArrayLiteral(".a");
                    Option<element::TPropertyDescAndValuePtr> gvalue
                            = behavior.FindProperty(CHash::HashAttribute(gname.data()));
                    Option<element::TPropertyDescAndValuePtr> bvalue
                            = behavior.FindProperty(CHash::HashAttribute(bname.data()));
                    Option<element::TPropertyDescAndValuePtr> avalue
                            = behavior.FindProperty(CHash::HashAttribute(aname.data()));
                    int count = 1;
                    count += gvalue.hasValue() ? 1 : 0;
                    count += bvalue.hasValue() ? 1 : 0;
                    count += avalue.hasValue() ? 1 : 0;
                    if (count == 2)
                        t = QVariant::Vector2D;
                    else if (count == 3)
                        t = QVariant::Vector3D;
                    else if (count == 4)
                        t = QVariant::Vector4D;
                }
            }
        }
        switch (t) {
        case QVariant::Bool: {
            const char *name = property.name();
            auto nameHash = CHash::HashAttribute(name);
            Option<element::TPropertyDescAndValuePtr> value = behavior.FindProperty(nameHash);
            if (value.hasValue()) {
                std::function<void()> mapper = [&object, property, value]() -> void {
                    Q3DStudio::UVariant *val = value->second;
                    property.write(&object, QVariant::fromValue<bool>(val->m_INT32 > 0));
                };
                m_mappedProperties.push_back(mapper);
            }
        } break;
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong: {
            const char *name = property.name();
            auto nameHash = CHash::HashAttribute(name);
            Option<element::TPropertyDescAndValuePtr> value = behavior.FindProperty(nameHash);
            if (value.hasValue()) {
                std::function<void()> mapper = [&object, property, value]() {
                    Q3DStudio::UVariant *val = value->second;
                    property.write(&object, QVariant::fromValue(val->m_INT32));
                };
                m_mappedProperties.push_back(mapper);
            }
        } break;
        case QVariant::Double: {
            const char *name = property.name();
            auto nameHash = CHash::HashAttribute(name);
            Option<element::TPropertyDescAndValuePtr> value = behavior.FindProperty(nameHash);
            if (value.hasValue()) {
                std::function<void()> mapper = [&object, property, value]() {
                    Q3DStudio::UVariant *val = value->second;
                    property.write(&object, QVariant::fromValue(val->m_FLOAT));
                };
                m_mappedProperties.push_back(mapper);
            }
        } break;
        case QVariant::String: {
            const char *name = property.name();
            auto nameHash = CHash::HashAttribute(name);
            Option<element::TPropertyDescAndValuePtr> value = behavior.FindProperty(nameHash);
            if (value.hasValue()) {
                std::function<void()> mapper = [&object, property, value, &strTable]() {
                    Q3DStudio::UVariant *val = value->second;
                    property.write(&object, QVariant::fromValue(
                             QString::fromUtf8(strTable.HandleToStr(val->m_StringHandle).c_str())));
                };
                m_mappedProperties.push_back(mapper);
            }
        } break;
        case QVariant::Vector2D: {
            std::function<void()> mapper;
            auto nameHash = CHash::HashAttribute(property.name());
            Option<element::TPropertyDescAndValuePtr> prop = behavior.FindProperty(nameHash);
            if (prop.hasValue() && prop->first.type() == Q3DStudio::ATTRIBUTETYPE_FLOAT2) {
                mapper = [&object, property, prop]() {
                    QVector2D vec;
                    Q3DStudio::UVariant *value = prop->second;
                    vec.setX(value->m_FLOAT3[0]);
                    vec.setY(value->m_FLOAT3[1]);
                    property.write(&object, QVariant::fromValue(vec));
                };
                m_mappedProperties.push_back(mapper);
            } else {
                QByteArray name(property.name());
                QByteArray cname = name + QByteArrayLiteral(".x");
                auto nameHash = CHash::HashAttribute(cname.data());
                Option<element::TPropertyDescAndValuePtr> xvalue = behavior.FindProperty(nameHash);
                if (xvalue.hasValue()) {
                    cname = name + QByteArrayLiteral(".y");
                    nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> yvalue
                            = behavior.FindProperty(nameHash);

                    if (xvalue.hasValue() && yvalue.hasValue()) {
                        mapper = [&object, property, xvalue, yvalue]() {
                            QVector2D vec;
                            vec.setX(xvalue->second->m_FLOAT);
                            vec.setY(yvalue->second->m_FLOAT);
                            property.write(&object, QVariant::fromValue(vec));
                        };
                        m_mappedProperties.push_back(mapper);
                    }
                }
            }
        } break;
        case QVariant::Vector3D: {
            std::function<void()> mapper;
            auto nameHash = CHash::HashAttribute(property.name());
            Option<element::TPropertyDescAndValuePtr> prop = behavior.FindProperty(nameHash);
            if (prop.hasValue() && prop->first.type() == Q3DStudio::ATTRIBUTETYPE_FLOAT3) {
                mapper = [&object, property, prop]() {
                    QVector3D vec;
                    Q3DStudio::UVariant *value = prop->second;
                    vec.setX(value->m_FLOAT3[0]);
                    vec.setY(value->m_FLOAT3[1]);
                    vec.setZ(value->m_FLOAT3[2]);
                    property.write(&object, QVariant::fromValue(vec));
                };
                m_mappedProperties.push_back(mapper);
            } else {
                QByteArray name(property.name());
                QByteArray cname = name + QByteArrayLiteral(".x");
                auto nameHash = CHash::HashAttribute(cname.data());
                Option<element::TPropertyDescAndValuePtr> xvalue = behavior.FindProperty(nameHash);
                if (xvalue.hasValue()) {
                    cname = name + QByteArrayLiteral(".y");
                    nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> yvalue
                            = behavior.FindProperty(nameHash);
                    cname = name + QByteArrayLiteral(".z");
                    nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> zvalue
                            = behavior.FindProperty(nameHash);

                    if (xvalue.hasValue() && yvalue.hasValue() && zvalue.hasValue()) {
                        mapper = [&object, property, xvalue, yvalue, zvalue]() {
                            QVector3D vec;
                            vec.setX(xvalue->second->m_FLOAT);
                            vec.setY(yvalue->second->m_FLOAT);
                            vec.setZ(zvalue->second->m_FLOAT);
                            property.write(&object, QVariant::fromValue(vec));
                        };
                        m_mappedProperties.push_back(mapper);
                    }
                }
            }
        } break;
        case QVariant::Color:
        case QVariant::Vector4D: {
            std::function<void()> mapper;
            auto nameHash = CHash::HashAttribute(property.name());
            Option<element::TPropertyDescAndValuePtr> prop = behavior.FindProperty(nameHash);
            if (prop.hasValue() && prop->first.type() == Q3DStudio::ATTRIBUTETYPE_FLOAT4) {
                mapper = [&object, property, prop]() {
                    QVector4D vec;
                    Q3DStudio::UVariant *value = prop->second;
                    vec.setX(value->m_FLOAT4[0]);
                    vec.setY(value->m_FLOAT4[1]);
                    vec.setZ(value->m_FLOAT4[2]);
                    vec.setW(value->m_FLOAT4[3]);
                    property.write(&object, QVariant::fromValue(vec));
                };
                m_mappedProperties.push_back(mapper);
            } else {
                QByteArray name(property.name());
                QByteArray cname = name + QByteArrayLiteral(".x");
                auto nameHash = CHash::HashAttribute(cname.data());
                Option<element::TPropertyDescAndValuePtr> xvalue = behavior.FindProperty(nameHash);
                if (xvalue.hasValue()) {
                    cname = name + QByteArrayLiteral(".y");
                    nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> yvalue
                            = behavior.FindProperty(nameHash);
                    cname = name + QByteArrayLiteral(".z");
                    nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> zvalue
                            = behavior.FindProperty(nameHash);
                    cname = name + QByteArrayLiteral(".w");
                    nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> wvalue
                            = behavior.FindProperty(nameHash);

                    if (xvalue.hasValue() && yvalue.hasValue() && zvalue.hasValue()
                            && wvalue.hasValue()) {
                        mapper = [&object, property, xvalue, yvalue, zvalue, wvalue]() {
                            QVector4D vec;
                            vec.setX(xvalue->second->m_FLOAT);
                            vec.setY(yvalue->second->m_FLOAT);
                            vec.setZ(zvalue->second->m_FLOAT);
                            vec.setW(wvalue->second->m_FLOAT);
                            property.write(&object, QVariant::fromValue(vec));
                        };
                        m_mappedProperties.push_back(mapper);
                    }
                } else {
                    cname = name + QByteArrayLiteral(".r");
                    auto nameHash = CHash::HashAttribute(cname.data());
                    Option<element::TPropertyDescAndValuePtr> rvalue
                            = behavior.FindProperty(nameHash);
                    if (rvalue.hasValue()) {
                        cname = name + QByteArrayLiteral(".g");
                        nameHash = CHash::HashAttribute(cname.data());
                        Option<element::TPropertyDescAndValuePtr> gvalue
                                = behavior.FindProperty(nameHash);
                        cname = name + QByteArrayLiteral(".b");
                        nameHash = CHash::HashAttribute(cname.data());
                        Option<element::TPropertyDescAndValuePtr> bvalue
                                = behavior.FindProperty(nameHash);
                        cname = name + QByteArrayLiteral(".a");
                        nameHash = CHash::HashAttribute(cname.data());
                        Option<element::TPropertyDescAndValuePtr> avalue
                                = behavior.FindProperty(nameHash);

                        if (rvalue.hasValue() && gvalue.hasValue() && bvalue.hasValue()
                                && avalue.hasValue()) {
                            mapper = [&object, property, rvalue, gvalue, bvalue, avalue]() {
                                QVector4D vec;
                                vec.setX(rvalue->second->m_FLOAT);
                                vec.setY(gvalue->second->m_FLOAT);
                                vec.setZ(bvalue->second->m_FLOAT);
                                vec.setW(avalue->second->m_FLOAT);
                                property.write(&object, QVariant::fromValue(vec));
                            };
                            m_mappedProperties.push_back(mapper);
                        }
                    }
                }
            }
        } break;
        default:
            break;
        }
    }
}

Q3DSQmlScript::~Q3DSQmlScript()
{
    for (const EventCallbackInfo &callback : m_eventCallbacks)
        delete callback.data;
}

void Q3DSQmlScript::update()
{
    updateProperties();

    bool active = m_behavior.GetActive();

    if (active && !m_initialized) {
        m_initialized = true;
        Q_EMIT m_object.initialize();
    }

    TTimeUnit time = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation())->GetTime();
    m_deltaTime = (time - m_lastTime) / 1000.0f;
    m_lastTime = time;

    if (m_lastActivationState != active) {
        if (active)
            Q_EMIT m_object.activate();
        else
            Q_EMIT m_object.deactivate();
    }
    m_lastActivationState = active;
    if (active)
        Q_EMIT m_object.update();
}

void Q3DSQmlScript::call(const QString &function)
{
    const QMetaObject *meta = m_object.metaObject();
    const QString normalized = function + "()";
    if (meta->indexOfMethod(normalized.toUtf8().constData()) != -1)
        QMetaObject::invokeMethod(&m_object, function.toUtf8().constData());
}

void Q3DSQmlScript::updateProperties()
{
    using namespace qt3ds::foundation;
    using namespace qt3ds::runtime::element;

    if (!m_behavior.GetActive() || !m_behavior.IsDirty())
        return;

    for (auto m : qAsConst(m_mappedProperties))
        m();
}

bool Q3DSQmlScript::hasBehavior(const TElement *behavior)
{
    return &m_behavior == behavior;
}

float Q3DSQmlScript::getDeltaTime()
{
    return m_deltaTime;
}

float Q3DSQmlScript::getAttribute(const QString &attribute)
{
    if (!m_behavior.GetActive())
        return 0;

    float floatValue = 0;
    m_api.GetAttribute(&m_owner,
                       attribute.toUtf8().constData(),
                       (char *)&floatValue);
    return floatValue;
}

void Q3DSQmlScript::setAttribute(const QString &attribute, const QVariant &value)
{
    setAttribute("", attribute, value);
}

void Q3DSQmlScript::setAttribute(const QString &handle, const QString &attribute,
                                 const QVariant &value)
{
    if (!m_behavior.GetActive())
        return;

    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return;

    QByteArray valueStr;
    float valueFloat;

    const char *valuePtr = nullptr;
    switch (static_cast<QMetaType::Type>(value.type())) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::Double:
    case QMetaType::Float:
        valueFloat = value.toFloat();
        valuePtr = reinterpret_cast<const char *>(&valueFloat);
        break;
    case QMetaType::QVector2D: {
        QVector2D vec = value.value<QVector2D>();
        float val[2];
        val[0] = vec.x();
        val[1] = vec.y();
        const QByteArray name = attribute.toUtf8();
        QByteArray cname = name + QByteArrayLiteral(".x");
        m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val));
        cname = name + QByteArrayLiteral(".y");
        m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val + 1));
        return;
    }
    case QMetaType::QVector3D: {
        QVector3D vec = value.value<QVector3D>();
        float val[3];
        val[0] = vec.x();
        val[1] = vec.y();
        val[2] = vec.z();
        const QByteArray name = attribute.toUtf8();
        QByteArray cname = name + QByteArrayLiteral(".x");
        m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val));
        cname = name + QByteArrayLiteral(".y");
        m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val + 1));
        cname = name + QByteArrayLiteral(".z");
        m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val + 2));
        return;
    }
    case QMetaType::QColor:
    case QMetaType::QVector4D: {
        QVector4D vec = value.value<QVector4D>();
        float val[4];
        val[0] = vec.x();
        val[1] = vec.y();
        val[2] = vec.z();
        val[3] = vec.w();
        const QByteArray name = attribute.toUtf8();
        QByteArray cname = name + QByteArrayLiteral(".x");
        if (m_api.GetAttribute(element, cname.constData(), reinterpret_cast<char *>(&valueFloat))) {
            m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val));
            cname = name + QByteArrayLiteral(".y");
            m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val + 1));
            cname = name + QByteArrayLiteral(".z");
            m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val + 2));
            cname = name + QByteArrayLiteral(".w");
            m_api.SetAttribute(element, cname.constData(), reinterpret_cast<const char *>(val + 3));
        } else {
            QByteArray cname = name + QByteArrayLiteral(".r");
            if (m_api.GetAttribute(element, cname.constData(),
                                   reinterpret_cast<char *>(&valueFloat))) {
                m_api.SetAttribute(element, cname.constData(),
                                   reinterpret_cast<const char *>(val));
                cname = name + QByteArrayLiteral(".g");
                m_api.SetAttribute(element, cname.constData(),
                                   reinterpret_cast<const char *>(val + 1));
                cname = name + QByteArrayLiteral(".b");
                m_api.SetAttribute(element, cname.constData(),
                                   reinterpret_cast<const char *>(val + 2));
                cname = name + QByteArrayLiteral(".a");
                m_api.SetAttribute(element, cname.constData(),
                                   reinterpret_cast<const char *>(val + 3));
            }
        }
        return;
    }
    case QMetaType::QString:
    default:
        valueStr = value.toString().toUtf8();
        valuePtr = valueStr.constData();
        break;
    }

    m_api.SetAttribute(element, attribute.toUtf8().constData(), valuePtr);
}

void Q3DSQmlScript::fireEvent(const QString &event)
{
    if (!m_behavior.GetActive())
        return;
    m_api.FireEvent(m_behavior.path(), event.toUtf8().constData());
}

void Q3DSQmlScript::registerForEvent(const QString &event, const QJSValue &function)
{
    registerForEvent("", event, function);
}

void Q3DSQmlScript::registerForEvent(const QString &handle, const QString &event,
                                     const QJSValue &function)
{
    if (!m_behavior.GetActive())
        return;

    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return;

    if (!function.isCallable())
        return;

    CPresentation *presentation
            = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation());
    TEventCommandHash eventHash = CHash::HashEventCommand(event.toUtf8().constData());

    for (auto &&callback : m_eventCallbacks) {
        if (callback.element == element && callback.eventHash == eventHash)
            return;
    }

    EventData *data = new EventData();
    data->function = function;

    m_eventCallbacks.push_back({element, eventHash, data});

    presentation->RegisterEventCallback(element, eventHash, &eventCallback, data);
}

void Q3DSQmlScript::unregisterForEvent(const QString &event)
{
    unregisterForEvent("", event);
}

void Q3DSQmlScript::unregisterForEvent(const QString &handle, const QString &event)
{
    if (!m_behavior.GetActive())
        return;

    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return;

    CPresentation *presentation
            = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation());
    TEventCommandHash eventHash = CHash::HashEventCommand(event.toUtf8().constData());
    EventData *data = nullptr;

    for (int i = 0; i < m_eventCallbacks.size(); ++i) {
        if (m_eventCallbacks[i].element == element && m_eventCallbacks[i].eventHash == eventHash) {
            data = m_eventCallbacks[i].data;
            m_eventCallbacks.erase(m_eventCallbacks.begin() + i);
            break;
        }
    }

    if (data) {
        presentation->UnregisterEventCallback(element, eventHash, &eventCallback, data);
        delete data;
    }
}

QVector2D Q3DSQmlScript::getMousePosition()
{
    CPresentation *presentation
            = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation());
    qt3ds::runtime::IApplication &app = presentation->GetApplication();

    SInputFrame input = app.GetInputEngine().GetInputFrame();
    if (app.GetPrimaryPresentation()) {
        Q3DStudio::SMousePosition position =
            app.GetPrimaryPresentation()->GetScene()->WindowToPresentation(
                Q3DStudio::SMousePosition(static_cast<INT32>(input.m_PickX),
                                          static_cast<INT32>(input.m_PickY)));

        return QVector2D(position.m_X, position.m_Y);
    }
    return QVector2D(0, 0);
}

QMatrix4x4 Q3DSQmlScript::calculateGlobalTransform(const QString &handle)
{
    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return QMatrix4x4();

    RuntimeMatrix transform;
    IScene *scene = element->GetBelongedPresentation()->GetScene();
    scene->CalculateGlobalTransform(element, transform);
    transform.FlipCoordinateSystem();

    return QMatrix4x4(&transform.m_Data[0][0]);
}

QVector3D Q3DSQmlScript::lookAt(const QVector3D &target)
{
    FLOAT theMag = ::sqrtf(target.x() * target.x() + target.z() * target.z());
    FLOAT thePitch = -::atan2f(target.y(), theMag);
    FLOAT theYaw = ::atan2f(target.x(), target.z());

    return QVector3D(radToDeg(thePitch), radToDeg(theYaw), 0.0f);
}

QVector3D Q3DSQmlScript::matrixToEuler(const QMatrix4x4 &matrix)
{
    CEulerAngleConverter converter;
    const float *qMatrix = matrix.constData();
    HMatrix hHatrix;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j)
            hHatrix[i][j] = qMatrix[j * 4 + i];
    }
    EulerAngles eulerAngles = converter.Eul_FromHMatrix(hHatrix, EulOrdYXZs);
    return QVector3D(-eulerAngles.y, -eulerAngles.x, -eulerAngles.z);
}

QString Q3DSQmlScript::getParent(const QString &handle)
{
    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return "";

    TElement *parent = element->GetParent();
    if (!parent)
        return "";
    return parent->path().c_str();
}

void Q3DSQmlScript::setDataInputValue(const QString &name, const QVariant &value,
                                      qt3ds::runtime::DataInputValueRole valueRole)
{
    m_api.SetDataInputValue(name, value, valueRole);
}

TElement *Q3DSQmlScript::getElementByPath(const QString &path)
{
    if (!m_api.GetApplication())
        return nullptr;

    TElement *element = CQmlElementHelper::GetElement(
                        *m_api.GetApplication(),
                        m_api.GetApplication()->GetPrimaryPresentation(),
                        path.toUtf8().constData(), &m_owner);
    return element;
}
