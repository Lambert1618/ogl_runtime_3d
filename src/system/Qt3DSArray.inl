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
 *
 *	@param inCapacity preallocated number of slots to avoid growth reallocation
 *	@param inGrowFactor factor by which the array grows when out of space
 *	@param inName 32 byte unique identifier for memory tracking
 */
template <typename T> inline
CArray<T>::CArray( const INT32 inCapacity, const FLOAT inGrowFactor, const CHAR* inName ) :
	m_Data( NULL ),
	m_Capacity( 0 ),
	m_Count( 0 ),
	m_GrowFactor( inGrowFactor )
{
	// Disallow negative growth
	if ( m_GrowFactor < 1.0f )
		m_GrowFactor = DEFAULT_GROWFACTOR;

	SetName( inName );
	Reserve( inCapacity );
}

//==============================================================================
/**		
 *	Berger's Copy Constructor.
 */
template <typename T> inline
CArray<T>::CArray( const CArray<T>& inOther ) : 
	m_Data( NULL ),
	m_Capacity( 0 ),
	m_Count( 0 ),
	m_GrowFactor( inOther.m_GrowFactor )
{
	// Disallow negative growth
	if ( m_GrowFactor < 1.0f )
		m_GrowFactor = DEFAULT_GROWFACTOR;

	SetName( inOther.m_Name );
	Reserve( inOther.m_Capacity );
	FOR_ARRAY( T, theTemp, inOther )
	{
		Push( *theTemp );
	}
}

//==============================================================================
/**		
 *	Destructor
 */
template <class T> inline
CArray<T>::~CArray( )
{
	Clear( true );
}

//==============================================================================
/**
 *	Name an array instance to easily identify it during memory reporting.
 *	@param inName 32 byte unique identifier
 */
template <class T> inline
void CArray<T>::SetName( const CHAR* inName /*= NULL*/ )
{
        Q3DStudio_sprintf( m_Name, sizeof( m_Name ), "%s", inName ? inName : "CArray<T>" );
}

//==============================================================================
/**
 *	Count is the number of elements in the array.
 *	@return current count of the array
 */
template <class T> inline
INT32 CArray<T>::GetCount( ) const
{
	return m_Count;
}

//==============================================================================
/**
 *	Get the capacity of the array
 *	Capacity reflects the max count before reallocation happens.
 *	@return capacity of the array
 */
template <class T> inline
INT32 CArray<T>::GetCapacity( ) const
{
	return m_Capacity;
}

//==============================================================================
/**
 *	Clear the array by reducing the number of elements to zero.
 *
 *	Using the release flag keeps the memory load low but could be
 *	suboptimal in situations where you are always using roughly the same
 *	number of elements.  In that scenario, the array would have to build up
 *	and reallocate up to the same number of elements every time causing
 *	stress on the system.  Clearing but keeping the buffer by setting
 *	release to false is optimal in that case.
 *
 *	@param inRelease	true to release memory, false keeps old buffer
 */
template <class T> inline
void CArray<T>::Clear( const BOOL inRelease )
{
	m_Count = 0;

	if ( inRelease )
	{
                Q3DStudio_free( m_Data, T, m_Capacity );
		m_Capacity = 0;
		m_Data = NULL;
	}
	else
                Q3DStudio_memset( m_Data, 0 , sizeof( T ) * m_Capacity );
}

//==============================================================================
/**
 *	Allocate enough memory to store T.
 *
 *	If current memory is not sufficient, new block of memory is allocated and 
 *	data in current memory is copied to the new block of memory. Data pointer will
 *	point to the new block of memory and current memory is released.
 *
 *	@param inCapacity	indicate the amount of memory required.
 *	@param inExpand		true to not only allocate but also extend array
 */
template <class T> inline
void CArray<T>::Reserve( const INT32 inCapacity, const BOOL inExpand )
{
	if ( inCapacity > m_Capacity )
	{
                T* theData = Q3DStudio_allocate_desc( T, inCapacity, m_Name );
                Q3DStudio_memset( theData, 0 , sizeof( T ) * inCapacity );
		if ( m_Data != NULL )
		{
                        Q3DStudio_memcpy( theData, m_Data, sizeof( T ) * m_Count );
                        Q3DStudio_free( m_Data, T, m_Capacity );
		}
		m_Data = theData;		
		m_Capacity = inCapacity;
	}

	if ( inExpand )
		m_Count = inCapacity;
}

//==============================================================================
/**
 *	Insert T to the end of the array
 *	@param inValue	value of datatype T
 */
template <class T> inline
void CArray<T>::Push( const T& inValue )
{
	if ( m_Count == m_Capacity )
		Reserve( static_cast<INT32>( m_GrowFactor * m_Capacity ) + 1 );

	m_Data[m_Count++] = inValue;
}

//==============================================================================
/**
 *	Remove and retrieve T which is at the end of the array
 *	@return value of datatype T
 */
template <class T> inline
const T& CArray<T>::Pop( )
{
	return m_Data[--m_Count];
}
	
//==============================================================================
/**
 *	Get T which is at the end of the array
 *	@return reference of value of datatype T
 */
template <class T> inline
const T& CArray<T>::Top( ) const
{
	return m_Data[m_Count - 1];
}

//==============================================================================
/**
 *	Get index of first element that matches inValue
 *	@return index of element matching T or INDEX_NOTFOUND
 */
template <class T> inline
INT32 CArray<T>::GetIndex( const T& inValue ) const
{
	for ( INT32	theIndex = 0; theIndex < m_Count; ++theIndex )
		if ( inValue == m_Data[theIndex] )
			return theIndex;
	return NVARRAY_NOTFOUND;
}

//==============================================================================
/**
 *	Get the element at a particular index position of the array.
 *	@note Range checking is only done is debug builds.
 *	@param inIndex	the index of the array
 *	@return reference of value of datatype T
 */
template <class T> inline
T& CArray<T>::operator[]( const INT32 inIndex ) const
{
        Q3DStudio_ASSERT( inIndex >= 0 && inIndex < m_Capacity );
	return m_Data[inIndex];
}

//==============================================================================
/**
 *	Remove T that resides at a particular index position of the array.
 *	Other T data with higher index value is shifted to occupy the empty slot 
 *	created in the removal process.
 *	@param inIndex		the index of the array
 */
template <class T> inline
void CArray<T>::Remove( const INT32 inIndex )
{
        Q3DStudio_memmove( m_Data + inIndex, m_Data + inIndex + 1, sizeof( T ) * ( m_Count - inIndex - 1 ) );
	--m_Count;
}

//==============================================================================
/**
 *	Get T at the first index of the array. 
 *	Mainly used for FOR_ARRAY macro implementation
 *	@return pointer to first value in array
 */
template <class T> inline
T* CArray<T>::Begin( ) const
{
	return m_Data;
}

//==============================================================================
/**
 *	Get T at the last index of the array. 
 *	Mainly used for FOR_ARRAY macro implementation
 *	@return pointer to one beyond last value in array
 */
template <class T> inline
T* CArray<T>::End( ) const
{
	return m_Data + m_Count;
}

} // namespace Q3DStudio
