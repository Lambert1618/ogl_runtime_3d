/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMValue.h"
#include "foundation/Qt3DSLogging.h"

#include <QtGui/qcolor.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <QtGui/qvector4d.h>

using namespace qt3dsdm;

SValue::SValue(const QVariant &inData)
{
    switch (inData.type()) {
    case QVariant::Bool:
    {
        *this = inData.toBool();
        break;
    }
    case QVariant::Color:
    {
        const QColor c = inData.value<QColor>();
        *this = qt3dsdm::SFloat4(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        break;
    }
    case QVariant::String:
    {
        const QString q = inData.toString();
        const int count = q.size() + 1;
#ifdef __INTEGRITY
        wchar_t* tempBuf = reinterpret_cast<wchar_t*>(malloc(count * sizeof(wchar_t)));
#else
        wchar_t* tempBuf = reinterpret_cast<wchar_t*>(alloca(count * sizeof(wchar_t)));
#endif
        tempBuf[count - 1] = 0;
        q.toWCharArray(tempBuf);
        *this = std::make_shared<qt3dsdm::CDataStr>(tempBuf);
#ifdef __INTEGRITY
        free(tempBuf);
#endif
        break;
    }
    case QVariant::Int: {
        *this = inData.toInt();
        break;
    }
    case QVariant::Double: {
        *this = inData.toFloat();
        break;
    }
    case QVariant::Vector2D: {
        const auto v = inData.value<QVector2D>();
        *this = qt3dsdm::SFloat2(v.x(), v.y());
        break;
    }
    case QVariant::Vector3D: {
        const auto v = inData.value<QVector3D>();
        *this = qt3dsdm::SFloat3(v.x(), v.y(), v.z());
        break;
    }
    case QVariant::Vector4D: {
        const auto v = inData.value<QVector4D>();
        *this = qt3dsdm::SFloat4(v.x(), v.y(), v.z(), v.w());
        break;
    }
    default:
        qCWarning(qt3ds::WARNING) << "Add a handler for QVariant::type" << inData.type();
        throw std::runtime_error("Cannot transform this QVariant into SValue");
    }
}

QVariant SValue::toQVariant() const
{
    switch (getType()) {
    case DataModelDataType::String:
    case DataModelDataType::StringRef:
    {
        return get<QString>(*this);
    }
    case DataModelDataType::Float: {
        return get<float>(*this);
    }
    case DataModelDataType::Float2: {
        return QVariant::fromValue(get<QVector2D>(*this));
    }
    case DataModelDataType::Float3: {
        return QVariant::fromValue(get<QVector3D>(*this));
    }
    case DataModelDataType::Float4: {
        return QVariant::fromValue(get<QVector4D>(*this));
    }
    case DataModelDataType::Long: {
        return QVariant::fromValue(get<qt3ds::QT3DSI32>(*this));
    }
    case DataModelDataType::Bool: {
        return get<bool>(*this);
    }
    case DataModelDataType::FloatList: {
        //KDAB_TODO
        qCWarning(qt3ds::WARNING) << "Add a handler for type DataModelDataType::FloatList";
        return {};
    }
    case DataModelDataType::Long4: {
        return QVariant::fromValue(get<QVector<qt3ds::QT3DSU32> >(*this));
    }
    case DataModelDataType::ObjectRef: {
        const SObjectRefType &theRef(get<SObjectRefType>(*this));
        switch (theRef.GetReferenceType()) {
        case ObjectReferenceType::Absolute:
            return SValue(get<SLong4>(theRef.m_Value)).toQVariant();
        case ObjectReferenceType::Relative:
            return SValue(get<TDataStrPtr>(theRef.m_Value)).toQVariant();
        case ObjectReferenceType::Unknown:
            return QVariant::fromValue(QVector<qt3ds::QT3DSU32>());
        }
    }
    case DataModelDataType::StringOrInt: {
        const SStringOrInt &theData(get<SStringOrInt>(*this));
        if (theData.GetType() == SStringOrIntTypes::Int) {
            return QVariant::fromValue(get<qt3ds::QT3DSI32>(theData.m_Value));
        } else if (theData.GetType() == SStringOrIntTypes::String) {
            auto wideStr = get<TDataStrPtr>(theData.m_Value)->GetData();
            return QString::fromWCharArray(wideStr);
        } else {
            return {};
        }

    }
    case DataModelDataType::None:
        return {};
    default:
        break;
    }
    return {};
}

SInternValue::SInternValue(const SValue &inValue, IStringTable &inStringTable)
{
    if (GetValueType(inValue) == DataModelDataType::StringRef) {
        const SStringRef &current = get<SStringRef>(inValue);
        SStringRef newId = inStringTable.RegisterStr(current.m_Id);
        m_Value = SValue(newId);
    } else {
        m_Value = inValue;
    }
}

SInternValue::SInternValue(const SInternValue &inOther)
    : m_Value(inOther.m_Value)
{
}
