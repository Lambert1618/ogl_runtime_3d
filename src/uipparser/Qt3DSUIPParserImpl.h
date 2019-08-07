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
/*
This is the main file responsible for conversion from the uip format into the runtime's data format.
The runtime works in a sort of global space so any object, be it an animation, action or element,
once
activated stays activated.  Furthermore it is important that the runtime declare and set the fewest
number
of properties possible to still get the right answer.  In order to achieve this the runtime relies
on the
rendering system to have the initial state of the entire scene graph, including state that is only
"Add"
commands declared in slides.

Given that you understand master slide/ slide distinction, an accurate description would be to do
this:
1.  Move all parse objects (actions, animations, elements) to be master objects and deactivate them.
2.  Running through the list of nonmaster slides, activate the objects on those slides that would be
activated.
3.  Now remove properties that are both unreferenced and applied 1 or fewer times.


The parsing system does this in reverse order so the passes look like:
1.  Load meta data information (scripts, effects) to have a complete meta data model.
1.  Build element graph.  This is used during attribute lookup
2.  Figure out which attributes will be on the elements in the graph by resolving behavior
references, action references and
        Animations, and Set command attribute applications.

        During this pass we also build the set of actions and animations which will appear on each
slide.  This involves both promoting
        actions/animations that appear on the master slide to child slides and pulling
animations/actions declared on all child slides
        onto the master slide.

3.  Count the objects that we will need.  The runtime currently requires fixed object counts that
can't be exceeded
        thus we have to effectively preallocate all runtime objects.

4.  Run through the file again and build the actual runtime datastructures.  Actions and animations
are built from
        an intermediate memory representation but the set commands and element activations are built
via running through
        the file.


Care needs to be taken so the runtime can survive poorly formed or just meaningless uip files.
Studio is only *one*
system for producing UIP files, clients are free to use code generates (like ruby scripts and such)
to build their final
uip files so we need to be careful that we are as robust as possible and can tolerate gracefully
some level of incorrect
uip file data.
*/

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSUIPParser.h"
#include "Qt3DSTypes.h"
#include "Qt3DSMetadata.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSSlideSystem.h"
#include "Qt3DSDMDataTypes.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include <EASTL/map.h>
#include <EASTL/set.h>
#include <EASTL/hash_map.h>
#include "Qt3DSElementSystem.h"
#include "Qt3DSSlideSystem.h"

namespace qt3ds {
namespace render {
    class IRefCountedInputStream;
}
}

namespace qt3dsdm {
class IDOMReader;
class IDOMWriter;
}
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class IPresentation;
class SElementBuilder;
class CContractBuilder;
class CAnimationBuilder;
class CLogicBuilder;
class CSlideBuilder;
class IRuntimeMetaData;
class CUIPParserActionHelper;
class CUIPParserObjectRefHelper;

typedef eastl::vector<qt3ds::runtime::element::TPropertyDescAndValue> TPropertyDescAndValueList;

using qt3ds::render::IInputStreamFactory;
using qt3ds::render::IRefCountedInputStream;
typedef NVScopedRefCounted<IRefCountedInputStream> TInputStreamPtr;
typedef qt3ds::foundation::CRegisteredString TStrType;
using qt3ds::runtime::element::SElement;

struct SElementPropertyInfo
{
    enum Enum {
        MaxArity = 4,
    };
    TStrType m_PropertyName;
    ERuntimeDataModelDataType m_DataType;
    ERuntimeAdditionalMetaDataType m_AdditionalType;
    // Number of sub components of the property, 1-4
    qt3ds::QT3DSU32 m_Arity;
    qt3ds::QT3DSU32 m_PropertyHashes[MaxArity];
    CRegisteredString m_PropertyNames[MaxArity];
    bool m_ElementFlag; // True if this property exists on the element.  Some properties do not.
    bool m_SlideFlag; // True if this property exists on the slides at all.
    // starttime,endtime are forced to be on the slides whether they are in the UIP or not.
    bool m_SlideForceFlag; // True if this property is *forced* on the slides whether it needs to be
                           // there or not

