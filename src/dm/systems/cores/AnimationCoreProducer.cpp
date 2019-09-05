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
#include "Qt3DSDMPrefix.h"
#include "AnimationCoreProducer.h"
#include "HandleSystemTransactions.h"
#include "VectorTransactions.h"
#include "SignalsImpl.h"
#ifdef _WIN32
#pragma warning(disable : 4512) // assignment operator not generated
#endif

using namespace std;

namespace qt3dsdm {

struct SArtistEditedUndoRedoScope
{
    TSimpleAnimationCorePtr m_AnimationCore;
    Qt3DSDMAnimationHandle m_Animation;
    TTransactionConsumerPtr m_Consumer;
    bool m_ArtistEdited;
    SArtistEditedUndoRedoScope(TSimpleAnimationCorePtr inAnimCore, Qt3DSDMAnimationHandle inAnim,
                               TTransactionConsumerPtr inConsumer)
        : m_AnimationCore(inAnimCore)
        , m_Animation(inAnim)
        , m_Consumer(inConsumer)
        , m_ArtistEdited(inAnimCore->IsArtistEdited(inAnim))
    {
    }
    ~SArtistEditedUndoRedoScope()
    {
        bool edited = m_AnimationCore->IsArtistEdited(m_Animation);
        if (m_Consumer && edited != m_ArtistEdited) {
            m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
                std::bind(&CSimpleAnimationCore::SetIsArtistEdited, m_AnimationCore, m_Animation,
                            edited),
                std::bind(&CSimpleAnimationCore::SetIsArtistEdited, m_AnimationCore, m_Animation,
                            m_ArtistEdited))));
        }
    }
};

struct SLookupCacheDoUndoOp : public ITransaction
{
    std::shared_ptr<SAnimationTrack> m_Animation;
    TSimpleAnimationCorePtr m_AnimationCore;
    bool m_AddOnDo;
    SLookupCacheDoUndoOp(const char *inFile, int inLine, Qt3DSDMAnimationHandle inAnimHandle,
                         TSimpleAnimationCorePtr inCore, bool addOnDo)
        : ITransaction(inFile, inLine)
        , m_AnimationCore(inCore)
        , m_AddOnDo(addOnDo)
    {
        THandleObjectMap::const_iterator theIter(inCore->m_Objects.find(inAnimHandle));
        if (theIter != inCore->m_Objects.end())
            m_Animation = static_pointer_cast<SAnimationTrack>(theIter->second);
    }
    void Remove()
    {
        if (m_Animation)
            m_AnimationCore->RemoveAnimationFromLookupCache(m_Animation);
    }
    void Add()
    {
        if (m_Animation)
            m_AnimationCore->AddAnimationToLookupCache(m_Animation);
    }
    void Do() override
    {
        if (m_AddOnDo)
            Add();
        else
            Remove();
    }
    void Undo() override
    {
        if (m_AddOnDo)
            Remove();
        else
            Add();
    }
};

Qt3DSDMAnimationHandle
CAnimationCoreProducer::CreateAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                        Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                        EAnimationType inAnimationType, bool inFirstKeyframeDynamic)
{
    Qt3DSDMAnimationHandle retval = m_Data->CreateAnimation(inSlide, inInstance, inProperty, inIndex,
                                                           inAnimationType, inFirstKeyframeDynamic);

    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    if (m_Consumer)
        m_Consumer->OnTransaction(
            std::make_shared<SLookupCacheDoUndoOp>(__FILE__, __LINE__, retval, m_Data, true));
    GetSignalSender()->SendAnimationCreated(retval, inSlide, inInstance, inProperty, inIndex,
                                            inAnimationType);
    return retval;
}

