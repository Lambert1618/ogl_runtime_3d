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
#ifndef QT3DS_EVENT_SYSTEM_C_H
#define QT3DS_EVENT_SYSTEM_C_H
#include <stddef.h> //size_t

// Interface we expect plugins to use in order to interace with the event system.
// This interface is design to avoid using c++ vtables and the C++ abi isn't stable
// across compilers.

#ifdef _cplusplus
extern "C" {
#endif

// Obnoxious long names to void namespace conflicts
enum QT3DSEventSystemEventTypes {
    QT3DSEventSystemEventTypesNoData = 0,
    QT3DSEventSystemEventTypesString = 1,
    QT3DSEventSystemEventTypesNumber = 2,
};

typedef const char *Qt3DSEventSystemStr;

typedef struct _Qt3DSEventSystemEventValue
{
    QT3DSEventSystemEventTypes m_Type;
    double m_Number;
    Qt3DSEventSystemStr m_String;
} Qt3DSEventSystemEventValue;

typedef struct _Qt3DSEventSystemRegisteredStr
{
    Qt3DSEventSystemStr m_Data;
} Qt3DSEventSystemRegisteredStr;

typedef struct _Qt3DSEventSystemEventData
{
    Qt3DSEventSystemRegisteredStr m_Name;
    Qt3DSEventSystemEventValue m_Value;
} Qt3DSEventSystemEventData;

typedef struct _Qt3DSEventSystemEvent
{
    Qt3DSEventSystemEventData *m_Data; // contiguous block of event data.
    int m_NumData;
} Qt3DSEventSystemEvent;

typedef Qt3DSEventSystemEvent *(*Qt3DSEventSystemEventFactoryCreateEventFn)(void *inFactory,
                                                                          int numData);
typedef size_t (*Qt3DSEventSystemEventFactoryMaxNumEventDataFn)(void *inFactory);
typedef size_t (*Qt3DSEventSystemEventFactoryMaxStrLenFn)(void *inFactory);
typedef Qt3DSEventSystemRegisteredStr (*Qt3DSEventSystemEventFactoryRegisterStrFn)(
    void *inFactory, Qt3DSEventSystemStr inStr);
typedef Qt3DSEventSystemStr (*Qt3DSEventSystemEventFactoryAllocateStrFn)(void *inFactory,
                                                                       Qt3DSEventSystemStr inStr);
typedef Qt3DSEventSystemStr (*Qt3DSEventSystemEventFactoryAllocateStrLenFn)(void *inFactory, int len);

// Factory from the provider's perspective.
typedef struct _Qt3DSEventSystemEventFactory
{
    void *m_Factory;
    Qt3DSEventSystemEventFactoryCreateEventFn createEvent;
    Qt3DSEventSystemEventFactoryMaxNumEventDataFn getMaxNumEventData;
    Qt3DSEventSystemEventFactoryMaxStrLenFn getMaxStrLength;
    Qt3DSEventSystemEventFactoryRegisterStrFn registerStr;
    Qt3DSEventSystemEventFactoryAllocateStrFn allocateStr;
    Qt3DSEventSystemEventFactoryAllocateStrLenFn allocateStrLen;

} Qt3DSEventSystemEventFactory;

typedef size_t (*Qt3DSEventSystemProviderGetNextEventsFn)(void *inProvider,
                                                         Qt3DSEventSystemEventFactory inFactory,
                                                         Qt3DSEventSystemEvent **outBuffer,
                                                         size_t outBufLen);
typedef void (*Qt3DSEventSystemProviderReleaseFn)(void *inProvider);

typedef struct _Qt3DSEventSystemEventProvider
{
    void *m_Provider;
    Qt3DSEventSystemProviderGetNextEventsFn getNextEvents;
    Qt3DSEventSystemProviderReleaseFn release;
} Qt3DSEventSystemEventProvider;

typedef void (*Qt3DSEventSystemPollerAddProviderFn)(void *inPoller,
                                                   Qt3DSEventSystemEventProvider inProvider);
typedef void (*Qt3DSEventSystemPollerRemoveProviderFn)(void *inPoller,
                                                      Qt3DSEventSystemEventProvider inProvider);
typedef int (*Qt3DSEventSystemPollerignoreProviderFn)(void *inPoller,
                                                     Qt3DSEventSystemEventProvider inProvider);
typedef int (*Qt3DSEventSystemPolleractivateProviderFn)(void *inPoller,
                                                       Qt3DSEventSystemEventProvider inProvider);

typedef struct _Qt3DSEventSystemEventPoller
{
    void *m_Poller;
    // Providers, once added, will be released by the event poller unless removed
    Qt3DSEventSystemPollerAddProviderFn addProvider;
    // Callers are responsible for releasing removed providers.
    Qt3DSEventSystemPollerRemoveProviderFn removeProvider;
    Qt3DSEventSystemPollerignoreProviderFn ignoreProvider;
    Qt3DSEventSystemPolleractivateProviderFn activateProvider;

} Qt3DSEventSystemEventPoller;

extern const char *PROVIDER_TABLE_ENTRY;
extern const char *POLLER_TABLE_ENTRY;

#ifdef _cplusplus
}
#endif

#endif