    SElementPropertyInfo(TStrType inPropName)
        : m_PropertyName(inPropName)
        , m_DataType(ERuntimeDataModelDataTypeNone)
        , m_Arity(0)
        , m_ElementFlag(false)
        , m_SlideFlag(false)
        , m_SlideForceFlag(false)
    {
        memZero(m_PropertyHashes, sizeof(m_PropertyHashes));
    }
    bool operator<(const SElementPropertyInfo &inOther) const
    {
        return m_PropertyName < inOther.m_PropertyName;
    }
};

typedef eastl::hash_map<TStrType, SElementPropertyInfo> TAttrMap;

struct SElementData
{
    SElement *m_Element;
    TStrType m_Id;
    TStrType m_Type;
    TStrType m_Class;
    TAttrMap m_PropertyMap;

    SElementData(TStrType inId)
        : m_Element(NULL)
        , m_Id(inId)
    {
    }
    void SetElement(SElement *inElement, TStrType inType, TStrType inClass)
    {
        m_Element = inElement;
        m_Type = inType;
        m_Class = inClass;
    }
};

typedef eastl::hash_map<TStrType, SElementData> TIdElementMap;

struct SParseElementManager
{
    IRuntimeMetaData &m_MetaData;
    TIdElementMap m_ElementMap;
    // Temporary variables for various algorithms
    eastl::string m_Workspace;
    eastl::vector<eastl::string> m_PropertyList;
    qt3ds::foundation::IStringTable &m_StringTable;

    SParseElementManager(IRuntimeMetaData &inMetaData, qt3ds::foundation::IStringTable &inStrTable)
        : m_MetaData(inMetaData)
        , m_StringTable(inStrTable)
    {
    }
    TStrType RegisterStr(const char8_t *inStr);
    SElementData &GetOrCreateElementData(const char8_t *inElemIdOrRef, const char8_t *inElemType,
                                         const char8_t *inElemClassId);
    SElementData *FindElementData(const char8_t *inIdOrRef);
    // We don't allow creation of properties that don't exist in the meta data in the first place.
    SElementPropertyInfo *GetOrCreateProperty(SElementData &inData, const char8_t *inPropertyName);
    SElementPropertyInfo *GetOrCreateProperty(const char8_t *inIdOrRef, const char8_t *inElemType,
                                              const char8_t *inElemClassId,
                                              const char8_t *inPropertyName);
    SElementPropertyInfo *FindProperty(SElementData &inElement, const char8_t *inPropertyName);
    SElementPropertyInfo *FindProperty(const char8_t *inIdOrRef, const char8_t *inPropertyName);
    eastl::pair<SElementPropertyInfo *, qt3ds::QT3DSU32>
    FindPropertyWithSubIndex(SElementData &inElement, const char8_t *inPropertyName);
    // Attribute marking stage; mark which attributes will be kept on elements at all.
    void MarkAttributeAsReferenced(SElementData &inElement, const char8_t *inPropertyName);
    void MarkAttributeAsReferenced(const char8_t *inElement, const char8_t *inPropertyName);
    void MarkAllAttributesAsReferenced(SElementData &inElement, bool searchParent = false);
};

struct SParseSlideActionEntry
{
    TStrType m_SlideId;
    TStrType m_ActionId;
    qt3ds::QT3DSI32 m_ActionCount;
    bool m_Active;
    qt3ds::runtime::SSlideAnimAction *m_Actions[SElementPropertyInfo::MaxArity];

    SParseSlideActionEntry(TStrType inSlideId, TStrType inActionId, qt3ds::QT3DSI32 inActionCount,
                           bool inActive)
        : m_SlideId(inSlideId)
        , m_ActionId(inActionId)
        , m_ActionCount(inActionCount)
        , m_Active(inActive)
    {
        memZero(m_Actions, sizeof(m_Actions));
    }
    SParseSlideActionEntry()
        : m_ActionCount(0)
        , m_Active(false)
    {
    }
    // Partial equals, used for finding object in vector.
    bool operator==(const SParseSlideActionEntry &inOther) const
    {
        return m_SlideId == inOther.m_SlideId && m_ActionId == inOther.m_ActionId;
    }
};

