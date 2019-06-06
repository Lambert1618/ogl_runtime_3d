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
#ifndef QT3DS_STATE_SCRIPT_CONTEXT_H
#define QT3DS_STATE_SCRIPT_CONTEXT_H
#pragma once
#include "Qt3DSState.h"
#include "Qt3DSStateTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/StringTable.h"

namespace qt3ds {
namespace state {

    class IScriptEvent : public IEvent
    {
    public:
        // returns true on success, false if error.execution should be signaled.
        virtual bool SetParam(CRegisteredString inName, const char8_t *inExpression) = 0;
        // Sets the expression as a string
        virtual bool SetParamStr(CRegisteredString inName, const char8_t *inStr) = 0;
        virtual bool SetDataExpr(const char8_t *inExpression) = 0;
        virtual bool SetDataStr(const char8_t *inStr) = 0;
    };

    struct SScriptExecutionResult
    {
        const char8_t *m_Result;
        const char8_t *m_Error;
        SScriptExecutionResult(const char8_t *inData, const char8_t *inError)
            : m_Result(inData)
            , m_Error(inError)
        {
        }
        SScriptExecutionResult()
            : m_Result("")
            , m_Error("")
        {
        }
        bool Valid() const { return m_Error == NULL || *m_Error == 0; }
        const char8_t *Result() const
        {
            QT3DS_ASSERT(Valid());
            return m_Result;
        }
        const char8_t *Error() const
        {
            QT3DS_ASSERT(!Valid());
            return m_Error;
        }
    };

    class IScriptContext : public NVRefCounted
    {
    protected:
        virtual ~IScriptContext() {}
    public:
        // Should functions returning options return othing
        virtual Option<bool> ExecuteCondition(const char8_t *inCond) = 0;
        // Used for logging.
        // Error and result are good until next call into the script context.
        virtual SScriptExecutionResult ExecuteExpressionToString(const char8_t *inExpr) = 0;

        // If return value is false, error is signaled with GetErrorInfo.
        virtual bool Assign(const char8_t *inVariable, const char8_t *inExpr) = 0;
        // Assign a string to this variable location.
        virtual void AssignStr(const char8_t *inVariable, const char8_t *inStr) = 0;

        // If return value is false, error is signaled via GetErrorInfo.
        // The actual pointer and content used for inscript is expected to not change during
        // execution of the system.
        // it is legal for contexts to cache information based off the script pointer value.
        virtual bool ExecuteScript(const char8_t *inScript) = 0;
        // Always return 1 result
        virtual int ExecuteStr(const char8_t *inScript, bool withRet) = 0;

        // Return true on success, false on failure, and Empty on error.
        virtual Option<bool> BeginForeach(const char8_t *inArray, const char8_t *inItem,
                                          const char8_t *inIdxVar = "") = 0;
        virtual Option<bool> NextForeach(const char8_t *inItem, const char8_t *inIdxVar = "") = 0;
        virtual void CancelForeach() = 0;

        virtual void SetCurrentEvent(TEventPtr inEvent) = 0;
        virtual void ClearCurrentEvent() = 0;
        // Create an event we can attach extra data do.
        virtual IScriptEvent *CreateScriptEvent(CRegisteredString inName) = 0;

        virtual const char8_t *GetErrorInfo() = 0;

        virtual void SetInterpreter(IStateInterpreter &inInterpreter) = 0;

        virtual CRegisteredString GetContextType() { return CRegisteredString(); }
    };
}
}
#endif
