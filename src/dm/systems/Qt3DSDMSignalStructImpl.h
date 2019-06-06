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
#pragma once
#ifndef QT3DSDM_SIGNAL_STRUCT_IMPL_H
#define QT3DSDM_SIGNAL_STRUCT_IMPL_H
#include "Qt3DSDMSignalStructs.h"

namespace qt3dsdm {

struct SSignalDataStruct0
{
    typedef SSignalDataStruct0 TThisType;
    typedef std::function<void()> TFunType;
    static void Send(ISignalSystem &system, void *sender, const char *sigName)
    {
        system.Signal(sender, sigName, NULL, 0);
    }

    static void Unpack(void *inSender, const char *inName, const char *inData, size_t inDataSize,
                       TFunType inHandler)
    {
        if (inDataSize == 0)
            inHandler();
        else {
            QT3DS_ASSERT(false);
        }
    }

    static TGenericSignalHandlerFunc CreateHandler(std::function<void()> inHandler)
    {
        return std::bind(&TThisType::Unpack, std::placeholders::_1, std::placeholders::_2,
                         std::placeholders::_3, std::placeholders::_4, inHandler);
    }

    static std::shared_ptr<ISignalConnection> AddListener(ISignalSystem &inSystem, void *inSender,
                                                            const char *inName, TFunType inHandler)
    {
        return inSystem.AddListener(inSender, inName, CreateHandler(inHandler));
    }
};

#define QT3DSDM_SIGNALS_DEFINE_SPECIFIC_SIGNAL_STRUCT0(name)                                         \
    std::shared_ptr<ISignalConnection> Sig##name::AddListener(                                   \
        ISignalSystem &inSystem, void *inSender, std::function<void()> inHandler)                \
    {                                                                                              \
        typedef SSignalDataStruct0 TBase;                                                          \
        return TBase::AddListener(inSystem, inSender, #name, inHandler);                           \
    }                                                                                              \
    void Sig##name::Send(ISignalSystem &system, void *sender)                                      \
    {                                                                                              \
        typedef SSignalDataStruct0 TBase;                                                          \
        TBase::Send(system, sender, #name);                                                        \
    }

template <typename TData1>
struct SSignalDataStruct1
{
    typedef SSignalDataStruct1<TData1> TThisType;
    TData1 m_Data1;
    typedef std::function<void(TData1)> TFunType;
    SSignalDataStruct1(TData1 d1)
        : m_Data1(d1)
    {
    }
    static void Send(ISignalSystem &system, void *sender, const char *sigName, const TData1 &d1)
    {
        TThisType theData(d1);
        system.Signal(sender, sigName, reinterpret_cast<const char *>(&theData), sizeof(TThisType));
    }

    static void Unpack(void *inSender, const char *inName, const char *inData, size_t inDataSize,
                       TFunType inHandler)
    {
        if (inDataSize == sizeof(TThisType)) {
            const TThisType *theData = reinterpret_cast<const TThisType *>(inData);
            inHandler(theData->m_Data1);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    static TGenericSignalHandlerFunc CreateHandler(TFunType inHandler)
    {
        return std::bind(&TThisType::Unpack, std::placeholders::_1, std::placeholders::_2,
                         std::placeholders::_3, std::placeholders::_4, inHandler);
    }

    static std::shared_ptr<ISignalConnection> AddListener(ISignalSystem &inSystem, void *inSender,
                                                            const char *inName, TFunType inHandler)
    {
        return inSystem.AddListener(inSender, inName, CreateHandler(inHandler));
    }
};

#define QT3DSDM_SIGNALS_DEFINE_SPECIFIC_SIGNAL_STRUCT1(name, d1)                                     \
    std::shared_ptr<ISignalConnection> Sig##name::AddListener(                                   \
        ISignalSystem &inSystem, void *inSender, std::function<void(d1)> inHandler)              \
    {                                                                                              \
        typedef SSignalDataStruct1<d1> TBase;                                                      \
        return TBase::AddListener(inSystem, inSender, #name, inHandler);                           \
    }                                                                                              \
    void Sig##name::Send(ISignalSystem &system, void *sender, const d1 &arg)                       \
    {                                                                                              \
        typedef SSignalDataStruct1<d1> TBase;                                                      \
        TBase::Send(system, sender, #name, arg);                                                   \
    }

template <typename TData1, typename TData2>
struct SSignalDataStruct2
{
    typedef SSignalDataStruct2<TData1, TData2> TThisType;
    typedef std::function<void(TData1, TData2)> TFunType;
    TData1 m_Data1;
    TData2 m_Data2;
    SSignalDataStruct2(TData1 d1, TData2 d2)
        : m_Data1(d1)
        , m_Data2(d2)
    {
    }
    static void Send(ISignalSystem &system, void *sender, const char *sigName, const TData1 &d1,
                     const TData2 &d2)
    {
        TThisType theData(d1, d2);
        system.Signal(sender, sigName, reinterpret_cast<const char *>(&theData), sizeof(TThisType));
    }

    static void Unpack(void *inSender, const char *inName, const char *inData, size_t inDataSize,
                       TFunType inHandler)
    {
        if (inDataSize == sizeof(TThisType)) {
            const TThisType *theData = reinterpret_cast<const TThisType *>(inData);
            inHandler(theData->m_Data1, theData->m_Data2);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    static TGenericSignalHandlerFunc CreateHandler(TFunType inHandler)
    {
        return std::bind(&TThisType::Unpack, std::placeholders::_1, std::placeholders::_2,
                         std::placeholders::_3, std::placeholders::_4, inHandler);
    }

    static std::shared_ptr<ISignalConnection> AddListener(ISignalSystem &inSystem, void *inSender,
                                                            const char *inName, TFunType inHandler)
    {
        return inSystem.AddListener(inSender, inName, CreateHandler(inHandler));
    }
};

#define QT3DSDM_SIGNALS_DEFINE_SPECIFIC_SIGNAL_STRUCT2(name, d1, d2)                                 \
    std::shared_ptr<ISignalConnection> Sig##name::AddListener(                                   \
        ISignalSystem &inSystem, void *inSender, std::function<void(d1, d2)> inHandler)          \
    {                                                                                              \
        return SSignalDataStruct2<d1, d2>::AddListener(inSystem, inSender, #name, inHandler);      \
    }                                                                                              \
    void Sig##name::Send(ISignalSystem &system, void *sender, const d1 &arg1, const d2 &arg2)      \
    {                                                                                              \
        SSignalDataStruct2<d1, d2>::Send(system, sender, #name, arg1, arg2);                       \
    }

template <typename TData1, typename TData2, typename TData3>
struct SSignalDataStruct3
{
    typedef SSignalDataStruct3<TData1, TData2, TData3> TThisType;
    typedef std::function<void(TData1, TData2, TData3)> TFunType;
    TData1 m_Data1;
    TData2 m_Data2;
    TData3 m_Data3;
    SSignalDataStruct3(const TData1 &d1, const TData2 &d2, const TData3 &d3)
        : m_Data1(d1)
        , m_Data2(d2)
        , m_Data3(d3)
    {
    }
    static void Send(ISignalSystem &system, void *sender, const char *sigName, const TData1 &d1,
                     const TData2 &d2, const TData3 &d3)
    {
        TThisType theData(d1, d2, d3);
        system.Signal(sender, sigName, reinterpret_cast<const char *>(&theData), sizeof(TThisType));
    }

    static void Unpack(void *inSender, const char *inName, const char *inData, size_t inDataSize,
                       TFunType inHandler)
    {
        if (inDataSize == sizeof(TThisType)) {
            const TThisType *theData = reinterpret_cast<const TThisType *>(inData);
            inHandler(theData->m_Data1, theData->m_Data2, theData->m_Data2);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    static TGenericSignalHandlerFunc CreateHandler(TFunType inHandler)
    {
        return std::bind(&TThisType::Unpack, std::placeholders::_1, std::placeholders::_2,
                         std::placeholders::_3, std::placeholders::_4, inHandler);
    }

    static std::shared_ptr<ISignalConnection> AddListener(ISignalSystem &inSystem, void *inSender,
                                                            const char *inName, TFunType inHandler)
    {
        return inSystem.AddListener(inSender, inName, CreateHandler(inHandler));
    }
};

#define QT3DSDM_SIGNALS_DEFINE_SPECIFIC_SIGNAL_STRUCT3(name, d1, d2, d3)                             \
    std::shared_ptr<ISignalConnection> Sig##name::AddListener(                                   \
        ISignalSystem &inSystem, void *inSender, std::function<void(d1, d2, d3)> inHandler)      \
    {                                                                                              \
        return SSignalDataStruct3<d1, d2, d3>::AddListener(inSystem, inSender, #name, inHandler);  \
    }                                                                                              \
    void Sig##name::Send(ISignalSystem &system, void *sender, const d1 &arg1, const d2 &arg2,      \
                         const d3 &arg3)                                                           \
    {                                                                                              \
        SSignalDataStruct3<d1, d2, d3>::Send(system, sender, #name, arg1, arg2, arg3);             \
    }

template <typename TData1, typename TData2, typename TData3, typename TData4>
struct SSignalDataStruct4
{
    typedef SSignalDataStruct4<TData1, TData2, TData3, TData4> TThisType;
    typedef std::function<void(TData1, TData2, TData3, TData4)> TFunType;
    TData1 m_Data1;
    TData2 m_Data2;
    TData3 m_Data3;
    TData4 m_Data4;
    SSignalDataStruct4(const TData1 &d1, const TData2 &d2, const TData3 &d3, const TData4 &d4)
        : m_Data1(d1)
        , m_Data2(d2)
        , m_Data3(d3)
        , m_Data4(d4)
    {
    }
    static void Send(ISignalSystem &system, void *sender, const char *sigName, const TData1 &d1,
                     const TData2 &d2, const TData3 &d3, const TData4 &d4)
    {
        TThisType theData(d1, d2, d3, d4);
        system.Signal(sender, sigName, reinterpret_cast<const char *>(&theData), sizeof(TThisType));
    }

    static void Unpack(void *inSender, const char *inName, const char *inData, size_t inDataSize,
                       TFunType inHandler)
    {
        if (inDataSize == sizeof(TThisType)) {
            const TThisType *theData = reinterpret_cast<const TThisType *>(inData);
            inHandler(theData->m_Data1, theData->m_Data2, theData->m_Data2, theData->m_Data4);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    static TGenericSignalHandlerFunc CreateHandler(TFunType inHandler)
    {
        return std::bind(&TThisType::Unpack, std::placeholders::_1, std::placeholders::_2,
                         std::placeholders::_3, std::placeholders::_4, inHandler);
    }

    static std::shared_ptr<ISignalConnection> AddListener(ISignalSystem &inSystem, void *inSender,
                                                            const char *inName, TFunType inHandler)
    {
        return inSystem.AddListener(inSender, inName, CreateHandler(inHandler));
    }
};

#define QT3DSDM_SIGNALS_DEFINE_SPECIFIC_SIGNAL_STRUCT4(name, d1, d2, d3, d4)                         \
    std::shared_ptr<ISignalConnection> Sig##name::AddListener(                                   \
        ISignalSystem &inSystem, void *inSender, std::function<void(d1, d2, d3, d4)> inHandler)  \
    {                                                                                              \
        return SSignalDataStruct4<d1, d2, d3, d4>::AddListener(inSystem, inSender, #name,          \
                                                               inHandler);                         \
    }                                                                                              \
    void Sig##name::Send(ISignalSystem &system, void *sender, const d1 &arg1, const d2 &arg2,      \
                         const d3 &arg3, const d4 &arg4)                                           \
    {                                                                                              \
        SSignalDataStruct4<d1, d2, d3, d4>::Send(system, sender, #name, arg1, arg2, arg3, arg4);   \
    }
}
#endif