typedef eastl::vector<SParseSlideActionEntry> TActionList;

struct SParseSlideAnimationTypes
{
    enum Enum {
        NoAnimation = 0,
        Linear,
        EaseInOut,
        Bezier,
    };
};

struct SParseSlideAnimationEntry
{
    TStrType m_SlideId;
    TStrType m_InstanceId;
    TStrType m_PropertyName;
    qt3ds::QT3DSU32 m_PropertyHash;
    qt3ds::QT3DSI32 m_AnimationIndex;
    eastl::vector<float> m_KeyframeData;
    SParseSlideAnimationTypes::Enum m_AnimationType;
    bool m_IsDynamic;

    SParseSlideAnimationEntry()
        : m_AnimationIndex(-1)
        , m_KeyframeData("")
        , m_AnimationType(SParseSlideAnimationTypes::NoAnimation)
        , m_IsDynamic(false)
    {
    }
    SParseSlideAnimationEntry(TStrType inSlideId, TStrType inInstanceId, TStrType inPropertyName,
                              qt3ds::QT3DSU32 inPropHash, SParseSlideAnimationTypes::Enum inAnimType,
                              const float *inKeyframeData, qt3ds::QT3DSU32 inNumFloats, bool inIsDynamic)
        : m_SlideId(inSlideId)
        , m_InstanceId(inInstanceId)
        , m_PropertyName(inPropertyName)
        , m_PropertyHash(inPropHash)
        , m_AnimationType(inAnimType)
        , m_IsDynamic(inIsDynamic)
    {
        m_KeyframeData.insert(m_KeyframeData.begin(), inKeyframeData, inKeyframeData + inNumFloats);
    }
};

struct SParseAnimationRef
{
    TStrType m_SlideId;
    TStrType m_InstanceId;
    TStrType m_PropertyName;
    bool m_Active;
    const SParseSlideAnimationEntry *m_Animation;

    SParseAnimationRef(TStrType inSlideId, TStrType inInstanceId, TStrType inPropName,
                       const SParseSlideAnimationEntry *inAnimation, bool inActive)
        : m_SlideId(inSlideId)
        , m_InstanceId(inInstanceId)
        , m_PropertyName(inPropName)
        , m_Active(inActive)
        , m_Animation(inAnimation)
    {
    }
    SParseAnimationRef()
        : m_Active(false)
        , m_Animation(NULL)
    {
    }
    // Partial equals, used for finding object in vector.
    bool operator==(const SParseAnimationRef &inOther) const
    {
        return m_SlideId == inOther.m_SlideId && m_InstanceId == inOther.m_InstanceId
            && m_PropertyName == inOther.m_PropertyName;
    }
};

typedef eastl::vector<SParseAnimationRef> TAnimationList;

typedef eastl::hash_map<TStrType, TAnimationList> TInstanceIdAnimationMap;

struct SParseSlide
{
    TStrType m_SlideId;
    qt3ds::QT3DSI32 m_SlideIndex;
    TActionList m_ActionList;
    TInstanceIdAnimationMap m_Animations;

    SParseSlide(TStrType inId, qt3ds::QT3DSI32 inSlideIndex)
        : m_SlideId(inId)
        , m_SlideIndex(inSlideIndex)
    {
    }
    SParseSlide() {}
};

typedef eastl::hash_map<TStrType, SParseSlide> TSlideIdToParseSlideMap;

