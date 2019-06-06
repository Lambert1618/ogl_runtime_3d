/****************************************************************************
**
** Copyright (C) 1993-2010 NVIDIA Corporation.
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
// Includes
//==============================================================================

namespace Q3DStudio {

//==============================================================================
/**
*	Interface for a function wrapper with an associated cost.
*/
class IFunctionWrapper
{
public:
    IFunctionWrapper();
    virtual ~IFunctionWrapper(){}

public:
    virtual void Execute() = 0;
    INT32 GetCost();
    void SetCost(const INT32 inCost);

protected:
    INT32 m_Cost;
};

//==============================================================================
/**
*	2 arguments function wrapper
*/
template <typename TArg1, typename TArg2>
class CFunctionWrapper2Args : public IFunctionWrapper
{
public:
    typedef void (*TFunction)(TArg1 inArg1, TArg2 inArg2);

public:
    CFunctionWrapper2Args(TFunction, TArg1 inArg1, TArg2 inArg2);

public:
    virtual void Execute();

protected:
    TFunction m_Function;
    TArg1 m_Arg1;
    TArg2 m_Arg2;
};

// Unused
/*
template <typename TArg1, typename TArg2, typename TArg3>
class CFunctionWrapper3Args : public IFunctionWrapper
{
        public:
                typedef void ( *TFunction )( TArg1 inArg1, TArg2 inArg2, TArg3 inArg3 );

        public:
                CFunctionWrapper3Args( TFunction inFunction, TArg1 inArg1, TArg2 inArg2, TArg3
inArg3 );

        public:
                virtual void Execute( );

        protected:
                TFunction m_Function;
                TArg1 m_Arg1;
                TArg2 m_Arg2;
                TArg3 m_Arg3;
};

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4>
class CFunctionWrapper4Args : public IFunctionWrapper
{
        public:
                typedef void ( *TFunction )( TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4 inArg4
);

        public:
                CFunctionWrapper4Args( TFunction inFunction, TArg1 inArg1, TArg2 inArg2, TArg3
inArg3, TArg4 inArg4 );

        public:
                virtual void Execute( );

        protected:
                TFunction m_Function;
                TArg1 m_Arg1;
                TArg2 m_Arg2;
                TArg3 m_Arg3;
                TArg4 m_Arg4;
};

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename
TArg6, typename TArg7, typename TArg8>
class CFunctionWrapper8Args : public IFunctionWrapper
{
        public:
                typedef void ( *TFunction )( TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4 inArg4,
TArg5 inArg5, TArg6 inArg6, TArg7 inArg7, TArg8 inArg8 );

        public:
                CFunctionWrapper8Args( TFunction inFunction, TArg1 inArg1, TArg2 inArg2, TArg3
inArg3, TArg4 inArg4, TArg5 inArg5, TArg6 inArg6, TArg7 inArg7, TArg8 inArg8 );

        public:
                virtual void Execute( );

        protected:
                TFunction m_Function;
                TArg1 m_Arg1;
                TArg2 m_Arg2;
                TArg3 m_Arg3;
                TArg4 m_Arg4;
                TArg5 m_Arg5;
                TArg6 m_Arg6;
                TArg7 m_Arg7;
                TArg8 m_Arg8;
};

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename
TArg6, typename TArg7, typename TArg8, typename TArg9>
class CFunctionWrapper9Args : public IFunctionWrapper
{
        public:
                typedef void ( *TFunction )( TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4 inArg4,
TArg5 inArg5, TArg6 inArg6, TArg7 inArg7, TArg8 inArg8, TArg9 inArg9 );

        public:
                CFunctionWrapper9Args( TFunction inFunction, TArg1 inArg1, TArg2 inArg2, TArg3
inArg3, TArg4 inArg4, TArg5 inArg5, TArg6 inArg6, TArg7 inArg7, TArg8 inArg8, TArg9 inArg9 );

        public:
                virtual void Execute( );

        protected:
                TFunction m_Function;
                TArg1 m_Arg1;
                TArg2 m_Arg2;
                TArg3 m_Arg3;
                TArg4 m_Arg4;
                TArg5 m_Arg5;
                TArg6 m_Arg6;
                TArg7 m_Arg7;
                TArg8 m_Arg8;
                TArg9 m_Arg9;
};
*/

