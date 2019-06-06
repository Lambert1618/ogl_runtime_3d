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
template <typename T> inline
CCircularArray<T>::CCircularArray( const INT32 inCapacity, const CHAR* inName ) :
	m_Data( NULL ),
	m_Capacity( 0 ),
	m_Count( 0 ),
	m_Begin( 0 ),
	m_End( 0 ),
	m_Full( false )
{
        Q3DStudio_sprintf( m_Name, sizeof( m_Name ), "%s", inName ? inName : "CircularArray" );
	Reserve( inCapacity );
}

//==============================================================================
/**		
 *	Destructor
 */
template <class T> inline
CCircularArray<T>::~CCircularArray( )
{
	Clear( true );
}

//==============================================================================
/**		
 *	Get the number of items in the array
 *	@return the number of items in the array
 */
template <class T> inline
INT32 CCircularArray<T>::GetCount( ) const
{
	return m_Count;
}

//==============================================================================
/**		
 *	Get the capacity of the array
 *	@return the capacity of the array
 */
template <class T> inline
INT32 CCircularArray<T>::GetCapacity( ) const
{
	return m_Capacity;
}

//==============================================================================
/**		
 *	Remove all items from the array
 *	@param inRelease	if true, release the allocated memory, if false, reset
 *						parameters back to zero.
 */
template <class T> inline
void CCircularArray<T>::Clear( const BOOL inRelease )
{
	m_Count = 0;
	m_Begin = 0;
	m_End = 0;
	m_Full = false;

	if ( inRelease )
	{
                Q3DStudio_free( m_Data, T, m_Capacity );
		m_Capacity = 0;
		m_Data = NULL;
	}
}

//==============================================================================
/**		
 *	Change capacity of the array
 *	Assume no expansion for circular array
 *	@param inCapacity		the new capacity to set
 */
template <class T> inline
void CCircularArray<T>::Reserve( const INT32 inCapacity )
{
        Q3DStudio_ASSERT( !m_Data );
        T* theData = Q3DStudio_allocate_desc( T, inCapacity, m_Name );
	m_Data = theData;		
	m_Capacity = inCapacity;
}

//==============================================================================
/**		
 *	Create a new uninitialized item at the end of the array.
 *	The returned reference is used to initialize the item.
 *	@return a reference to the item
 */
template <class T> inline
T& CCircularArray<T>::NewEntry( )
{
        Q3DStudio_ASSERT( !IsFull( ) );
	T& theResult = m_Data[m_End];
	Increment( m_End );
	UpdateFullStatus( );
	++m_Count;
	return theResult;
}

//==============================================================================
/**		
 *	Remove the top entry from the array
 */
template <class T> inline
void CCircularArray<T>::Pop( )
{
        Q3DStudio_ASSERT( !IsEmpty( ) );
	Increment( m_Begin );
	m_Full = false;
	--m_Count;
}

//==============================================================================
/**		
 *	Get the top item from the 
 *	@return a reference to the item
 */
template <class T> inline
T& CCircularArray<T>::Top( )
{
        Q3DStudio_ASSERT( !IsEmpty( ) );
	return m_Data[m_Begin];
}

//==============================================================================
/**		
 *	Check if the array is empty
 *	@return true if empty, false otherwise.
 */
template <class T> inline
BOOL CCircularArray<T>::IsEmpty( ) const
{
	return ( m_Begin == m_End ) && !m_Full;
}

//==============================================================================
/**		
 *	Check if the array is full
 *	@return true if full, false otherwise.
 */
template <class T> inline
BOOL CCircularArray<T>::IsFull( ) const
{
	return m_Full;
}

//==============================================================================
/**		
 *	Increment index of the array
 *	@param outIndex		the index to increment
 */
template <class T> inline
void CCircularArray<T>::Increment( INT32& outIndex )
{
	++outIndex;
	// circular list wrap-around
	if ( outIndex >= m_Capacity )
		outIndex = 0;
}

//==============================================================================
/**		
 *	Decrement index of the array
 *	@param outIndex		the index to decrement
 */
template <class T> inline
void CCircularArray<T>::Decrement( INT32& outIndex )
{
	--outIndex;
	// circular list wrap-around
	if ( outIndex < 0 )
		outIndex = m_Capacity - 1;
}

//==============================================================================
/**		
 *	Update the status of the m_Full flag.
 *	This is an internal method for use ONLY AFTER a item has been added to the array.
 */
template <class T> inline
void CCircularArray<T>::UpdateFullStatus( )
{
	m_Full = ( m_End == m_Begin );
}

} // namespace Q3DStudio
