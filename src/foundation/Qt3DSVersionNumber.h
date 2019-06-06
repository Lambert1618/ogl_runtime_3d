/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

/*
VersionNumbers:  The combination of these
numbers uniquely identifies the API, and should
be incremented when the SDK API changes.  This may
include changes to file formats.

This header is included in the main SDK header files
so that the entire SDK and everything that builds on it
is completely rebuilt when this file changes.  Thus,
this file is not to include a frequently changing
build number.  See BuildNumber.h for that.

Each of these three values should stay below 255 because
sometimes they are stored in a byte.
*/
/** \addtogroup foundation
  @{
*/
#ifndef QT3DS_FOUNDATION_QT3DS_VERSION_NUMBER_H
#define QT3DS_FOUNDATION_QT3DS_VERSION_NUMBER_H

#define QT3DS_FOUNDATION_VERSION_MAJOR 3
#define QT3DS_FOUNDATION_VERSION_MINOR 3
#define QT3DS_FOUNDATION_VERSION_BUGFIX 0

/**
The constant QT3DS_FOUNDATION_VERSION is used when creating certain PhysX module objects.
This is to ensure that the application is using the same header version as the library was built
with.
*/
#define QT3DS_FOUNDATION_VERSION                                                                      \
    ((QT3DS_FOUNDATION_VERSION_MAJOR << 24) + (QT3DS_FOUNDATION_VERSION_MINOR << 16)                     \
     + (QT3DS_FOUNDATION_VERSION_BUGFIX << 8) + 0)

#endif // QT3DS_FOUNDATION_QT3DS_VERSION_NUMBER_H

/** @} */