//==============================================================================
/**
*	1 argument member function wrapper
*/
template <typename TObject, typename TReturn, typename TArg1>
class CMemberFunctionWrapper1Args : public IFunctionWrapper
{
public:
    typedef TReturn (TObject::*TFunction)(TArg1 inArg1);

public:
    CMemberFunctionWrapper1Args(TObject *inObject, TFunction inFunction, TArg1 inArg1);

public:
    virtual void Execute();

protected:
    TFunction m_Function;
    TObject *m_Object;
    TArg1 m_Arg1;
};

//==============================================================================
/**
*	2 argument member function wrapper
*/
template <typename TObject, typename TReturn, typename TArg1, typename TArg2>
class CMemberFunctionWrapper2Args : public IFunctionWrapper
{
public:
    typedef TReturn (TObject::*TFunction)(TArg1 inArg1, TArg2 inArg2);

public:
    CMemberFunctionWrapper2Args(TObject *inObject, TFunction inFunction, TArg1 inArg1,
                                TArg2 inArg2);

public:
    virtual void Execute();

protected:
    TFunction m_Function;
    TObject *m_Object;
    TArg1 m_Arg1;
    TArg2 m_Arg2;
};

//==============================================================================
/**
*	6 argument member function wrapper
*/
template <typename TObject, typename TReturn, typename TArg1, typename TArg2, typename TArg3,
          typename TArg4, typename TArg5, typename TArg6>
class CMemberFunctionWrapper6Args : public IFunctionWrapper
{
public:
    typedef TReturn (TObject::*TFunction)(TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4 inArg4,
                                          TArg5 inArg5, TArg6 inArg6);

public:
    CMemberFunctionWrapper6Args(TObject *inObject, TFunction inFunction, TArg1 inArg1, TArg2 inArg2,
                                TArg3 inArg3, TArg4 inArg4, TArg5 inArg5, TArg6 inArg6);

public:
    virtual void Execute();

protected:
    TFunction m_Function;
    TObject *m_Object;
    TArg1 m_Arg1;
    TArg2 m_Arg2;
    TArg3 m_Arg3;
    TArg4 m_Arg4;
    TArg5 m_Arg5;
    TArg6 m_Arg6;
};

// Implementation
template <typename TArg1, typename TArg2>
CFunctionWrapper2Args<TArg1, TArg2>::CFunctionWrapper2Args(TFunction inFunction, TArg1 inArg1,
                                                           TArg2 inArg2)
    : m_Function(inFunction)
    , m_Arg1(inArg1)
    , m_Arg2(inArg2)
{
}

template <typename TArg1, typename TArg2>
void CFunctionWrapper2Args<TArg1, TArg2>::Execute()
{
    m_Function(m_Arg1, m_Arg2);
}

// Unused
/*
template <typename TArg1, typename TArg2, typename TArg3>
CFunctionWrapper3Args<TArg1, TArg2, TArg3>::CFunctionWrapper3Args( TFunction inFunction, TArg1
inArg1, TArg2 inArg2, TArg3 inArg3 )
:	m_Function( inFunction ),
        m_Arg1( inArg1 ),
        m_Arg2( inArg2 ),
        m_Arg3( inArg3 )
{
}

template <typename TArg1, typename TArg2, typename TArg3>
void CFunctionWrapper3Args<TArg1, TArg2, TArg3>::Execute( )
{
        m_Function( m_Arg1, m_Arg2, m_Arg3 );
}

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4>
CFunctionWrapper4Args<TArg1, TArg2, TArg3, TArg4>::CFunctionWrapper4Args( TFunction inFunction,
TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4 inArg4 )
:	m_Function( inFunction ),
        m_Arg1( inArg1 ),
        m_Arg2( inArg2 ),
        m_Arg3( inArg3 ),
        m_Arg4( inArg4 )
{
}

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4>
void CFunctionWrapper4Args<TArg1, TArg2, TArg3, TArg4>::Execute( )
{
        m_Function( m_Arg1, m_Arg2, m_Arg3, m_Arg4 );
}

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename
TArg6, typename TArg7, typename TArg8>
CFunctionWrapper8Args<TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7,
TArg8>::CFunctionWrapper8Args( TFunction inFunction, TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4
inArg4, TArg5 inArg5, TArg6 inArg6, TArg7 inArg7, TArg8 inArg8 )
:	m_Function( inFunction ),
        m_Arg1( inArg1 ),
        m_Arg2( inArg2 ),
        m_Arg3( inArg3 ),
        m_Arg4( inArg4 ),
        m_Arg5( inArg5 ),
        m_Arg6( inArg6 ),
        m_Arg7( inArg7 ),
        m_Arg8( inArg8 )

{
}

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename
TArg6, typename TArg7, typename TArg8>
void CFunctionWrapper8Args<TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7, TArg8>::Execute( )
{
        m_Function( m_Arg1, m_Arg2, m_Arg3, m_Arg4, m_Arg5, m_Arg6, m_Arg7, m_Arg8 );
}

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename
TArg6, typename TArg7, typename TArg8, typename TArg9>
CFunctionWrapper9Args<TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7, TArg8,
TArg9>::CFunctionWrapper9Args( TFunction inFunction, TArg1 inArg1, TArg2 inArg2, TArg3 inArg3, TArg4
inArg4, TArg5 inArg5, TArg6 inArg6, TArg7 inArg7, TArg8 inArg8, TArg9 inArg9 )
:	m_Function( inFunction ),
        m_Arg1( inArg1 ),
        m_Arg2( inArg2 ),
        m_Arg3( inArg3 ),
        m_Arg4( inArg4 ),
        m_Arg5( inArg5 ),
        m_Arg6( inArg6 ),
        m_Arg7( inArg7 ),
        m_Arg8( inArg8 ),
        m_Arg9( inArg9 )
{
}

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4, typename TArg5, typename
TArg6, typename TArg7, typename TArg8, typename TArg9>
void CFunctionWrapper9Args<TArg1, TArg2, TArg3, TArg4, TArg5, TArg6, TArg7, TArg8, TArg9>::Execute(
)
{
        m_Function( m_Arg1, m_Arg2, m_Arg3, m_Arg4, m_Arg5, m_Arg6, m_Arg7, m_Arg8, m_Arg9 );
}
*/