void CAnimationCoreProducer::DeleteAnimation(Qt3DSDMAnimationHandle inAnimation)
{
    // Ensure animation exists
    SAnimationTrack *theAnimation =
        CSimpleAnimationCore::GetAnimationNF(inAnimation, m_Data->m_Objects);
    GetSignalSender()->SendBeforeAnimationDeleted(inAnimation);
    if (m_Consumer)
        m_Consumer->OnTransaction(std::make_shared<SLookupCacheDoUndoOp>(
            __FILE__, __LINE__, inAnimation, m_Data, false));
    CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inAnimation, m_Data->m_Objects);
    do_all(theAnimation->m_Keyframes,
           std::bind(DoCreateHandleDeleteTransaction, __FILE__, __LINE__, m_Consumer,
                     std::placeholders::_1, std::ref(m_Data->m_Objects)));
    SAnimationInfo theInfo = m_Data->GetAnimationInfo(inAnimation);
    m_Data->DeleteAnimation(inAnimation);
    GetSignalSender()->SendAnimationDeleted(inAnimation, theInfo.m_Slide, theInfo.m_Instance,
                                            theInfo.m_Property, theInfo.m_Index,
                                            theInfo.m_AnimationType);
}

Qt3DSDMAnimationHandle CAnimationCoreProducer::GetAnimation(Qt3DSDMSlideHandle inSlide,
                                                           Qt3DSDMInstanceHandle inInstance,
                                                           Qt3DSDMPropertyHandle inProperty,
                                                           size_t inIndex) const
{
    return m_Data->GetAnimation(inSlide, inInstance, inProperty, inIndex);
}

SAnimationInfo CAnimationCoreProducer::GetAnimationInfo(Qt3DSDMAnimationHandle inAnimation) const
{
    return m_Data->GetAnimationInfo(inAnimation);
}

void CAnimationCoreProducer::GetAnimations(TAnimationHandleList &outAnimations) const
{
    return m_Data->GetAnimations(outAnimations);
}

void CAnimationCoreProducer::GetAnimations(TAnimationInfoList &outAnimations,
                                           Qt3DSDMSlideHandle inMaster,
                                           Qt3DSDMSlideHandle inSlide) const
{
    return m_Data->GetAnimations(outAnimations, inMaster, inSlide);
}

void CAnimationCoreProducer::SetFirstKeyframeDynamic(Qt3DSDMAnimationHandle inAnimation,
                                                     bool inValue)
{
    SAnimationInfo theInfo(m_Data->GetAnimationInfo(inAnimation));
    SArtistEditedUndoRedoScope __editedScope(m_Data, inAnimation, m_Consumer);
    if (m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSimpleAnimationCore::SetFirstKeyframeDynamic, m_Data, inAnimation,
                        inValue),
            std::bind(&CSimpleAnimationCore::SetFirstKeyframeDynamic, m_Data, inAnimation,
                        theInfo.m_DynamicFirstKeyframe))));
    }
    m_Data->SetFirstKeyframeDynamic(inAnimation, inValue);
    GetSignalSender()->SendFirstKeyframeDynamicSet(inAnimation, inValue);
}

inline void DirtyKeyframes(Qt3DSDMAnimationHandle inAnimation, THandleObjectMap &inObjects)
{
    SAnimationTrack *theAnimation = CSimpleAnimationCore::GetAnimationNF(inAnimation, inObjects);
    theAnimation->m_KeyframesDirty = true;
}

inline void CreateDirtyKeyframesTransaction(TTransactionConsumerPtr inConsumer,
                                            Qt3DSDMAnimationHandle inAnimation,
                                            THandleObjectMap &inObjects)
{
    if (inConsumer) {
        inConsumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(DirtyKeyframes, inAnimation, std::ref(inObjects)),
            std::bind(DirtyKeyframes, inAnimation, std::ref(inObjects)))));
    }
}

Qt3DSDMKeyframeHandle CAnimationCoreProducer::InsertKeyframe(Qt3DSDMAnimationHandle inAnimation,
                                                            const TKeyframe &inKeyframe)
{
    SArtistEditedUndoRedoScope __editedScope(m_Data, inAnimation, m_Consumer);
    Qt3DSDMKeyframeHandle retval = m_Data->InsertKeyframe(inAnimation, inKeyframe);
    SAnimationTrack *theAnimation =
        CSimpleAnimationCore::GetAnimationNF(inAnimation, m_Data->m_Objects);
    CreateVecInsertTransaction<Qt3DSDMKeyframeHandle>(__FILE__, __LINE__, m_Consumer, retval,
                                                     theAnimation->m_Keyframes);
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    CreateDirtyKeyframesTransaction(m_Consumer, inAnimation, m_Data->m_Objects);
    GetSignalSender()->SendKeyframeInserted(inAnimation, retval, inKeyframe);
    return retval;
}