struct SParseSlideManager
{
    IRuntimeMetaData &m_MetaData;
    SParseElementManager &m_ElementManager;
    TSlideIdToParseSlideMap m_SlideMap;
    eastl::vector<SParseSlideAnimationEntry *> m_Animations;
    qt3ds::QT3DSI32 m_SlideAnimationCount;
    qt3ds::QT3DSI32 m_AnimationKeyframeCount;
    qt3ds::QT3DSI32 m_ActionCount;
    eastl::vector<qt3ds::QT3DSF32> m_KeyframeParseFloatBuffer;
    eastl::string m_KeyframeParseBuffer;

    SParseSlideManager(IRuntimeMetaData &inMD, SParseElementManager &inElemMgr)
        : m_MetaData(inMD)
        , m_ElementManager(inElemMgr)
        , m_SlideAnimationCount(0)
        , m_AnimationKeyframeCount(0)
        , m_ActionCount(0)
    {
    }
    ~SParseSlideManager();
    TStrType RegisterId(const char8_t *inStr);
    TStrType RegisterStr(const char8_t *inStr) { return m_ElementManager.RegisterStr(inStr); }
    SParseSlide &GetOrCreateSlide(const char8_t *inSlideId, qt3ds::QT3DSI32 inSlideIndex);
    SParseSlide *FindSlide(const char8_t *inSlideId);
    // Create an animation and reference that animation
    void SetAnimationData(SParseSlide &inSlide, const char8_t *inInstanceId,
                          const char8_t *inPropertyName, const char8_t *inKeyframeData,
                          SParseSlideAnimationTypes::Enum inType, bool inIsDynamic,
                          bool inIsActive);
    // Reference an animation.
    void ReferenceAnimation(SParseSlide &inSlide, const SParseSlideAnimationEntry &inSource,
                            bool inIsActive);
    TAnimationList *GetAnimationsForInstance(SParseSlide &inSlide, const char8_t *inInstanceId);
    SParseSlideActionEntry &GetOrCreateAction(SParseSlide &inSlide, const char8_t *inActionId,
                                              qt3ds::QT3DSI32 inActionCount, bool inEyeball);
    SParseSlideActionEntry *FindAction(SParseSlide &inSlide, const char8_t *inActionId);
};

//==============================================================================
/**
 *	@class	CUIPParserImpl
 *	@brief	Class for parsing UIP file
 */
class CUIPParserImpl : public IUIPParser
{

    typedef eastl::basic_string<char8_t> TNarrowStr;

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    std::shared_ptr<qt3dsdm::IDOMReader> m_DOMReader;
    std::shared_ptr<qt3dsdm::IDOMWriter> m_DOMWriter;
    IRuntimeMetaData &m_MetaData; ///< Reference to Metadata
    IInputStreamFactory &m_InputStreamFactory;
    SParseElementManager m_ParseElementManager; ///< Map of id, SElement*
    SParseSlideManager m_ParseSlideManager;

    CUIPParserActionHelper *m_ActionHelper; ///< Action Helper
    CUIPParserObjectRefHelper *m_ObjectRefHelper; ///< Object Reference Helper

    typedef eastl::set<SElement *> TAddedSlideElements;
    TAddedSlideElements m_SlideElements; ///< Caching of slide elements added, so that we don't need
                                         ///to pass this around

    typedef eastl::map<eastl::string, eastl::string> TIdSourcePathMap;
    TIdSourcePathMap m_IdScriptMap; ///< Map of the class id and script sourcepath

    typedef eastl::map<eastl::string, eastl::string> TIdClassMap;
    TIdClassMap m_IdClassMap; ///< Map of id and class reference

    typedef eastl::set<eastl::string> TStringSet;
    typedef eastl::vector<eastl::string> TStringVector;

    TStringSet m_SourcePathSet;
    TStringVector m_SourcePathList;
    TStringSet m_iblSources;
    QVector<QString> m_slideSourcePaths;

    struct SElementRefCache
    {
        SElementData *m_Element;
        eastl::string m_Name;
        eastl::string m_Value;
    };
    eastl::vector<SElementRefCache> m_ElementRefCache; ///< Cache those element refs in the scene
                                                       ///graph that needs to be patch later