template <typename TObject, typename TReturn, typename TArg1>
CMemberFunctionWrapper1Args<TObject, TReturn, TArg1>::CMemberFunctionWrapper1Args(
    TObject *inObject, TFunction inFunction, TArg1 inArg1)
    : m_Function(inFunction)
    , m_Object(inObject)
    , m_Arg1(inArg1)
{
}

template <typename TObject, typename TReturn, typename TArg1>
void CMemberFunctionWrapper1Args<TObject, TReturn, TArg1>::Execute()
{
    (m_Object->*m_Function)(m_Arg1);
}

template <typename TObject, typename TReturn, typename TArg1, typename TArg2>
CMemberFunctionWrapper2Args<TObject, TReturn, TArg1, TArg2>::CMemberFunctionWrapper2Args(
    TObject *inObject, TFunction inFunction, TArg1 inArg1, TArg2 inArg2)
    : m_Function(inFunction)
    , m_Object(inObject)
    , m_Arg1(inArg1)
    , m_Arg2(inArg2)
{
}

template <typename TObject, typename TReturn, typename TArg1, typename TArg2>
void CMemberFunctionWrapper2Args<TObject, TReturn, TArg1, TArg2>::Execute()
{
    (m_Object->*m_Function)(m_Arg1, m_Arg2);
}

template <typename TObject, typename TReturn, typename TArg1, typename TArg2, typename TArg3,
          typename TArg4, typename TArg5, typename TArg6>
CMemberFunctionWrapper6Args<TObject, TReturn, TArg1, TArg2, TArg3, TArg4, TArg5,
                            TArg6>::CMemberFunctionWrapper6Args(TObject *inObject,
                                                                TFunction inFunction, TArg1 inArg1,
                                                                TArg2 inArg2, TArg3 inArg3,
                                                                TArg4 inArg4, TArg5 inArg5,
                                                                TArg6 inArg6)
    : m_Function(inFunction)
    , m_Object(inObject)
    , m_Arg1(inArg1)
    , m_Arg2(inArg2)
    , m_Arg3(inArg3)
    , m_Arg4(inArg4)
    , m_Arg5(inArg5)
    , m_Arg6(inArg6)
{
}

template <typename TObject, typename TReturn, typename TArg1, typename TArg2, typename TArg3,
          typename TArg4, typename TArg5, typename TArg6>
void CMemberFunctionWrapper6Args<TObject, TReturn, TArg1, TArg2, TArg3, TArg4, TArg5,
                                 TArg6>::Execute()
{
    (m_Object->*m_Function)(m_Arg1, m_Arg2, m_Arg3, m_Arg4, m_Arg5, m_Arg6);
}
}