void CAnimationCoreProducer::EraseKeyframe(Qt3DSDMKeyframeHandle inKeyframe)
{
    SKeyframe *theKeyframe = CSimpleAnimationCore::GetKeyframeNF(inKeyframe, m_Data->m_Objects);
    SArtistEditedUndoRedoScope __editedScope(m_Data, theKeyframe->m_Animation, m_Consumer);
    GetSignalSender()->SendBeforeKeyframeErased(inKeyframe);
    SAnimationTrack *theAnimation =
        CSimpleAnimationCore::GetAnimationNF(theKeyframe->m_Animation, m_Data->m_Objects);
    if (exists(theAnimation->m_Keyframes, std::bind(equal_to<int>(), inKeyframe,
                                                    std::placeholders::_1))) {
        CreateVecEraseTransaction<Qt3DSDMKeyframeHandle>(__FILE__, __LINE__, m_Consumer, inKeyframe,
                                                        theAnimation->m_Keyframes);
        CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inKeyframe, m_Data->m_Objects);
        CreateDirtyKeyframesTransaction(m_Consumer, theKeyframe->m_Animation, m_Data->m_Objects);
    }
    int theKeyframeHandle = theKeyframe->m_Handle;
    TKeyframe theData = theKeyframe->m_Keyframe;
    m_Data->EraseKeyframe(inKeyframe);
    GetSignalSender()->SendKeyframeErased(theAnimation->m_Handle, theKeyframeHandle, theData);
}

template <typename TDataType>
struct VectorSwapTransaction : public ITransaction
{
    vector<TDataType> m_Data;
    vector<TDataType> &m_Target;
    VectorSwapTransaction(const char *inFile, int inLine, vector<TDataType> &inTarget)
        : ITransaction(inFile, inLine)
        , m_Data(inTarget)
        , m_Target(inTarget)
    {
    }
    void Do() override { std::swap(m_Data, m_Target); }
    void Undo() override { Do(); }
};

void CAnimationCoreProducer::DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation)
{
    // Ensure animation exists
    SAnimationTrack *theAnimation =
        CSimpleAnimationCore::GetAnimationNF(inAnimation, m_Data->m_Objects);
    SArtistEditedUndoRedoScope __editedScope(m_Data, inAnimation, m_Consumer);
    GetSignalSender()->SendBeforeAllKeyframesErased(inAnimation);
    do_all(theAnimation->m_Keyframes,
           std::bind(DoCreateHandleDeleteTransaction, __FILE__, __LINE__, m_Consumer,
                     std::placeholders::_1, std::ref(m_Data->m_Objects)));
    if (m_Consumer)
        m_Consumer->OnTransaction(std::make_shared<VectorSwapTransaction<Qt3DSDMKeyframeHandle>>(
            __FILE__, __LINE__, std::ref(theAnimation->m_Keyframes)));
    theAnimation->m_Keyframes.clear();
}

Qt3DSDMAnimationHandle
CAnimationCoreProducer::GetAnimationForKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const
{
    return m_Data->GetAnimationForKeyframe(inKeyframe);
}

struct KeyframeDataTransaction : public ITransaction, public IMergeableTransaction<TKeyframe>
{
    TSimpleAnimationCorePtr m_AnimationCore;
    Qt3DSDMKeyframeHandle m_Keyframe;
    TKeyframe m_OldData;
    TKeyframe m_NewData;

    KeyframeDataTransaction(const char *inFile, int inLine, TSimpleAnimationCorePtr animCore,
                            Qt3DSDMKeyframeHandle keyframe, TKeyframe oldData, TKeyframe newData)
        : ITransaction(inFile, inLine)
        , m_AnimationCore(animCore)
        , m_Keyframe(keyframe)
        , m_OldData(oldData)
        , m_NewData(newData)
    {
    }