    struct SEaseInEaseOutKeyframe
    {
        float m_Time;
        float m_Value;
        float m_EaseIn;
        float m_EaseOut;
    };

    struct SGraphSectionCount
    {
        INT32 m_ElementCount;
        INT32 m_ComponentCount;
        INT32 m_AttributeCount;
        INT32 m_StringAttrCount;

        SGraphSectionCount()
            : m_ElementCount(0)
            , m_ComponentCount(0)
            , m_AttributeCount(0)
            , m_StringAttrCount(0)
        {
        }
    };

    struct SLogicSectionCount
    {
        INT32 m_SlideCount;
        INT32 m_SlideElementCount;
        INT32 m_SlideAttributeCount;
        INT32 m_StringAttrCount;
        INT32 m_PaddingCount;

        SLogicSectionCount()
            : m_SlideCount(0)
            , m_SlideElementCount(0)
            , m_SlideAttributeCount(0)
            , m_StringAttrCount(0)
            , m_PaddingCount(0)
        {
        }
    };

    struct SActionSectionCount
    {
        INT32 m_TriggerElementCount;
        INT32 m_TriggerEventCount;
        INT32 m_ActionCount;
        INT32 m_StringAttrCount;
        INT32 m_CustomParamCount;

        SActionSectionCount()
            : m_TriggerElementCount(0)
            , m_TriggerEventCount(0)
            , m_ActionCount(0)
            , m_StringAttrCount(0)
            , m_CustomParamCount(0)
        {
        }
    };

    long m_NumGraphElements; /// eclee testing only, must remove
    long m_NumGraphComponents; /// eclee testing only, must remove
    long m_NumGraphAttributes; /// eclee testing only, must remove
    long m_NumSlides; /// eclee testing only, must remove
    long m_NumSlideElements; /// eclee testing only, must remove
    long m_NumSlideAttributes; /// eclee testing only, must remove
    long m_NumAnimationTracks; /// eclee testing only, must remove
    long m_NumAnimationKeys; /// eclee testing only, must remove
    long m_NumSlideAnimations; /// eclee testing only, must remove
    long m_NumSlideActions; /// klarinda testing only, must remove

    // temporarily this is here
    qt3ds::foundation::MemoryBuffer<RawAllocator> m_TempBuffer;
    qt3ds::foundation::MemoryBuffer<RawAllocator> m_ValueBuffer;

    IPresentation *m_CurrentPresentation;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CUIPParserImpl(const QString &inFileName, IRuntimeMetaData &inMetaData,
                   IInputStreamFactory &inFactory, qt3ds::foundation::IStringTable &inStringTable);
    virtual ~CUIPParserImpl();

public: // Parse UIP file
    BOOL Load(IPresentation &inPresentation,
              NVConstDataRef<SElementAttributeReference> inStateReferences,
              bool initInRenderThread) override;
    qt3dsdm::IDOMReader &GetDOMReader() override;
    IRuntimeMetaData &GetMetaData() override;
    SElementAndType GetElementForID(const char *inStringId) override;
    eastl::string ResolveReference(const char *inStringId, const char *inReferance) override;
    NVConstDataRef<eastl::string> GetSourcePaths() const override
    {
        return NVConstDataRef<eastl::string>(m_SourcePathList.data(), m_SourcePathList.size());
    }
    QVector<QString> GetSlideSourcePaths() const override
    {
        return m_slideSourcePaths;
    }

