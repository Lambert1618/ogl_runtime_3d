/****************************************************************************
**
** Copyright (C) 1999-2007 NVIDIA Corporation.
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

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
template <class T, int Size>
inline CFixedArray<T, Size>::CFixedArray()
    : m_Capacity(Size)
    , m_Count(0)
{
}

//==============================================================================
/**
 *	Destructor
 */
template <class T, int Size>
inline CFixedArray<T, Size>::~CFixedArray()
{
}

//==============================================================================
/**
 *	Remove all elements in the array
 */
template <class T, int Size>
inline void CFixedArray<T, Size>::Clear()
{
    m_Count = 0;
}

//==============================================================================
/**
 *	Get current count of T in the array
 *	@return current count of the array
 */
template <class T, int Size>
inline INT32 CFixedArray<T, Size>::GetCount() const
{
    return m_Count;
}

//==============================================================================
/**
 *	Get the capacity of the array
 *	Capacity reflects the allocated memory size
 *	@return capacity of the array
 */
template <class T, int Size>
inline INT32 CFixedArray<T, Size>::GetCapacity() const
{
    return m_Capacity;
}

//==============================================================================
/**
 *	Insert T to the end of the array
 *	@param inValue		value of datatype T
 */
template <class T, int Size>
inline void CFixedArray<T, Size>::Push(const T &inValue)
{
    if (m_Count == m_Capacity) {
        qWarning("Trying to push to many objects onto a CFixedArray", "Capacity=%d", m_Capacity);
        return;
    }

    m_Data[m_Count] = inValue;
    ++m_Count;
}

//==============================================================================
/**
 *	Remove and retrieve T which is at the end of the array
 *	@return value of datatype T
 */
template <class T, int Size>
inline const T &CFixedArray<T, Size>::Pop()
{
    if (m_Count == 0)
        return {};

    --m_Count;
    return m_Data[m_Count];
}

//==============================================================================
/**
 *	Get T at a particular index position of the array.
 *	Need to ensure only valid index is allowed
 *	Clear( ) will set the m_counter = 0
 *	@param inIndex	the index of the array
 *	@return reference of value of datatype T
 */
template <class T, int Size>
inline T &CFixedArray<T, Size>::operator[](const INT32 inIndex)
{
    Q3DStudio_ASSERT(inIndex < m_Capacity);

    return m_Data[inIndex];
}

//==============================================================================
/**
 * Removes a specific entry from the array.
 * Moves all entries below it up, expensive.
 * @param inIndex	the index of the entry
 */
template <class T, int Size>
inline void CFixedArray<T, Size>::Remove(const INT32 inIndex)
{
    Q3DStudio_memmove(m_Data + inIndex, m_Data + inIndex + 1, sizeof(T) * (m_Count - inIndex - 1));
    --m_Count;
}

} // namespace Q3DStudio