    void Do() override { m_AnimationCore->DoSetKeyframeData(m_Keyframe, m_NewData); }
    void Undo() override { m_AnimationCore->DoSetKeyframeData(m_Keyframe, m_OldData); }

    void Update(const TKeyframe &inKeyframe) override { m_NewData = inKeyframe; }
};

void CAnimationCoreProducer::SetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe,
                                             const TKeyframe &inData)
{
    SKeyframe *theKeyframe = CSimpleAnimationCore::GetKeyframeNF(inKeyframe, m_Data->m_Objects);
    SArtistEditedUndoRedoScope __editedScope(m_Data, theKeyframe->m_Animation, m_Consumer);
    TKeyframe theNewData(inData);
    TKeyframe theOldData = theKeyframe->m_Keyframe;
    m_Data->SetKeyframeData(inKeyframe, inData);
    TKeyframeDataMergeMap::iterator iter(m_KeyframeMergeMap.find(inKeyframe));
    if (iter != m_KeyframeMergeMap.end())
        iter->second->Update(theNewData);
    else {
        if (m_Consumer) {
            std::shared_ptr<KeyframeDataTransaction> theKeyframeTransaction(
                std::make_shared<KeyframeDataTransaction>(__FILE__, __LINE__, m_Data, inKeyframe,
                                                            theOldData, theNewData));
            m_Consumer->OnTransaction(static_pointer_cast<ITransaction>(theKeyframeTransaction));
            m_KeyframeMergeMap.insert(make_pair(
                inKeyframe,
                static_pointer_cast<IMergeableTransaction<TKeyframe>>(theKeyframeTransaction)));
        }
        GetSignalSender()->SendKeyframeUpdated(inKeyframe, inData);
    }
}

TKeyframe CAnimationCoreProducer::GetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe) const
{
    return m_Data->GetKeyframeData(inKeyframe);
}

void CAnimationCoreProducer::GetKeyframes(Qt3DSDMAnimationHandle inAnimation,
                                          TKeyframeHandleList &outKeyframes) const
{
    return m_Data->GetKeyframes(inAnimation, outKeyframes);
}

size_t CAnimationCoreProducer::GetKeyframeCount(Qt3DSDMAnimationHandle inAnimation) const
{
    return m_Data->GetKeyframeCount(inAnimation);
}

void CAnimationCoreProducer::OffsetAnimations(Qt3DSDMSlideHandle slide,
                                              Qt3DSDMInstanceHandle instance,
                                              long timeOffset)
{
    for (THandleObjectMap::const_iterator iter = m_Data->m_Objects.begin(),
                                          end = m_Data->m_Objects.end();
         iter != end; ++iter) {
        SAnimationTrack *theTrack = static_cast<SAnimationTrack *>(iter->second.get());
        if (theTrack->m_Slide == slide && theTrack->m_Instance == instance) {
            for (size_t keyframeIdx = 0, keyframeEnd = theTrack->m_Keyframes.size();
                 keyframeIdx < keyframeEnd; ++keyframeIdx) {
                Qt3DSDMKeyframeHandle theKeyframeHandle(theTrack->m_Keyframes[keyframeIdx]);
                TKeyframe kfData = m_Data->GetKeyframeData(theKeyframeHandle);

                // offset control points for bezier keyframes
                offsetBezier(kfData, timeOffset);

                kfData = setKeyframeTime(kfData, getKeyframeTime(kfData) + timeOffset);
                SetKeyframeData(theKeyframeHandle, kfData);
            }
        }
    }
}

void CAnimationCoreProducer::SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited)
{
    SArtistEditedUndoRedoScope __editedScope(m_Data, inAnimation, m_Consumer);
    m_Data->SetIsArtistEdited(inAnimation, inEdited);
}

bool CAnimationCoreProducer::IsArtistEdited(Qt3DSDMAnimationHandle inAnimation) const
{
    return m_Data->IsArtistEdited(inAnimation);
}
// Animation Evaluation.
float CAnimationCoreProducer::EvaluateAnimation(Qt3DSDMAnimationHandle animation, long time) const
{
    return m_Data->EvaluateAnimation(animation, time);
}