    bool isIblImage(const eastl::string &sourcepath) const override
    {
        return m_iblSources.find(sourcepath) != m_iblSources.end();
    }

protected: // Operation
    BOOL LoadProjectSettings(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader);
    BOOL LoadClasses(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader);
    BOOL LoadGraph(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader,
                   bool initInRenderThread);
    BOOL LoadSceneGraph(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader,
                        qt3ds::runtime::element::SElement *inNewStyleParent,
                        bool initInRenderThread);
    BOOL LoadLogic(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader);
    BOOL LoadStateGraph(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader);

protected: // Memory Counter
    void ComputeAndReserveMemory(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader);
    void DoGraphSectionCount(qt3dsdm::IDOMReader &inReader, SGraphSectionCount &outGraphCounter);
    void DoLogicSectionCount(qt3dsdm::IDOMReader &inReader, SLogicSectionCount &outLogicCounter);
    void DoStateSectionCount(qt3dsdm::IDOMReader &inReader, INT32 inSlideIndex,
                             SLogicSectionCount &outLogicCounter);
    void DoRefSectionCount(qt3dsdm::IDOMReader &inReader, INT32 inNumSlideMultiplier,
                           INT32 inSlideIndex, SLogicSectionCount &outLogicCounter);

protected: // Scene graph heler
    void CacheGraphRequiredAttributes(qt3dsdm::IDOMReader &inReader);
    BOOL CacheLogicRequiredAttributes(qt3dsdm::IDOMReader &inReader, qt3ds::QT3DSI32 inSlideIndex = 0,
                                      SParseSlide *inParentSlide = NULL);
    void CacheClassRequiredAttributes(qt3dsdm::IDOMReader &inReader);
    void CacheClassSceneAttributes(qt3dsdm::IDOMReader &inReader);
    void CacheClassStateAttributes(qt3dsdm::IDOMReader &inReader);
    void AddElementRefToPatch(TPropertyDescAndValueList &outDescList, SElementData &inElement,
                              const char *inName, const char *inValue);
    void PatchSceneElementRef();

protected: // Slide helper
    BOOL LoadState(IPresentation &inPresentation, SElement *inComponent, INT32 inSlideIndex,
                   qt3dsdm::IDOMReader &inReader);
    qt3ds::runtime::SSlidePlayInformation GetPlayMode(qt3dsdm::IDOMReader &inReader);
    INT32 GetPlayThroughTo(INT32 inCurrentSlideIndex, qt3dsdm::IDOMReader &inReader);
    eastl::string GetSlideName(qt3dsdm::IDOMReader &inReader);
    eastl::string GetSlideId(qt3dsdm::IDOMReader &inReader);
    BOOL LoadSlideElements(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader,
                           bool inMaster, SElement *inComponent, INT32 *outMaxTime = NULL);
    SElementData *AddSlideElement(IPresentation &inPresentation, bool inMaster,
                                  qt3dsdm::IDOMReader &inReader, INT32 *outMaxTime = NULL);
    BOOL LoadSlideElementAttrs(IPresentation &inPresentation, bool inMaster,
                               SElementData &inElementData, qt3dsdm::IDOMReader &inReader,
                               SElement *inComponent);
    BOOL ProcessStateRef(IPresentation &inPresentation, SElement *inComponent, bool inMaster,
                         qt3dsdm::IDOMReader &inReader);

protected: // Animation helper
    void LoadAnimationTrack(IPresentation &inPresentation, SElementData &inElementData,
                            SParseSlideAnimationEntry &inAnimation);
    void LoadAnimationKeys(IPresentation &inPresentation,
                           const SParseSlideAnimationEntry &inAnimation, bool inIsRotation);
    BOOL LoadBezierKeys(IPresentation &inPresentation, NVConstDataRef<float> &inValues,
                        bool inIsRotation);
    BOOL LoadLinearKeys(IPresentation &inPresentation, NVConstDataRef<float> &inValues,
                        bool inIsRotation);
    BOOL LoadEaseInOutKeys(IPresentation &inPresentation, NVConstDataRef<float> &inValues,
                           bool inIsRotation);
    SEaseInEaseOutKeyframe ParseEaseInOutKey(const float *&inFloatIter);
    BOOL ProcessSlideAnimAction(IPresentation &inPresentation, eastl::string &inSlideId,
                                bool inMaster, qt3dsdm::IDOMReader &inReader);
    BOOL AddSlideAction(IPresentation &inPresentation, eastl::string &inSlideId, bool inActive,
                        qt3dsdm::IDOMReader &inReader);
    void CreateBezierKeyframeFromEaseInEaseOutKeyframe(float *inPreviousValue,
                                                       SEaseInEaseOutKeyframe &inCurrent,
                                                       float *inNextValue, float *&outBezierValues);

protected: // Helper methods
    bool IsStringType(ERuntimeDataModelDataType inDataType);
    void GetAttributeList(IPresentation &inPresentation, TPropertyDescAndValueList &outDescList,
                          TStrType inType, TStrType inName, TStrType inClassId, const char *inValue,
                          CRegisteredString *inPropNameStrs);
    void GetAttributeList(IPresentation &inPresentation, TPropertyDescAndValueList &outDescList,
                          ERuntimeDataModelDataType inDataType,
                          ERuntimeAdditionalMetaDataType inAdditionalType, const char *inName,
                          const char *inValue, CRegisteredString *inPropNameStrs);

