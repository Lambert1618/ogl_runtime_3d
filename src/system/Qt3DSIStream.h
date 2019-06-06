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
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Pure virtual interface providing the stream for class serialization.
 */
class IStream
{
    //==============================================================================
    //	Typedef
    //==============================================================================
public:
    typedef INT32 TStreamPosition;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    virtual ~IStream(){}

public: // Serializing
    virtual INT32 ReadRawCopy(void *inDestination, const UINT32 inByteCount) = 0;
    virtual INT32 ReadRaw(const void *&inPtr, const UINT32 inByteCount) = 0;
    virtual INT32 WriteRaw(const void *inSource, const UINT32 inByteCount) = 0;

    // Seeking
    virtual void Offset(const INT32 inOffset) = 0;
    virtual TStreamPosition Current() = 0;
    virtual void Set(const TStreamPosition &inPosition) = 0;

public: // Template Readers	that make a copy
    template <typename T>
    INT32 ReadCopy(T &outValue);
    template <typename T>
    INT32 ReadArrayCopy(T *outValue, const UINT32 inCount);

public: // Template Readers that access value directly
    template <typename T>
    INT32 Read(const T *&inPtr);
    template <typename T>
    INT32 ReadArray(const T *&inPtr, UINT32 inCount);

public: // Template Writers
    template <typename T>
    INT32 Write(const T &inValue);
    template <typename T>
    INT32 WriteArray(const T *inValue, const UINT32 inCount);
};

/** Notes concerning usage of Read or ReadCopy (and ReadArray and ReadArrayCopy)**/
/**
If the value being read fits into 4 bytes (i.e float, int/long), either Read/ReadCopy can be used.
However, if the read value is going to be accessed frequently (in loops etc), ReadCopy is
recommended so a copy is available instead of dereferencing.

ReadCopy should be used if the data read is an array or is significantly larger than 4 bytes (like
vectors or matrixes).

For cases where the value is to be stored in a persistant data structures, ReadCopy should be used.

One important thing about Read/ReadArray, is that the pointer returned is only valid before the next
Read/ReadyArray is called.
There is no guarantee that the pointer data will remain unchanged during subsequent calls to
Read/ReadArray.
**/

//==============================================================================
/**
 *	Read and assign a value by copying.
 */
template <typename T>
INT32 IStream::ReadCopy(T &outValue)
{
    return ReadRawCopy(&outValue, sizeof(T));
}

//==============================================================================
/**
 *	Read and copy data to a preallocated array.
 */
template <typename T>
INT32 IStream::ReadArrayCopy(T *outValue, const UINT32 inCount)
{
    return ReadRawCopy(outValue, sizeof(T) * inCount);
}

//==============================================================================
/**
 * Obtain a pointer directly to the data.
 */
template <typename T>
INT32 IStream::Read(const T *&inPtr)
{
    return ReadRaw(reinterpret_cast<const void *&>(inPtr), sizeof(T));
}

//==============================================================================
/**
 * Obtain a pointer directly to the array data.
 */
template <typename T>
INT32 IStream::ReadArray(const T *&inPtr, const UINT32 inCount)
{
    return ReadRaw(reinterpret_cast<const void *&>(inPtr), sizeof(T) * inCount);
}

//==============================================================================
/**
 * Write a value.
 */
template <typename T>
INT32 IStream::Write(const T &inValue)
{
    return WriteRaw(&inValue, sizeof(T));
}

//==============================================================================
/**
 * Write an array of values.
 */
template <typename T>
INT32 IStream::WriteArray(const T *inValue, const UINT32 inCount)
{
    return WriteRaw(inValue, sizeof(T) * inCount);
}

} // namespace Q3DStudio