std::pair<float, float> CAnimationCoreProducer::getAnimationExtrema(
                                                        Qt3DSDMAnimationHandle animation,
                                                        long startTime, long endTime) const
{
    return m_Data->getAnimationExtrema(animation, startTime, endTime);
}

bool CAnimationCoreProducer::KeyframeValid(Qt3DSDMKeyframeHandle inKeyframe) const
{
    return m_Data->KeyframeValid(inKeyframe);
}

bool CAnimationCoreProducer::AnimationValid(Qt3DSDMAnimationHandle inAnimation) const
{
    return m_Data->AnimationValid(inAnimation);
}

void CAnimationCoreProducer::CopyAnimations(Qt3DSDMSlideHandle inSourceSlide,
                                            Qt3DSDMInstanceHandle inSourceInstance,
                                            Qt3DSDMSlideHandle inDestSlide,
                                            Qt3DSDMInstanceHandle inDestInstance)
{
    std::vector<SAnimationTrack *> theAnimations;
    for (THandleObjectMap::const_iterator iter = m_Data->m_Objects.begin(),
                                          end = m_Data->m_Objects.end();
         iter != end; ++iter) {
        SAnimationTrack *theTrack = static_cast<SAnimationTrack *>(iter->second.get());
        if (theTrack->m_Instance == inSourceInstance && theTrack->m_Slide == inSourceSlide)
            theAnimations.push_back(theTrack);
    }
    for (size_t idx = 0, end = theAnimations.size(); idx < end; ++idx) {
        const SAnimationTrack &newTrack(*theAnimations[idx]);
        Qt3DSDMAnimationHandle theNewAnimation(
            CreateAnimation(inDestSlide, inDestInstance, newTrack.m_Property, newTrack.m_Index,
                            newTrack.m_AnimationType, newTrack.m_FirstKeyframeDynamic));
        for (size_t keyIdx = 0, keyEnd = newTrack.m_Keyframes.size(); keyIdx < keyEnd; ++keyIdx)
            InsertKeyframe(theNewAnimation, GetKeyframeData(newTrack.m_Keyframes[keyIdx]));
    }
}

// ITransactionProducer implementation
void CAnimationCoreProducer::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_Consumer = inConsumer;
    m_KeyframeMergeMap.clear();
}

TSignalConnectionPtr CAnimationCoreProducer::ConnectAnimationCreated(
    const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                               Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback)
{
    return GetSignalProvider()->ConnectAnimationCreated(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectBeforeAnimationDeleted(
    const std::function<void(Qt3DSDMAnimationHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectBeforeAnimationDeleted(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectAnimationDeleted(
    const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                               Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback)
{
    return GetSignalProvider()->ConnectAnimationDeleted(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectKeyframeInserted(
    const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
        &inCallback)
{
    return GetSignalProvider()->ConnectKeyframeInserted(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectBeforeKeyframeErased(
    const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectBeforeKeyframeErased(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectKeyframeErased(
    const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
        &inCallback)
{
    return GetSignalProvider()->ConnectKeyframeErased(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectBeforeAllKeyframesErased(
    const std::function<void(Qt3DSDMAnimationHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectBeforeAllKeyframesErased(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectKeyframeUpdated(
    const std::function<void(Qt3DSDMKeyframeHandle, const TKeyframe &)> &inCallback)
{
    return GetSignalProvider()->ConnectKeyframeUpdated(inCallback);
}
TSignalConnectionPtr CAnimationCoreProducer::ConnectFirstKeyframeDynamicSet(
    const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback)
{
    return GetSignalProvider()->ConnectFirstKeyframeDynamicSet(inCallback);
}

void CAnimationCoreProducer::InitSignaller()
{
    m_Signaller = CreateAnimationCoreSignaller();
}

IAnimationCoreSignalProvider *CAnimationCoreProducer::GetSignalProvider()
{
    return dynamic_cast<IAnimationCoreSignalProvider *>(m_Signaller.get());
}

IAnimationCoreSignalSender *CAnimationCoreProducer::GetSignalSender()
{
    return dynamic_cast<IAnimationCoreSignalSender *>(m_Signaller.get());
}
}