    SElementData *GetElementData(TStrType inElementName);
    SElementData *GetElementData(const char8_t *inElementName);
    SElement *GetElement(const char8_t *inElementName);
    // Returns the data type and a boolean indicating if this attribute is actually used on a
    // particular slide.
    eastl::pair<ERuntimeDataModelDataType, bool>
    GetAttributeType(const char8_t *inElementName, const char8_t *inPropertyName,
                     const SElementData &inElementData);
    SElementData *ParseObjectRef(const eastl::string &inObjectPath, const char8_t *inOwnerId);
    TStrType ParseObjectRefId(const eastl::string &inObjectPath, const char8_t *inOwnerId);

    void GetMetaAttribute(IPresentation &inPresentation, TPropertyDescAndValueList &outDescList,
                          TStrType inType, TStrType inName, TStrType inClassId,
                          CRegisteredString *inAttStrNames);

protected:
    void AddFloatAttribute(TPropertyDescAndValueList &outDescList, CRegisteredString inAttStrName,
                           float &inValue);
    void AddLongAttribute(TPropertyDescAndValueList &outDescList, CRegisteredString inAttStrName,
                          qt3ds::QT3DSI32 &inValue);
    void AddBoolAttribute(TPropertyDescAndValueList &outDescList, CRegisteredString inAttStrName,
                          bool &inValue);
    void AddFloat2Attribute(TPropertyDescAndValueList &outDescList,
                            CRegisteredString *inAttStrNames, qt3dsdm::SFloat2 &inValue);
    void AddFloat3Attribute(TPropertyDescAndValueList &outDescList,
                            ERuntimeAdditionalMetaDataType inAdditionalType,
                            CRegisteredString *inAttStrNames, qt3dsdm::SFloat3 &inValue);
    void AddFloat4Attribute(TPropertyDescAndValueList &outDescList,
                            ERuntimeAdditionalMetaDataType inAdditionalType,
                            CRegisteredString *inAttStrNames, qt3dsdm::SFloat4 &inValue);
    void AddStringAttribute(IPresentation &inPresentation, TPropertyDescAndValueList &outDescList,
                            CRegisteredString inAttStrName, const char *inValue);
    void AddElementRefAttribute(TPropertyDescAndValueList &outDescList,
                                CRegisteredString inAttStrName, SElement *inElement);

    void AddSourcePath(const char *inValue, bool ibl)
    {
        if (m_SourcePathSet.find(inValue) == m_SourcePathSet.end()) {
            m_SourcePathSet.insert(inValue);
            m_SourcePathList.push_back(eastl::string(inValue));
        }
        if (ibl && m_iblSources.find(inValue) == m_iblSources.end())
            m_iblSources.insert(inValue);
    }

public: //
    void release() override { delete this; }

    //==============================================================================
    //	Friends
    //==============================================================================
    friend class CUIPParserActionHelper;
};

} // namespace Q3DStudio
