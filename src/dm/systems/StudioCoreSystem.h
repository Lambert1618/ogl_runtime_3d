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
#ifndef STUDIOCORESYSTEMH
#define STUDIOCORESYSTEMH
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMHandles.h"

namespace qt3dsdm {
class IDataCore;
class ISlideCore;
class ISlideGraphCore;
class IAnimationCore;
class IActionCore;
class ICustomPropCore;
class ISignalConnection;
class IMetaData;
class IGuideSystem;

// Manages cascading operations between cores so that the entire group of
// cores stays up to date.
class CStudioCoreSystem : public ITransactionProducer
{
    Q_DISABLE_COPY(CStudioCoreSystem)

    TStringTablePtr m_StringTable;
    std::shared_ptr<IDataCore> m_DataCore;
    std::shared_ptr<ISlideCore> m_SlideCore;
    std::shared_ptr<ISlideGraphCore> m_SlideGraphCore;
    std::shared_ptr<IAnimationCore> m_AnimationCore;
    std::shared_ptr<IActionCore> m_ActionCore;
    std::shared_ptr<ICustomPropCore> m_CustomPropCore;
    std::shared_ptr<IMetaData> m_NewMetaData;
    std::shared_ptr<IGuideSystem> m_GuideSystem;

    std::vector<std::shared_ptr<ISignalConnection>> m_Connections;

public:
    CStudioCoreSystem(TStringTablePtr strTable = TStringTablePtr());
    virtual ~CStudioCoreSystem();
    TStringTablePtr GetStringTablePtr() const { return m_StringTable; }

    std::shared_ptr<IDataCore> GetDataCore();
    std::shared_ptr<ISlideCore> GetSlideCore();
    std::shared_ptr<ISlideGraphCore> GetSlideGraphCore();
    std::shared_ptr<IAnimationCore> GetAnimationCore();
    std::shared_ptr<IActionCore> GetActionCore();
    std::shared_ptr<ICustomPropCore> GetCustomPropCore();
    std::shared_ptr<IMetaData> GetNewMetaData();
    std::shared_ptr<IGuideSystem> GetGuideSystem();

    std::shared_ptr<IDataCore> GetTransactionlessDataCore();
    std::shared_ptr<ISlideCore> GetTransactionlessSlideCore();
    std::shared_ptr<ISlideGraphCore> GetTransactionlessSlideGraphCore();
    std::shared_ptr<IAnimationCore> GetTransactionlessAnimationCore();
    std::shared_ptr<IActionCore> GetTransactionlessActionCore();
    std::shared_ptr<ICustomPropCore> GetTransactionlessCustomPropCore();

    Qt3DSDMInstanceHandle FindInstanceByName(Qt3DSDMPropertyHandle inNameProperty,
                                            const TCharStr &inName) const;

    void SetConsumer(TTransactionConsumerPtr inConsumer) override;
};

typedef std::shared_ptr<CStudioCoreSystem> TStudioCoreSystemPtr;
}

#endif
