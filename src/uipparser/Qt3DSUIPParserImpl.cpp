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

#include "RuntimePrefix.h"
#ifdef EA_PLATFORM_WINDOWS
#pragma warning(disable : 4396) // specializer warning nonsense
#endif

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSIPresentation.h"
#include "Qt3DSIScriptBridge.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "Qt3DSUIPParserActionHelper.h"
#include "Qt3DSUIPParserObjectRefHelper.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
#include "foundation/Qt3DSFoundation.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSAnimationSystem.h"
#include "Qt3DSSlideSystem.h"
#include "Qt3DSRenderDynamicObjectSystem.h"

using namespace qt3dsdm;

#ifndef M_PI
#define M_PI 3.1415926535898
#endif

#define TODEG(x) x = (float)(x * 180 / M_PI);
#define TORAD(x) x = (float)(x / 180 * M_PI);

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

TStrType SParseElementManager::RegisterStr(const char8_t *inStr)
{
    return m_MetaData.GetStringTable()->GetRenderStringTable().RegisterStr(inStr);
}
SElementData &SParseElementManager::GetOrCreateElementData(const char8_t *inIdOrRef,
                                                           const char8_t *inElemType,
                                                           const char8_t *inElemClassId)
{
    if (inIdOrRef == NULL)
        inIdOrRef = "";
    if (inIdOrRef[0] == '#')
        ++inIdOrRef;
    if (inElemClassId == NULL)
        inElemClassId = "";
    if (inElemClassId[0] == '#')
        ++inElemClassId;
    if (inElemType == NULL)
        inElemType = "";
    TStrType theStr = RegisterStr(inIdOrRef);
    eastl::pair<TIdElementMap::iterator, bool> theInserter =
        m_ElementMap.insert(eastl::make_pair(theStr, SElementData(theStr)));
    SElementData &retval(theInserter.first->second);
    if (theInserter.second) {
        retval.m_Class = RegisterStr(inElemClassId);
        retval.m_Type = RegisterStr(inElemType);
    }
    return retval;
}
SElementData *SParseElementManager::FindElementData(const char8_t *inIdOrRef)
{
    if (inIdOrRef == NULL)
        inIdOrRef = "";
    if (inIdOrRef[0] == '#')
        ++inIdOrRef;
    TStrType theStr = RegisterStr(inIdOrRef);
    TIdElementMap::iterator theIter = m_ElementMap.find(theStr);
    if (theIter != m_ElementMap.end())
        return &theIter->second;
    return NULL;
}
static void SetPropertyValueHash(eastl::string &inWorkspace, const char8_t *inExtension,
                                 qt3ds::QT3DSU32 *&ioHashes, eastl::string::size_type originalSize,
                                 CRegisteredString *&ioPropNames,
                                 qt3ds::foundation::IStringTable &ioStringTable)
{
    if (inExtension && *inExtension)
        inWorkspace.append(inExtension);
    *ioHashes = Q3DStudio::CHash::HashAttribute(inWorkspace.c_str());
    *ioPropNames = ioStringTable.RegisterStr(inWorkspace.c_str());
    ++ioHashes;
    ++ioPropNames;
    inWorkspace.resize(originalSize);
}
static eastl::pair<qt3ds::QT3DSU32, eastl::string::size_type>
SetupPropertyWorkspace(const char8_t *inPropName, eastl::string &ioWorkspace)
{
    qt3ds::QT3DSU32 outSubIndex = 0;
    ioWorkspace.assign(inPropName ? inPropName : "");
    eastl::string::size_type thePeriodPos = ioWorkspace.find('.');
    if (thePeriodPos != eastl::string::npos) {
        if (thePeriodPos < ioWorkspace.size() - 1) {
            switch (ioWorkspace[thePeriodPos + 1]) {
            default:
                QT3DS_ASSERT(false);
            case 'r':
            case 'x':
                break;
            case 'g':
            case 'y':
                outSubIndex = 1;
                break;
            case 'b':
            case 'z':
                outSubIndex = 2;
                break;
            case 'a':
            case 'w':
                outSubIndex = 3;
                break;
            }
        }
        ioWorkspace.resize(thePeriodPos);
    }
    return eastl::make_pair(ioWorkspace.size(), outSubIndex);
}
SElementPropertyInfo *SParseElementManager::GetOrCreateProperty(SElementData &inData,
                                                                const char8_t *inPropertyName)
{
    SElementData &theData(inData);
    SElementPropertyInfo *retvalPtr = FindProperty(inData, inPropertyName);
    if (retvalPtr)
        return retvalPtr;

    eastl::string::size_type thePeriodPos =
        SetupPropertyWorkspace(inPropertyName, m_Workspace).first;
    TStrType theStr = RegisterStr(m_Workspace.c_str());

    if (m_MetaData.IsPropertyExist(inData.m_Type, theStr, inData.m_Class) == false)
        return NULL;

    eastl::pair<TAttrMap::iterator, bool> theInserter =
        theData.m_PropertyMap.insert(eastl::make_pair(theStr, SElementPropertyInfo(theStr)));
    SElementPropertyInfo &retval(theInserter.first->second);
    if (theInserter.second) {
        qt3ds::intrinsics::memZero(retval.m_PropertyHashes, sizeof(retval.m_PropertyHashes));
        qt3ds::QT3DSU32 *thePropHashes(retval.m_PropertyHashes);
        CRegisteredString *thePropNames(retval.m_PropertyNames);
        retval.m_DataType = m_MetaData.GetPropertyType(theData.m_Type, theStr, theData.m_Class);
        retval.m_AdditionalType =
            m_MetaData.GetAdditionalType(theData.m_Type, theStr, theData.m_Class);
        if (retval.m_DataType != ERuntimeDataModelDataTypeNone) {
            switch (retval.m_DataType) {
            default:
                retval.m_Arity = 1;
                break;
            case ERuntimeDataModelDataTypeFloat2:
                retval.m_Arity = 2;
                break;
            case ERuntimeDataModelDataTypeFloat3:
                retval.m_Arity = 3;
                break;
            case ERuntimeDataModelDataTypeFloat4:
                retval.m_Arity = 4;
                break;
            }
            if (retval.m_Arity > 1) {
                if (retval.m_Arity == 2) {
                    SetPropertyValueHash(m_Workspace, ".x", thePropHashes, thePeriodPos,
                                         thePropNames, m_StringTable);
                    SetPropertyValueHash(m_Workspace, ".y", thePropHashes, thePeriodPos,
                                         thePropNames, m_StringTable);
                } else if (retval.m_Arity == 3) {
                    if (m_MetaData.GetAdditionalType(theData.m_Type, theStr, theData.m_Class)
                        == ERuntimeAdditionalMetaDataTypeColor) {
                        SetPropertyValueHash(m_Workspace, ".r", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".g", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".b", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                    } else {
                        SetPropertyValueHash(m_Workspace, ".x", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".y", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".z", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                    }
                } else if (retval.m_Arity == 4) {
                    if (m_MetaData.GetAdditionalType(theData.m_Type, theStr, theData.m_Class)
                        == ERuntimeAdditionalMetaDataTypeColor) {
                        SetPropertyValueHash(m_Workspace, ".r", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".g", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".b", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".a", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                    } else {
                        SetPropertyValueHash(m_Workspace, ".x", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".y", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".z", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                        SetPropertyValueHash(m_Workspace, ".w", thePropHashes, thePeriodPos,
                                             thePropNames, m_StringTable);
                    }
                }
            } else {
                retval.m_PropertyHashes[0] = Q3DStudio::CHash::HashAttribute(theStr.c_str());
                retval.m_PropertyNames[0] = m_StringTable.RegisterStr(m_Workspace.c_str());
            }
        }
    }
    return &retval;
}

SElementPropertyInfo *SParseElementManager::GetOrCreateProperty(const char8_t *inIdOrRef,
                                                                const char8_t *inElemType,
                                                                const char8_t *inElemClassId,
                                                                const char8_t *inPropertyName)
{
    return GetOrCreateProperty(GetOrCreateElementData(inIdOrRef, inElemType, inElemClassId),
                               inPropertyName);
}

eastl::pair<SElementPropertyInfo *, qt3ds::QT3DSU32>
SParseElementManager::FindPropertyWithSubIndex(SElementData &inElement,
                                               const char8_t *inPropertyName)
{
    qt3ds::QT3DSU32 subIndex = SetupPropertyWorkspace(inPropertyName, m_Workspace).second;
    TStrType theStr = RegisterStr(m_Workspace.c_str());
    TAttrMap::iterator theIter = inElement.m_PropertyMap.find(theStr);
    SElementPropertyInfo *retval = NULL;
    if (theIter != inElement.m_PropertyMap.end())
        retval = &theIter->second;
    return eastl::make_pair(retval, subIndex);
}

SElementPropertyInfo *SParseElementManager::FindProperty(SElementData &inElement,
                                                         const char8_t *inPropertyName)
{
    return FindPropertyWithSubIndex(inElement, inPropertyName).first;
}
SElementPropertyInfo *SParseElementManager::FindProperty(const char8_t *inIdOrRef,
                                                         const char8_t *inPropertyName)
{
    SElementData *theData(FindElementData(inIdOrRef));
    if (theData)
        return FindProperty(*theData, inPropertyName);
    return NULL;
}

void SParseElementManager::MarkAttributeAsReferenced(SElementData &inElement,
                                                     const char8_t *inPropertyName)
{
    SElementPropertyInfo *theProp = GetOrCreateProperty(inElement, inPropertyName);
    if (theProp)
        theProp->m_ElementFlag = true;
}

void SParseElementManager::MarkAttributeAsReferenced(const char8_t *inElement,
                                                     const char8_t *inPropertyName)
{
    SElementData *theData = FindElementData(inElement);
    if (theData)
        MarkAttributeAsReferenced(*theData, inPropertyName);
}

void SParseElementManager::MarkAllAttributesAsReferenced(SElementData &inElement,
                                                         bool searchParent)
{
    m_PropertyList.clear();
    m_MetaData.GetInstanceProperties(inElement.m_Type, inElement.m_Class, m_PropertyList,
                                     searchParent);
    for (QT3DSU32 idx = 0, end = m_PropertyList.size(); idx < end; ++idx)
        MarkAttributeAsReferenced(inElement, m_PropertyList[idx].c_str());
}

TStrType SParseSlideManager::RegisterId(const char8_t *inStr)
{
    if (!inStr)
        inStr = "";
    if (*inStr == '#')
        ++inStr;
    return RegisterStr(inStr);
}

SParseSlideManager::~SParseSlideManager()
{
    for (QT3DSU32 idx = 0, end = m_Animations.size(); idx < end; ++idx)
        delete m_Animations[idx];
    m_Animations.clear();
}

SParseSlide &SParseSlideManager::GetOrCreateSlide(const char8_t *inSlideId, qt3ds::QT3DSI32 inSlideIndex)
{
    TStrType theSlideId(RegisterId(inSlideId));
    return m_SlideMap.insert(eastl::make_pair(theSlideId, SParseSlide(theSlideId, inSlideIndex)))
        .first->second;
}

SParseSlide *SParseSlideManager::FindSlide(const char8_t *inSlideId)
{
    TStrType theSlideId(RegisterId(inSlideId));
    TSlideIdToParseSlideMap::iterator theIter = m_SlideMap.find(theSlideId);
    if (theIter != m_SlideMap.end())
        return &theIter->second;
    return NULL;
}

void SParseSlideManager::SetAnimationData(SParseSlide &inSlide, const char8_t *inInstanceId,
                                          const char8_t *inPropertyName,
                                          const char8_t *inKeyframeData,
                                          SParseSlideAnimationTypes::Enum inType, bool inIsDynamic,
                                          bool inIsActive)
{
    TStrType thePropertyName(RegisterStr(inPropertyName));
    SElementData *theElemData = m_ElementManager.FindElementData(inInstanceId);
    if (theElemData == NULL) {
        QT3DS_ASSERT(false);
        return;
    }
    eastl::pair<SElementPropertyInfo *, qt3ds::QT3DSU32> theProperty =
        m_ElementManager.FindPropertyWithSubIndex(*theElemData, thePropertyName);
    if (theProperty.first == NULL) {
        QT3DS_ASSERT(false);
        return;
    }
    qt3ds::QT3DSU32 thePropertyHash = theProperty.first->m_PropertyHashes[theProperty.second];
    // Parse the keyframe data into floating point numbers.
    if (inKeyframeData == NULL)
        inKeyframeData = "";
    m_KeyframeParseBuffer.clear();
    m_KeyframeParseBuffer.assign(inKeyframeData);
    m_KeyframeParseFloatBuffer.clear();
    char8_t *theBufData = const_cast<char8_t *>(m_KeyframeParseBuffer.c_str());

    char8_t *theStartPtr = qt3dsdm::FindNextNonWhitespace(theBufData);
    while (theStartPtr && *theStartPtr) {
        char8_t *nextPtr = qt3dsdm::FindNextWhitespace(theStartPtr);
        if (nextPtr && *nextPtr)
            *nextPtr = 0;
        else
            nextPtr = NULL;
        QT3DSF32 temp;
        WStrOps<QT3DSF32>().StrTo(theStartPtr, temp);
        m_KeyframeParseFloatBuffer.push_back(temp);
        theStartPtr = nextPtr;
        if (theStartPtr)
            theStartPtr = FindNextNonWhitespace(theStartPtr + 1);
    }
    // Don't create animations with no keyframes.
    if (m_KeyframeParseFloatBuffer.empty()) {
        return;
    }

    SParseSlideAnimationEntry *theEntry =
        new SParseSlideAnimationEntry(inSlide.m_SlideId, theElemData->m_Id, thePropertyName,
                                      thePropertyHash, inType, NULL, 0, inIsDynamic);
    theEntry->m_KeyframeData = m_KeyframeParseFloatBuffer;
    m_Animations.push_back(theEntry);
    switch (inType) {
    case SParseSlideAnimationTypes::Linear:
        m_AnimationKeyframeCount += m_KeyframeParseFloatBuffer.size() / 2;
        break;
    case SParseSlideAnimationTypes::Bezier:
        m_AnimationKeyframeCount += m_KeyframeParseFloatBuffer.size() / 6;
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    case SParseSlideAnimationTypes::EaseInOut:
        m_AnimationKeyframeCount += m_KeyframeParseFloatBuffer.size() / 4;
        break;
    }

    ReferenceAnimation(inSlide, *theEntry, inIsActive);
}

void SParseSlideManager::ReferenceAnimation(SParseSlide &inSlide,
                                            const SParseSlideAnimationEntry &inSource,
                                            bool inIsActive)
{
    TAnimationList &theList =
        inSlide.m_Animations.insert(eastl::make_pair(inSource.m_InstanceId, TAnimationList()))
            .first->second;
    theList.push_back(SParseAnimationRef(inSlide.m_SlideId, inSource.m_InstanceId,
                                         inSource.m_PropertyName, &inSource, inIsActive));
    m_SlideAnimationCount++;
}

TAnimationList *SParseSlideManager::GetAnimationsForInstance(SParseSlide &inSlide,
                                                             const char8_t *inInstanceId)
{
    SElementData *theElemData = m_ElementManager.FindElementData(inInstanceId);
    if (theElemData) {
        TInstanceIdAnimationMap::iterator theAnimations =
            inSlide.m_Animations.find(theElemData->m_Id);
        if (theAnimations != inSlide.m_Animations.end())
            return &theAnimations->second;
    }
    return NULL;
}

SParseSlideActionEntry &SParseSlideManager::GetOrCreateAction(SParseSlide &inSlide,
                                                              const char8_t *inActionId,
                                                              qt3ds::QT3DSI32 inActionCount,
                                                              bool inActive)
{
    TStrType theActionId(RegisterId(inActionId));
    SParseSlideActionEntry theAction(inSlide.m_SlideId, theActionId, inActionCount, inActive);
    memZero(theAction.m_Actions, sizeof(theAction.m_Actions));
    TActionList::iterator theIter =
        eastl::find(inSlide.m_ActionList.begin(), inSlide.m_ActionList.end(), theAction);
    if (theIter != inSlide.m_ActionList.end())
        return *theIter;
    inSlide.m_ActionList.push_back(theAction);
    m_ActionCount += inActionCount;
    return inSlide.m_ActionList.back();
}

SParseSlideActionEntry *SParseSlideManager::FindAction(SParseSlide &inSlide,
                                                       const char8_t *inActionId)
{
    TStrType theActionId(RegisterId(inActionId));
    SParseSlideActionEntry theAction(inSlide.m_SlideId, theActionId, 0, false);
    TActionList::iterator theIter =
        eastl::find(inSlide.m_ActionList.begin(), inSlide.m_ActionList.end(), theAction);
    if (theIter != inSlide.m_ActionList.end())
        return &(*theIter);
    return NULL;
}

//==============================================================================
/**
 * Constructor
 * @param	inFileName			name of UIP file
 */
CUIPParserImpl::CUIPParserImpl(const QString &inFileName, IRuntimeMetaData &inMetaData,
                               IInputStreamFactory &inFactory,
                               qt3ds::foundation::IStringTable &inStringTable)
    : m_MetaData(inMetaData)
    , m_InputStreamFactory(inFactory)
    , m_ParseElementManager(inMetaData, inStringTable)
    , m_ParseSlideManager(inMetaData, m_ParseElementManager)
{
    // Setup DOMSerializer and DOMReader to read the file
    std::shared_ptr<qt3dsdm::IStringTable> theStrTable(inMetaData.GetStringTable());
    shared_ptr<qt3dsdm::IDOMFactory> theFactory(qt3dsdm::IDOMFactory::CreateDOMFactory(theStrTable));
    TInputStreamPtr theInStream(inFactory.GetStreamForFile(inFileName));
    qt3dsdm::SDOMElement *topElement = NULL;
    if (theInStream)
        topElement = CDOMSerializer::Read(*theFactory, *theInStream);
    if (!topElement) {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Could not open UIP file: " << inFileName.toLatin1().constData();
    } else {
        eastl::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>> theWriteReader =
            IDOMWriter::CreateDOMWriter(theFactory, *topElement, theStrTable);
        m_DOMWriter = theWriteReader.first;
        m_DOMReader = theWriteReader.second;
        if (!AreEqual(m_DOMReader->GetNarrowElementName(), "UIP")) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "Invalid UIP file: " << inFileName.toLatin1().constData();
        }
    }

    // Create Helper class
    m_ObjectRefHelper = new CUIPParserObjectRefHelper(m_MetaData);
    m_ActionHelper = new CUIPParserActionHelper(this, m_ObjectRefHelper, m_MetaData);
}

//==============================================================================
/**
 * Destructor
 */
CUIPParserImpl::~CUIPParserImpl()
{
    // Delete Helper class
    delete m_ObjectRefHelper;
    m_ObjectRefHelper = NULL;

    delete m_ActionHelper;
    m_ActionHelper = NULL;
}

//==============================================================================
/**
 *	Load a presentation from a UIP file.
 *	@param	inPresentation		presentation written to
 *	@return a flag indicating whether or not we successfully loaded the file
 */
BOOL CUIPParserImpl::Load(IPresentation &inPresentation,
                          NVConstDataRef<SElementAttributeReference> inStateReferences,
                          bool initInRenderThread)
{
    m_CurrentPresentation = &inPresentation;
    if (!m_DOMReader) {
        qCCritical(qt3ds::INVALID_PARAMETER) << "CUIPParserImpl::Load, No DOM reader";
        return FALSE;
    }

    BOOL theLoadResult = m_DOMReader->MoveToFirstChild("Project");

    // Load project settings
    if (theLoadResult)
        theLoadResult &= LoadProjectSettings(inPresentation, *m_DOMReader);

    // First, we need to load the classes, because the class information will be stored in metadata
    if (theLoadResult)
        theLoadResult &= LoadClasses(inPresentation, *m_DOMReader);

    // Next is to cache the scene graph because we need the id-name information to resolve object
    // reference
    m_ObjectRefHelper->CacheGraph(*m_DOMReader, *m_DOMWriter);

    // Next is to cache the required attributes. This will be used to build Element
    // We also need to cache the actions because we need to know the ActionCount to reserve the
    // memory
    {
        IDOMReader::Scope __graphScope(*m_DOMReader);
        m_DOMReader->MoveToFirstChild("Graph");
        CacheGraphRequiredAttributes(*m_DOMReader);
    }

    {
        IDOMReader::Scope __logicScope(*m_DOMReader);
        m_DOMReader->MoveToFirstChild("Logic");
        {

            IDOMReader::Scope __stateScope(*m_DOMReader);
            for (BOOL theSuccess = m_DOMReader->MoveToFirstChild("State"); theSuccess;
                 theSuccess = m_DOMReader->MoveToNextSibling("State")) {
                IDOMReader::Scope __subSlideScope(*m_DOMReader);
                if (theLoadResult)
                    theLoadResult &= CacheLogicRequiredAttributes(*m_DOMReader);
            }
        }
    }

    // go through all the references and make sure the elements exist and if they do ensure the
    // attributes are marked as referenced.
    for (QT3DSU32 idx = 0, end = inStateReferences.size(); idx < end; ++idx) {
        const SElementAttributeReference &reference = inStateReferences[idx];
        // All references are absolute, so the start node doesn't really matter.
        CRegisteredString result = m_ObjectRefHelper->ParseObjectRefId(reference.m_Path, "Scene");
        if (result.IsValid() == false) {
            qCCritical(INVALID_OPERATION, "Failed to resolve reference: %s",
                reference.m_Path.c_str());
        } else {
            m_ParseElementManager.MarkAttributeAsReferenced(result.c_str(),
                                                            reference.m_Attribute.c_str());
        }
    }

    if (theLoadResult)
        CacheClassRequiredAttributes(*m_DOMReader);

    ComputeAndReserveMemory(inPresentation, *m_DOMReader);

    // Now we are ready to load the scene graph
    if (theLoadResult)
        theLoadResult &= LoadGraph(inPresentation, *m_DOMReader, initInRenderThread);
    if (theLoadResult)
        theLoadResult &= LoadLogic(inPresentation, *m_DOMReader);

    return theLoadResult;
}

IDOMReader &CUIPParserImpl::GetDOMReader()
{
    return *m_DOMReader.get();
}

//==============================================================================
/**
 *	Load Project Settings
 *	@param	inPresentation		presentation written to
 *	@return a flag indicating whether or not we successfully loaded the file
 */
BOOL CUIPParserImpl::LoadProjectSettings(IPresentation &inPresentation, IDOMReader &inReader)
{
    IDOMReader::Scope __projectSettingsScope(inReader);

    if (inReader.MoveToFirstChild("ProjectSettings")) {
        SPresentationSize thePresentationSize;
        qt3ds::QT3DSI32 theWidth, theHeight;

        if (inReader.Att("presentationWidth", theWidth))
            thePresentationSize.m_Width = static_cast<UINT16>(theWidth);
        if (inReader.Att("presentationHeight", theHeight))
            thePresentationSize.m_Height = static_cast<UINT16>(theHeight);
        thePresentationSize.m_ScaleMode = SCALEMODE_EXACT; // ScaleMode is always EXACT

        inPresentation.SetSize(thePresentationSize);

        return true;
    } else
        return false;
}

BOOL CUIPParserImpl::LoadClasses(IPresentation & /*inPresentation*/, IDOMReader &inReader)
{
    IDOMReader::Scope __classesScope(inReader);

    if (inReader.MoveToFirstChild("Classes")) {
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            const char *theSourcePath;
            const char *theId;
            const char *theName;
            // Read the sourcepath and id
            if (inReader.Att("sourcepath", theSourcePath) && inReader.Att("id", theId)) {
                // Read the name
                if (!inReader.Att("name", theName))
                    theName = theId;

                bool theLoadFlag = false;
                QString theFullPath(theSourcePath);

                if (theFullPath.endsWith(".qml")) {
                    theLoadFlag = m_MetaData.LoadScriptFile(inReader.GetNarrowElementName(), theId,
                                                            theName, theFullPath.toUtf8().data());
                    if (theLoadFlag)
                        m_IdScriptMap[theId] = theSourcePath;
                } else if (theFullPath.endsWith(".effect")) {
                    theLoadFlag = m_MetaData.LoadEffectXMLFile(inReader.GetNarrowElementName(),
                                                               theId, theName,
                                                               theFullPath.toUtf8().data());
                } else if (theFullPath.endsWith(".material") || theFullPath.endsWith(".shader")) {
                    theLoadFlag = m_MetaData.LoadMaterialXMLFile(
                        inReader.GetNarrowElementName(), theId, theName,
                                theFullPath.toUtf8().data());
                } else if (theFullPath.endsWith(".plugin")) {
                    theLoadFlag = m_MetaData.LoadPluginXMLFile(inReader.GetNarrowElementName(),
                                                               theId, theName,
                                                               theFullPath.toUtf8().data());
                }

                if (!theLoadFlag) {
                    // What file is this?!?
                    qCCritical(qt3ds::INVALID_OPERATION)
                            << "Could not load sourcepath file: "
                            << theFullPath.toLatin1().constData();
                }
            }
        }
    }

    // Always return true. Failing to load classes should not block the rest of the system.
    return true;
}

BOOL CUIPParserImpl::LoadGraph(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader,
                               bool initInRenderThread)
{
    IDOMReader::Scope __childScope(inReader);

    m_NumGraphElements = 0;
    m_NumGraphComponents = 0;
    m_NumGraphAttributes = 0;

    bool theLoadResult = inReader.MoveToFirstChild("Graph");
    if (theLoadResult) {
        theLoadResult &= LoadSceneGraph(inPresentation, inReader, nullptr, initInRenderThread);
        if (theLoadResult)
            PatchSceneElementRef();
    }

    return theLoadResult;
}

using qt3ds::runtime::element::SPropertyDesc;
using qt3ds::runtime::element::TPropertyDescAndValue;

void CUIPParserImpl::GetMetaAttribute(IPresentation &inPresentation,
                                      TPropertyDescAndValueList &outDescList, TStrType inType,
                                      TStrType inName, TStrType inClassId,
                                      CRegisteredString *inAttStrNames)
{
    ERuntimeDataModelDataType thePropertyType =
        m_MetaData.GetPropertyType(inType, inName, inClassId);

    // TODO: Handle all other types
    switch (thePropertyType) {
    case ERuntimeDataModelDataTypeFloat:
        AddFloatAttribute(outDescList, inAttStrNames[0],
                          m_MetaData.GetPropertyValueFloat(inType, inName, inClassId));
        break;
    case ERuntimeDataModelDataTypeFloat2: {
        QT3DSVec2 vec2 = m_MetaData.GetPropertyValueVector2(inType, inName, inClassId);
        SFloat2 theValue(vec2.x, vec2.y);
        AddFloat2Attribute(outDescList, inAttStrNames, theValue);
        break;
    }
    case ERuntimeDataModelDataTypeFloat3: {
        QT3DSVec3 vec3 = m_MetaData.GetPropertyValueVector3(inType, inName, inClassId);
        SFloat3 theValue(vec3.x, vec3.y, vec3.z);
        ERuntimeAdditionalMetaDataType theAdditionalType =
            m_MetaData.GetAdditionalType(inType, inName, inClassId);
        AddFloat3Attribute(outDescList, theAdditionalType, inAttStrNames, theValue);
        break;
    }
    case ERuntimeDataModelDataTypeFloat4: {
        QT3DSVec4 vec4 = m_MetaData.GetPropertyValueVector4(inType, inName, inClassId);
        SFloat4 theValue(vec4.x, vec4.y, vec4.z, vec4.w);
        ERuntimeAdditionalMetaDataType theAdditionalType =
            m_MetaData.GetAdditionalType(inType, inName, inClassId);
        AddFloat4Attribute(outDescList, theAdditionalType, inAttStrNames, theValue);
        break;
    }
    case ERuntimeDataModelDataTypeLong:
        AddLongAttribute(outDescList, inAttStrNames[0],
                         m_MetaData.GetPropertyValueLong(inType, inName, inClassId));
        break;
    case ERuntimeDataModelDataTypeString: {
        Option<eastl::string> theRuntimeStr =
            m_MetaData.GetPropertyValueString(inType, inName, inClassId);
        if (theRuntimeStr.hasValue())
            AddStringAttribute(inPresentation, outDescList, inAttStrNames[0],
                               theRuntimeStr->c_str());
        else
            AddStringAttribute(inPresentation, outDescList, inAttStrNames[0], "");
    } break;
    case ERuntimeDataModelDataTypeBool:
        AddBoolAttribute(outDescList, inAttStrNames[0],
                         m_MetaData.GetPropertyValueBool(inType, inName, inClassId));
        break;
    case ERuntimeDataModelDataTypeLong4:
        AddElementRefAttribute(outDescList, inAttStrNames[0], NULL);
        break;
    case ERuntimeDataModelDataTypeObjectRef: {
        // Object ref is converted to string path
        eastl::string theRuntimeStr =
            m_MetaData.GetPropertyValueObjectRef(inType, inName, inClassId);
        eastl::string theObjectRef = m_ObjectRefHelper->BuildReferenceString(theRuntimeStr);
        AddStringAttribute(inPresentation, outDescList, inAttStrNames[0], theObjectRef.c_str());
        break;
    }
    default:
        QT3DS_ASSERT(false);
    }
}

void CUIPParserImpl::AddFloatAttribute(TPropertyDescAndValueList &outDescList,
                                       CRegisteredString inAttStrName, float &inValue)
{
    UVariant theValue;
    theValue.m_FLOAT = inValue;
    outDescList.push_back(
        eastl::make_pair(SPropertyDesc(inAttStrName, ATTRIBUTETYPE_FLOAT), theValue));
}

void CUIPParserImpl::AddLongAttribute(TPropertyDescAndValueList &outDescList,
                                      CRegisteredString inAttStrName, qt3ds::QT3DSI32 &inValue)
{
    UVariant theValue;
    theValue.m_INT32 = inValue;
    outDescList.push_back(
        eastl::make_pair(SPropertyDesc(inAttStrName, ATTRIBUTETYPE_INT32), theValue));
}

void CUIPParserImpl::AddBoolAttribute(TPropertyDescAndValueList &outDescList,
                                      CRegisteredString inAttStrName, bool &inValue)
{
    UVariant theValue;
    theValue.m_INT32 = inValue;
    outDescList.push_back(
        eastl::make_pair(SPropertyDesc(inAttStrName, ATTRIBUTETYPE_BOOL), theValue));
}

void CUIPParserImpl::AddFloat2Attribute(TPropertyDescAndValueList &outDescList,
                                        CRegisteredString *inAttStrNames, SFloat2 &inValue)
{
    for (long theIndex = 0; theIndex < 2; ++theIndex) {
        UVariant theValue;
        theValue.m_FLOAT = inValue[theIndex];
        outDescList.push_back(eastl::make_pair(
            SPropertyDesc(inAttStrNames[theIndex], ATTRIBUTETYPE_FLOAT), theValue));
    }
}

void CUIPParserImpl::AddFloat3Attribute(TPropertyDescAndValueList &outDescList,
                                        ERuntimeAdditionalMetaDataType inAdditionalType,
                                        CRegisteredString *inAttStrNames, SFloat3 &inValue)
{
    for (long theIndex = 0; theIndex < 3; ++theIndex) {
        float theValue = inValue[theIndex];
        if (inAdditionalType == ERuntimeAdditionalMetaDataTypeRotation)
            TORAD(theValue);
        UVariant theVarValue;
        theVarValue.m_FLOAT = theValue;
        outDescList.push_back(eastl::make_pair(
            SPropertyDesc(inAttStrNames[theIndex], ATTRIBUTETYPE_FLOAT), theVarValue));
    }
}

void CUIPParserImpl::AddFloat4Attribute(TPropertyDescAndValueList &outDescList,
                                        ERuntimeAdditionalMetaDataType inAdditionalType,
                                        CRegisteredString *inAttStrNames, SFloat4 &inValue)
{
    for (long i = 0; i < 4; ++i) {
        UVariant varVal;
        varVal.m_FLOAT = inValue[i];
        outDescList.push_back(eastl::make_pair(
            SPropertyDesc(inAttStrNames[i], ATTRIBUTETYPE_FLOAT), varVal));
    }
}


void CUIPParserImpl::AddStringAttribute(IPresentation &inPresentation,
                                        TPropertyDescAndValueList &outDescList,
                                        CRegisteredString inAttStrName, const char *inValue)
{
    qt3ds::foundation::CStringHandle theString = inPresentation.GetStringTable().GetHandle(inValue);
    UVariant theValue;
    theValue.m_StringHandle = theString.handle();
    outDescList.push_back(
        eastl::make_pair(SPropertyDesc(inAttStrName, ATTRIBUTETYPE_STRING), theValue));
    if (CHash::HashAttribute(inAttStrName.c_str()) == Q3DStudio::ATTRIBUTE_SOURCEPATH && inValue
        && *inValue)
        AddSourcePath(inValue, false);
}

void CUIPParserImpl::AddElementRefAttribute(TPropertyDescAndValueList &outDescList,
                                            CRegisteredString inAttStrName, SElement *inElement)
{
    UVariant theValue;
    if (inElement) {
        theValue.m_ElementHandle = inElement->GetHandle();
        QT3DS_ASSERT(theValue.m_ElementHandle);
    } else
        theValue.m_ElementHandle = 0;
    outDescList.push_back(
        eastl::make_pair(SPropertyDesc(inAttStrName, ATTRIBUTETYPE_ELEMENTREF), theValue));
}

void CUIPParserImpl::GetAttributeList(IPresentation &inPresentation,
                                      TPropertyDescAndValueList &outDescList, TStrType inType,
                                      TStrType inName, TStrType inClassId, const char *inValue,
                                      CRegisteredString *inPropNameStrs)
{
    ERuntimeDataModelDataType theDataType = m_MetaData.GetPropertyType(inType, inName, inClassId);
    ERuntimeAdditionalMetaDataType theAdditionalType =
        m_MetaData.GetAdditionalType(inType, inName, inClassId);

    GetAttributeList(inPresentation, outDescList, theDataType, theAdditionalType, inName, inValue,
                     inPropNameStrs);
}

void CUIPParserImpl::GetAttributeList(IPresentation &inPresentation,
                                      TPropertyDescAndValueList &outDescList,
                                      ERuntimeDataModelDataType inDataType,
                                      ERuntimeAdditionalMetaDataType inAdditionalType, const char *,
                                      const char *inValue, CRegisteredString *inPropNameStrs)
{
    // Create a destructible value
    m_ValueBuffer.clear();
    m_TempBuffer.clear();
    m_ValueBuffer.write(inValue, (QT3DSU32)strlen(inValue) + 1);
    WCharTReader theReader((char8_t *)m_ValueBuffer.begin(), m_TempBuffer,
                           *m_DOMReader->GetStringTable());

    // TODO: Handle all other types
    // Note that the default value should be consistent with the SetDefault method of
    // TDataModelValue
    switch (inDataType) {
    case ERuntimeDataModelDataTypeLong: {
        qt3ds::QT3DSI32 theValue = 0;
        if (!IsTrivial(inValue))
            theReader.Read(theValue);
        AddLongAttribute(outDescList, inPropNameStrs[0], theValue);
        break;
    }
    case ERuntimeDataModelDataTypeFloat: {
        float theValue = 0;
        if (!IsTrivial(inValue))
            theReader.Read(theValue);
        AddFloatAttribute(outDescList, inPropNameStrs[0], theValue);
        break;
    }
    case ERuntimeDataModelDataTypeFloat2: {
        SFloat2 theValue(0);
        if (!IsTrivial(inValue))
            theReader.ReadRef(NVDataRef<QT3DSF32>(&theValue[0], 2));
        AddFloat2Attribute(outDescList, inPropNameStrs, theValue);
        break;
    }
    case ERuntimeDataModelDataTypeFloat3: {
        SFloat3 theValue(0);
        if (!IsTrivial(inValue))
            theReader.ReadRef(NVDataRef<QT3DSF32>(&theValue[0], 3));
        AddFloat3Attribute(outDescList, inAdditionalType, inPropNameStrs, theValue);
        break;
    }
    case ERuntimeDataModelDataTypeFloat4: {
        SFloat4 theValue(0);
        if (!IsTrivial(inValue))
            theReader.ReadRef(NVDataRef<QT3DSF32>(&theValue[0], 4));
        AddFloat4Attribute(outDescList, inAdditionalType, inPropNameStrs, theValue);
        break;
    }
    case ERuntimeDataModelDataTypeBool: {
        bool theValue = false;
        if (!IsTrivial(inValue))
            theReader.Read(theValue);
        AddBoolAttribute(outDescList, inPropNameStrs[0], theValue);
        break;
    }
    case ERuntimeDataModelDataTypeStringRef:
    case ERuntimeDataModelDataTypeString: {
        const char *theDataPtr = "";
        if (!IsTrivial(inValue))
            theReader.Read(theDataPtr);
        AddStringAttribute(inPresentation, outDescList, inPropNameStrs[0], theDataPtr);
        break;
    }
    case ERuntimeDataModelDataTypeLong4: {
        const char *theDataPtr = NULL;
        if (!IsTrivial(inValue))
            theReader.Read(theDataPtr);
        SElement *theElementData = GetElement(theDataPtr);
        AddElementRefAttribute(outDescList, inPropNameStrs[0], theElementData);
        break;
    }
    case ERuntimeDataModelDataTypeObjectRef: {
        // Object ref is converted to string path
        const char *theDataPtr = NULL;
        if (!IsTrivial(inValue))
            theReader.Read(theDataPtr);
        if (theDataPtr) {
            eastl::string theObjectRef = m_ObjectRefHelper->BuildReferenceString(theDataPtr);
            AddStringAttribute(inPresentation, outDescList, inPropNameStrs[0],
                               theObjectRef.c_str());
        }
        break;
    }
    default:
        QT3DS_ASSERT(false);
    }
}

//==============================================================================
/**
 *	Looks up the EElementType from the asset type.
 */
EElementType GetElementType(const char *inType)
{
    if (AreEqual(inType, "Scene") || AreEqual(inType, "Layer") || AreEqual(inType, "Group")
        || AreEqual(inType, "Model") || AreEqual(inType, "Path"))
        return ELEMENTTYPE_NODE;

    else if (AreEqual(inType, "Camera"))
        return ELEMENTTYPE_CAMERA;

    else if (AreEqual(inType, "Light"))
        return ELEMENTTYPE_LIGHT;

    else if (AreEqual(inType, "Component"))
        return ELEMENTTYPE_COMPONENT;

    else if (AreEqual(inType, "Material"))
        return ELEMENTTYPE_MATERIAL;

    else if (AreEqual(inType, "Image"))
        return ELEMENTTYPE_TEXTURE;

    else if (AreEqual(inType, "Behavior"))
        return ELEMENTTYPE_BEHAVIOR;

    else if (AreEqual(inType, "Text"))
        return ELEMENTTYPE_TEXT;

    else if (AreEqual(inType, "PathAnchorPoint"))
        return ELEMENTTYPE_PATHANCHORPOINT;

    else if (AreEqual(inType, "SubPath"))
        return ELEMENTTYPE_SUBPATH;

    else
        return ELEMENTTYPE_UNKNOWN;
}

//==============================================================================
/**
 *	Build out the graph.
 *	@param	inPresentation		presentation written to
 *	@return a flag indicating whether or not we successfully loaded the file
 */
BOOL CUIPParserImpl::LoadSceneGraph(IPresentation &inPresentation, IDOMReader &inReader,
                                    qt3ds::runtime::element::SElement *inNewStyleParent,
                                    bool initInRenderThread, bool isInsideLayer)
{
    IDOMReader::Scope __childScope(inReader);
    IScriptBridge *theScriptBridgeQml = inPresentation.GetScriptBridgeQml();

    eastl::string theFileString;
    qt3ds::runtime::IApplication &theApp(inPresentation.GetApplication());
    qt3ds::runtime::IElementAllocator &theElemAllocator(theApp.GetElementAllocator());
    TPropertyDescAndValueList theProperties;
    using qt3ds::runtime::element::SPropertyDesc;
    qt3ds::foundation::IStringTable &theStringTable(inPresentation.GetStringTable());
    // build out the graph.
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        theProperties.clear();
        const char *theId;
        inReader.Att("id", theId);
        const char *theClass;
        inReader.Att("class", theClass);
        const char *theType(inReader.GetNarrowElementName());
        eastl::string theName(m_ObjectRefHelper->GetName(theId).c_str());
        // Get default end time from metadata
        UINT32 theLoopTime = 0;
        bool isComponent = false;
        bool isBehavior = false;
        bool isChildInsideLayer = isInsideLayer;

        // Create SElement
        if (AreEqual(theType, "Scene") || AreEqual(theType, "Component")) {
            // TDataModelValue theValue;
            theLoopTime = m_MetaData.GetPropertyValueLong(m_MetaData.Register(theType),
                                                          m_MetaData.Register("endtime"));
            Q3DStudio_ASSERT(theLoopTime != 0); // Something is wrong here
            isComponent = true;
        } else {
            if (AreEqual(theType, "Layer")) {
                isChildInsideLayer = true;
            } else if ((AreEqual(theType, "Material") || AreEqual(theType, "CustomMaterial")
                        || AreEqual(theType, "Image")) && !isInsideLayer) {
                // Identify objects outside a Layer object with types of Material, CustomMaterial
                // or Image to be inside the material container
                m_materialContainerIds.append(QString::fromUtf8(theId));
            } else if (AreEqual(theType, "Behavior") && !IsTrivial(theClass)) {
                // Find the sourcepath and load the script
                ++theClass; // remove the '#'
                TIdSourcePathMap::iterator theSourcePathIter = m_IdScriptMap.find(theClass);
                if (theSourcePathIter != m_IdScriptMap.end()) {
                    theFileString.assign(theSourcePathIter->second);

// For non-windows platforms, we need to reverse the slashes
#ifndef EA_PLATFORM_WINDOWS
                    for (eastl::string::size_type thePos = theFileString.find('\\');
                         thePos != eastl::string::npos;
                         thePos = theFileString.find('\\', thePos + 1)) {
                        theFileString.replace(thePos, 1, 1, '/');
                    }
#endif
                    isBehavior = true;
                    UVariant theValue;
                    theValue.m_StringHandle =
                        inPresentation.GetStringTable().GetHandle(theFileString.c_str());
                    theProperties.push_back(eastl::make_pair(
                        SPropertyDesc(theStringTable.RegisterStr("behaviorscripts"),
                                      ATTRIBUTETYPE_STRING),
                        theValue));
                } else {
                    qCCritical(qt3ds::INVALID_OPERATION)
                            << "Could not load Behavior script: " << theClass;
                }
            }
        }

        SElementData &theElementData =
            m_ParseElementManager.GetOrCreateElementData(theId, theType, theClass);

        const char8_t *theSourcePath;
        if (inReader.UnregisteredAtt("sourcepath", theSourcePath)) {
            const char8_t *mapping;
            bool ibl = false;
            if (inReader.UnregisteredAtt("mappingmode", mapping)) {
                if (QString::fromUtf8(mapping) == QLatin1String("Light Probe"))
                    ibl = true;
            }
            AddSourcePath(theSourcePath, ibl);
        }

        TAttrMap &theList = theElementData.m_PropertyMap;

        const char *theAttribute;
        for (TAttrMap::iterator theIter = theList.begin(), theMapEnd = theList.end();
             theIter != theMapEnd; ++theIter) {
            if (theIter->second.m_ElementFlag) {
                // Parse the property value from uip, this will overwrite the default value
                const TStrType &thePropertyName(theIter->first);
                CRegisteredString *thePropertyNames = theIter->second.m_PropertyNames;
                size_t theListSize = theProperties.size();
                if (inReader.Att(thePropertyName.c_str(), theAttribute)) {
                    ERuntimeDataModelDataType theDataType = m_MetaData.GetPropertyType(
                        theElementData.m_Type, thePropertyName, theElementData.m_Class);
                    if (theDataType == ERuntimeDataModelDataTypeLong4) {
                        AddElementRefToPatch(theProperties, theElementData, thePropertyName.c_str(),
                                             theAttribute);
                    } else
                        GetAttributeList(inPresentation, theProperties, theElementData.m_Type,
                                         thePropertyName, theElementData.m_Class, theAttribute,
                                         thePropertyNames);
                } else {
                    GetMetaAttribute(inPresentation, theProperties, theElementData.m_Type,
                                     thePropertyName, theElementData.m_Class, thePropertyNames);
                }
                size_t theNumAtts = theProperties.size() - theListSize;
                QT3DS_ASSERT(theNumAtts == (size_t)theIter->second.m_Arity);
                (void)theNumAtts;
            }
        }
        qt3ds::runtime::element::SElement &theNewElem = theElemAllocator.CreateElement(
            theStringTable.RegisterStr(theName.c_str()), theStringTable.RegisterStr(theType),
            theStringTable.RegisterStr(theElementData.m_Class.c_str()),
            toConstDataRef(theProperties.data(), (QT3DSU32)theProperties.size()), &inPresentation,
            inNewStyleParent, isComponent);
        theElementData.m_Element = &theNewElem;

        if (inPresentation.GetRoot() == NULL)
            inPresentation.SetRoot(theNewElem);

        eastl::string thePath;
        // build out the full path to the element for debugging
        for (CUIPParserObjectRefHelper::SGraphNode *theNode =
                 this->m_ObjectRefHelper->GetNode(theId);
             theNode; theNode = theNode->m_Parent) {
            if (thePath.length())
                thePath.insert(0, ".");
            thePath.insert(0, theNode->m_Name);
        }
        if (thePath.size())
            theNewElem.setPath(m_ParseElementManager.m_StringTable.RegisterStr(thePath.c_str()));

        if (isBehavior) {
            if (theFileString.find(".qml") != eastl::string::npos) {
                theScriptBridgeQml->LoadScript(&inPresentation, &theNewElem,
                                               theFileString.c_str(), initInRenderThread);
            }
        }
        LoadSceneGraph(inPresentation, inReader, &theNewElem, initInRenderThread,
                       isChildInsideLayer);
    }

    return true;
}

//==============================================================================
/**
 *	This method will parse through the graph sections to cache those attributes that
 *	should never be optimized
 */
void CUIPParserImpl::CacheGraphRequiredAttributes(qt3dsdm::IDOMReader &inReader)
{
    IDOMReader::Scope __childScope(inReader);

    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        const char *theId;
        inReader.Att("id", theId);
        const char *theType(inReader.GetNarrowElementName());
        const char *theClass = "";
        inReader.Att("class", theClass);
        SElementData &theData =
            m_ParseElementManager.GetOrCreateElementData(theId, theType, theClass);
        // name and sourcepath are never optimized out.
        m_ParseElementManager.MarkAttributeAsReferenced(theData, "name");
        // not everything needs to be in the time graph.
        if (AreEqual(theType, "Scene") == false && AreEqual(theType, "Material") == false
            && AreEqual(theType, "CustomMaterial") == false && AreEqual(theType, "Image") == false
            && AreEqual(theType, "RenderPlugin") == false) {
            SElementPropertyInfo *theProp =
                m_ParseElementManager.GetOrCreateProperty(theData, "starttime");
            theProp->m_SlideFlag = true;
            theProp->m_SlideForceFlag = true;
            theProp->m_ElementFlag = true;
            theProp = m_ParseElementManager.GetOrCreateProperty(theData, "endtime");
            theProp->m_SlideFlag = true;
            theProp->m_SlideForceFlag = true;
            theProp->m_ElementFlag = true;
        }

        // Probably not the most optimal thing to expose this attribute for all elements that
        // support it, since it is not really needed except during initialization
        m_ParseElementManager.MarkAttributeAsReferenced(theData, "controlledproperty");
        m_ParseElementManager.MarkAttributeAsReferenced(theData, "observedproperty");

        // Behaviors need all attributes possible on the object on them all the time.
        if (AreEqual(theType, "Behavior") || AreEqual(theType, "RenderPlugin")) {
            m_ParseElementManager.MarkAllAttributesAsReferenced(theData);
        }

        // Recursive
        CacheGraphRequiredAttributes(inReader);
    }
}

//==============================================================================
/**
 *	This method will parse through the logic sections to cache those attributes that
 *	will be changed and thus needs to be at the SElement
 */
BOOL CUIPParserImpl::CacheLogicRequiredAttributes(qt3dsdm::IDOMReader &inReader,
                                                  qt3ds::QT3DSI32 inSlideIndex,
                                                  SParseSlide *inParentSlide)
{
    IDOMReader::Scope __stateScope(inReader);
    eastl::string theSlideId = GetSlideId(inReader);
    SParseSlide &theSlide = m_ParseSlideManager.GetOrCreateSlide(theSlideId.c_str(), inSlideIndex);
    eastl::hash_set<TStrType> theCommandInstances;
    QT3DSU32 slideAnimActions = 0;

    {
        IDOMReader::Scope __commandLoopScope(inReader);
        for (BOOL theSuccess = inReader.MoveToFirstChild(); theSuccess;
             theSuccess = inReader.MoveToNextSibling()) {
            IDOMReader::Scope __commandScope(inReader);
            if (AreEqual(inReader.GetNarrowElementName(), "Add")
                || AreEqual(inReader.GetNarrowElementName(), "Set")) {
                bool isSet = AreEqual(inReader.GetNarrowElementName(), "Set");

                const char *theRef;
                if (inReader.Att("ref", theRef)) {
                    theRef++; // remove the '#'

                    // Set/Add command applied to an element *not* in the scene graph.
                    SElementData *theData = m_ParseElementManager.FindElementData(theRef);
                    if (theData == NULL) {
                        QT3DS_ASSERT(false);
                        continue;
                    }
                    // If this element has controlledproperty, need to mark all as referenced
                    // in order for datainput initialisation to find attributes that
                    // are controlled
                    // TODO: we could parse out the exact controlled property names and
                    // set only those as referenced. In this case we might have to expand vec3
                    // type attributes to component parts
                    const char *ctrldProp;
                    const char *observedProp;
                    inReader.Att("controlledproperty", ctrldProp);
                    inReader.Att("observedproperty", observedProp);
                    if (ctrldProp || observedProp)
                        m_ParseElementManager.MarkAllAttributesAsReferenced(*theData, true);

                    theCommandInstances.insert(theData->m_Id);
                    if (isSet) {
                        for (eastl::pair<const char *, const char *> theAtt =
                                 inReader.GetNarrowFirstAttribute();
                             !IsTrivial(theAtt.first); theAtt = inReader.GetNarrowNextAttribute()) {
                            if (AreEqual(theAtt.first, "ref") || AreEqual(theAtt.first, "shy"))
                                continue;

                            SElementPropertyInfo *theProperty =
                                m_ParseElementManager.GetOrCreateProperty(*theData, theAtt.first);
                            // This property exists on both the elements and the slides.
                            if (theProperty) {
                                theProperty->m_ElementFlag = true;
                                theProperty->m_SlideFlag = true;
                            }
                        }
                        // Run through parent animations and forward them *if* this set command does
                        // not have this property present.
                        if (inParentSlide) {
                            TAnimationList *theAnimations =
                                m_ParseSlideManager.GetAnimationsForInstance(*inParentSlide,
                                                                             theData->m_Id);
                            if (theAnimations != NULL) {
                                for (QT3DSU32 animIdx = 0, animEnd = theAnimations->size();
                                     animIdx < animEnd; ++animIdx) {
                                    SParseAnimationRef &theEntry((*theAnimations)[animIdx]);
                                    const char8_t *propValue = "";
                                    SElementPropertyInfo *thePropInfo =
                                        m_ParseElementManager.FindProperty(
                                            *theData, theEntry.m_PropertyName.c_str());
                                    if (!thePropInfo) {
                                        QT3DS_ASSERT(false);
                                        continue;
                                    }
                                    // If the attribute doesn't exist in the command, then we
                                    // forward the animation to here
                                    if (inReader.Att(thePropInfo->m_PropertyName, propValue)
                                        == false) {
                                        ++slideAnimActions;
                                        m_ParseSlideManager.ReferenceAnimation(
                                            theSlide, *theEntry.m_Animation, true);
                                    }
                                }
                            }
                        }
                    }

                    // find all the animations, regardless of add or set.
                    {
                        IDOMReader::Scope __animationScope(inReader);
                        for (BOOL theAnimScopeSuccess = inReader.MoveToFirstChild("AnimationTrack");
                             theAnimScopeSuccess;
                             theAnimScopeSuccess = inReader.MoveToNextSibling("AnimationTrack")) {
                            const char *theProperty;
                            const char8_t *theValue = "";
                            inReader.Value(theValue);
                            if (theValue != NULL && *theValue
                                && inReader.UnregisteredAtt("property", theProperty)) {
                                m_ParseElementManager.MarkAttributeAsReferenced(*theData,
                                                                                theProperty);
                                // PreParse the animation at this point.  This allows us to easily
                                // count animations.
                                bool isDynamic = false;
                                inReader.Att("dynamic", isDynamic);
                                SParseSlideAnimationTypes::Enum theParseType =
                                    SParseSlideAnimationTypes::EaseInOut;
                                const char8_t *animType;
                                if (inReader.Att("type", animType)) {
                                    if (AreEqual(animType, "EaseInOut")) {
                                    } else if (AreEqual(animType, "Bezier"))
                                        theParseType = SParseSlideAnimationTypes::Bezier;
                                    else if (AreEqual(animType, "Linear"))
                                        theParseType = SParseSlideAnimationTypes::Linear;
                                    else {
                                        QT3DS_ASSERT(false);
                                    }
                                }
                                ++slideAnimActions;
                                m_ParseSlideManager.SetAnimationData(
                                    theSlide, theData->m_Id, theProperty, theValue, theParseType,
                                    isDynamic, inSlideIndex != 0);
                            }
                        }
                    }

                    // find all the actions
                    {
                        IDOMReader::Scope __actionScope(inReader);
                        for (BOOL theActionScopeSuccess = inReader.MoveToFirstChild("Action");
                             theActionScopeSuccess;
                             theActionScopeSuccess = inReader.MoveToNextSibling("Action")) {
                            const char8_t *theActionId;
                            // Don't cache reference actions
                            if (inReader.Att("id", theActionId)) {
                                m_ActionHelper->CacheAction(inReader, theData->m_Id);
                                bool active = true;
                                inReader.Att("eyeball", active);
                                ++slideAnimActions;
                                m_ParseSlideManager.GetOrCreateAction(
                                    theSlide, theActionId,
                                    m_ActionHelper->GetActionCount(theActionId), active);
                            }
                        }
                    }
                }
            }
        }
    }
    // Now forward all animations from the parent slide that are for instances that we haven't
    // covered at all on this slide.
    if (inParentSlide) {
        for (TInstanceIdAnimationMap::iterator theIter = inParentSlide->m_Animations.begin(),
                                               theEnd = inParentSlide->m_Animations.end();
             theIter != theEnd; ++theIter) {
            if (theCommandInstances.find(theIter->first) == theCommandInstances.end()) {
                TAnimationList &theList(theIter->second);
                for (QT3DSU32 animIdx = 0, animEnd = theList.size(); animIdx < animEnd; ++animIdx) {
                    SParseAnimationRef &theEntry(theList[animIdx]);
                    m_ParseSlideManager.ReferenceAnimation(theSlide, *theEntry.m_Animation, true);
                    ++slideAnimActions;
                }
            }
        }
        // Forward action creation onto this slide.
        for (TActionList::iterator theIter = inParentSlide->m_ActionList.begin(),
                                   theEnd = inParentSlide->m_ActionList.end();
             theIter != theEnd; ++theIter) {
            SParseSlideActionEntry &theAction(*theIter);
            m_ParseSlideManager.GetOrCreateAction(theSlide, theAction.m_ActionId,
                                                  theAction.m_ActionCount, theAction.m_Active);
            ++slideAnimActions;
        }
    }

    eastl::vector<eastl::string> theChildSlideIds;

    for (BOOL theSuccess = inReader.MoveToFirstChild("State"); theSuccess;
         theSuccess = inReader.MoveToNextSibling("State")) {
        IDOMReader::Scope __subSlideScope(inReader);
        ++inSlideIndex;
        theChildSlideIds.push_back(GetSlideId(inReader));
        CacheLogicRequiredAttributes(inReader, inSlideIndex, &theSlide);
    }

    // Pull all child animations up into this slide, but set them to deactivated.
    for (QT3DSU32 childSlideIdx = 0, childSlideEnd = theChildSlideIds.size();
         childSlideIdx < childSlideEnd; ++childSlideIdx) {
        SParseSlide *theChildSlide =
            m_ParseSlideManager.FindSlide(theChildSlideIds[childSlideIdx].c_str());
        if (theChildSlide) {
            // Add animations that did not original in this slide to this slide.
            for (TInstanceIdAnimationMap::iterator theIter = theChildSlide->m_Animations.begin(),
                                                   theEnd = theChildSlide->m_Animations.end();
                 theIter != theEnd; ++theIter) {
                TAnimationList &theList(theIter->second);
                for (QT3DSU32 animIdx = 0, animEnd = theList.size(); animIdx < animEnd; ++animIdx) {
                    SParseAnimationRef &theEntry(theList[animIdx]);
                    if (theEntry.m_Animation
                        && theEntry.m_Animation->m_SlideId != theSlide.m_SlideId) {
                        m_ParseSlideManager.ReferenceAnimation(theSlide, *theEntry.m_Animation,
                                                               false);
                        ++slideAnimActions;
                    }
                }
            }
            for (TActionList::iterator theIter = theChildSlide->m_ActionList.begin(),
                                       theEnd = theChildSlide->m_ActionList.end();
                 theIter != theEnd; ++theIter) {
                SParseSlideActionEntry &theEntry(*theIter);
                // This deactivates actions that may be active on another slide.
                m_ParseSlideManager.GetOrCreateAction(theSlide, theEntry.m_ActionId,
                                                      theEntry.m_ActionCount, false);
                ++slideAnimActions;
            }
        }
    }
    // qt3ds::NVFoundationBase& fnd(
    // m_CurrentPresentation->GetApplication().GetRuntimeFactory().GetFoundation() );
    // fnd.error( QT3DS_TRACE, "Loading slide %s, %d anim actions", theSlideId.c_str(),
    // slideAnimActions );

    return TRUE;
}

void CUIPParserImpl::CacheClassRequiredAttributes(qt3dsdm::IDOMReader &inReader)
{
    {
        // First, we parse Graph section to cache custom attributes specified by the class
        IDOMReader::Scope __graphScope(inReader);
        inReader.MoveToFirstChild("Graph");
        CacheClassSceneAttributes(inReader);
    }
    if (!m_IdClassMap.empty()) {
        // Next, we parse Logic section to update the references
        IDOMReader::Scope __logicScope(inReader);
        inReader.MoveToFirstChild("Logic");
        CacheClassStateAttributes(inReader);
    }
}

void CUIPParserImpl::CacheClassSceneAttributes(IDOMReader &inReader)
{
    IDOMReader::Scope __childScope(inReader);
    // build out the graph.
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        const char *theId;
        const char *theClass;
        if (inReader.Att("class", theClass) && inReader.Att("id", theId)) {
            // Add to id-class map
            m_IdClassMap[theId] = theClass;
        }
        CacheClassSceneAttributes(inReader);
    }
}

void CUIPParserImpl::CacheClassStateAttributes(IDOMReader &inReader)
{
    IDOMReader::Scope __childScope(inReader);

    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        if (AreEqual(inReader.GetNarrowElementName(), "State"))
            CacheClassStateAttributes(inReader);
        else if (AreEqual(inReader.GetNarrowElementName(), "Add")
                 || AreEqual(inReader.GetNarrowElementName(), "Set")) {
            const char *theRef;
            if (inReader.Att("ref", theRef)) {
                theRef++; // remove the '#'

                // Find the class, if any
                TIdClassMap::iterator theIter = m_IdClassMap.find(theRef);
                if (theIter != m_IdClassMap.end()) {
                    // Get References of the class
                    eastl::vector<eastl::string> theReferences;
                    m_MetaData.GetReferences("", theReferences, theIter->second.c_str());
                    if (!theReferences.empty()) {
                        // Cache the references
                        m_ObjectRefHelper->MarkAllReferencedAttributes(
                            theRef, theReferences, inReader, m_ParseElementManager);
                    }
                }
            }
        }
    }
}

//==============================================================================
/**
 *	For ElementRef type, there could be times where the SElement isn't created yet, thus
 *	this method cache those Long4 type and add an empty attribute into the SElement
 */
void CUIPParserImpl::AddElementRefToPatch(TPropertyDescAndValueList &outDescList,
                                          SElementData &inElement, const char *inName,
                                          const char *inValue)
{
    SElementRefCache theElementRefData;
    theElementRefData.m_Element = &inElement;
    theElementRefData.m_Name = inName;
    theElementRefData.m_Value = inValue;
    m_ElementRefCache.push_back(theElementRefData);

    UVariant theValue;
    theValue.m_ElementHandle = 0;
    outDescList.push_back(
        eastl::make_pair(SPropertyDesc(m_ParseElementManager.m_StringTable.RegisterStr(inName),
                                       ATTRIBUTETYPE_ELEMENTREF),
                         theValue));
}

//==============================================================================
/**
 *	For those Long4 type that was not loaded correctly, go through all of them and
 *	fix up the ElementRef
 */
void CUIPParserImpl::PatchSceneElementRef()
{
    for (eastl::vector<SElementRefCache>::iterator theIter = m_ElementRefCache.begin();
         theIter != m_ElementRefCache.end(); ++theIter) {
        SElementRefCache &theCache = *theIter;

        SElementData *theElementData = GetElementData(theCache.m_Value.c_str());

        SElement *theElement = theCache.m_Element->m_Element;
        UVariant *theValue = theElement->FindPropertyValue(
            m_ParseElementManager.RegisterStr(theCache.m_Value.c_str()));
        if (theValue) {
            theValue->m_ElementHandle = 0;
            if (theElementData->m_Element) {
                theValue->m_ElementHandle = theElementData->m_Element->GetHandle();
                QT3DS_ASSERT(theValue->m_ElementHandle);
            }
        }
    }
}

BOOL CUIPParserImpl::LoadLogic(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader)
{
    IDOMReader::Scope __logicScope(inReader);

    m_NumSlides = 0;
    m_NumSlideElements = 0;
    m_NumSlideAttributes = 0;
    m_NumAnimationTracks = 0;
    m_NumAnimationKeys = 0;
    m_NumSlideAnimations = 0;
    m_NumSlideActions = 0;

    bool theStatus = true;

    // Build all of the animation objects that we have tallied up.
    for (QT3DSU32 animIdx = 0, animEnd = m_ParseSlideManager.m_Animations.size(); animIdx < animEnd;
         ++animIdx) {
        SParseSlideAnimationEntry &theEntry = *m_ParseSlideManager.m_Animations[animIdx];
        SElementData *theData = m_ParseElementManager.FindElementData(theEntry.m_InstanceId);
        // Generate valid animation indexes while we are at it.
        LoadAnimationTrack(inPresentation, *theData, theEntry);
    }

    if (inReader.MoveToFirstChild("Logic")) {

        // Action
        m_ActionHelper->BuildActions(inPresentation);

        for (BOOL theSuccess = inReader.MoveToFirstChild("State"); theSuccess;
             theSuccess = inReader.MoveToNextSibling("State")) {
            theStatus &= LoadStateGraph(inPresentation, inReader);
        }
    }

    return theStatus;
}

//==============================================================================
/**
 *	Load the State (Slides)
 *	@param	inPresentation		presentation written to
 *	@return a flag indicating whether or not we successfully loaded the file
 */
BOOL CUIPParserImpl::LoadStateGraph(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader)
{
    IDOMReader::Scope __slideScope(inReader);

    BOOL theStatus = true;

    SElement *theComponent = NULL;
    const char8_t *theAttribute;
    if (inReader.UnregisteredAtt("component", theAttribute))
        theComponent = GetElement(theAttribute);
    if (theComponent) {
        INT32 theSlideIndex = 0;
        LoadState(inPresentation, theComponent, theSlideIndex, inReader);

        for (BOOL theResult = inReader.MoveToFirstChild("State"); theResult;
             theResult = inReader.MoveToNextSibling("State")) {
            if (theStatus)
                theStatus &= LoadState(inPresentation, theComponent, ++theSlideIndex, inReader);
        }
    } else {
        if (theAttribute) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "Slide load failure!! Failed to find component: " << theAttribute;
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "Slide load failure!! Failed to find component.";
        }
    }

    // Add source paths of the source materials of the ReferencedMaterials
    // to the slide source paths
    for (const auto &slideRefMats : qAsConst(m_referencedMaterialsBySlide)) {
        const auto &slideKey = slideRefMats.first;
        const auto &refMats = slideRefMats.second;

        QVector<QString> sourcePathRefs;
        for (const auto &refMat : refMats) {
            // CustomMaterials contains their own source paths
            sourcePathRefs.append(refMat);
            // Materials have their source paths inside Image objects
            if (m_imagesByMaterial.contains(refMat))
                sourcePathRefs.append(m_imagesByMaterial[refMat]);
        }

        for (const auto &sourcePathRef : qAsConst(sourcePathRefs)) {
            if (m_sourcePathsById.contains(sourcePathRef)) {
                const auto &sourcePaths = m_sourcePathsById[sourcePathRef];
                for (const auto &sourcePath : sourcePaths) {
                    inPresentation.GetSlideSystem().AddSourcePath(slideKey,
                                                                  sourcePath.toUtf8().constData());
                }
            }
        }
    }

    return theStatus;
}

//==============================================================================
/**
 *	Load the State (Slides)
 *	@param	inPresentation		presentation written to
 *	@return a flag indicating whether or not we successfully loaded the file
 */
BOOL CUIPParserImpl::LoadState(IPresentation &inPresentation, SElement *inComponent,
                               INT32 inSlideIndex, qt3dsdm::IDOMReader &inReader)
{
    IDOMReader::Scope __stateScope(inReader);

    BOOL theResult = true;
    ISlideSystem &theBuilder = inPresentation.GetSlideSystem();
    m_currentSlideReferencedMaterials.clear();

    eastl::string theSlideName = GetSlideName(inReader);

    qt3ds::runtime::SSlidePlayInformation thePlayMode = GetPlayMode(inReader);
    thePlayMode.m_PlayThroughTo = (QT3DSU8)GetPlayThroughTo(inSlideIndex, inReader);
    // Setup with the default value from the metadata
    INT32 theMinTime = m_MetaData.GetPropertyValueLong(m_MetaData.Register("Asset"),
                                                       m_MetaData.Register("starttime"));
    INT32 theMaxTime = m_MetaData.GetPropertyValueLong(m_MetaData.Register("Asset"),
                                                       m_MetaData.Register("endtime"));

    theBuilder.AddSlide(*inComponent, theSlideName.c_str(), thePlayMode.m_PlayMode,
                        thePlayMode.m_Paused, thePlayMode.m_PlayThroughTo, theMinTime, theMaxTime);
    m_NumSlides++;

    m_SlideElements.clear();
    m_SlideElements.insert(inComponent);

    BOOL theActiveFlag = (inSlideIndex != 0);
    theBuilder.AddSlideElement(*inComponent, theActiveFlag);
    ProcessStateRef(inPresentation, inComponent, inSlideIndex == 0, inReader);

    m_NumSlideElements++;

    theMaxTime = 0;
    theResult &=
        LoadSlideElements(inPresentation, inReader, inSlideIndex == 0, inComponent, &theMaxTime);
    if (theMaxTime != 0)
        theBuilder.SetSlideMaxTime((QT3DSU32)theMaxTime);

    if (!m_currentSlideReferencedMaterials.empty()) {
        qt3ds::runtime::SSlideKey key;
        key.m_Component = inComponent;
        key.m_Index = inSlideIndex;
        QPair<qt3ds::runtime::SSlideKey, QVector<QString>> pair;
        pair.first = key;
        pair.second = m_currentSlideReferencedMaterials;
        m_referencedMaterialsBySlide.append(pair);
    }

    return theResult;
}

//==============================================================================
/**
 *	Process the state information to add all the animations and actions that might be setup
 */
BOOL CUIPParserImpl::ProcessStateRef(IPresentation &inPresentation, SElement *inComponent,
                                     bool inMaster, qt3dsdm::IDOMReader &inReader)
{
    IDOMReader::Scope __stateScope(inReader);
    eastl::string theSlideId = GetSlideId(inReader);
    for (BOOL theResult = inReader.MoveToFirstChild("Set"); theResult;
         theResult = inReader.MoveToNextSibling("Set")) {
        const char8_t *theElementRef;
        if (inReader.UnregisteredAtt("ref", theElementRef)
            && GetElement(theElementRef) == inComponent)
            ProcessSlideAnimAction(inPresentation, theSlideId, inMaster, inReader);
    }
    return TRUE;
}

void CUIPParserImpl::ComputeAndReserveMemory(IPresentation & /*inPresentation*/,
                                             qt3dsdm::IDOMReader &inReader)
{
    SGraphSectionCount theGraphSectionCount;
    SLogicSectionCount theLogicSectionCount;
    SActionSectionCount theActionSectionCount;

    {
        IDOMReader::Scope __graphScope(inReader);
        if (inReader.MoveToFirstChild("Graph"))
            DoGraphSectionCount(inReader, theGraphSectionCount);
    }
    DoLogicSectionCount(inReader, theLogicSectionCount);
    m_ActionHelper->GetActionSectionCount(theActionSectionCount);
}

void CUIPParserImpl::DoGraphSectionCount(qt3dsdm::IDOMReader &inReader,
                                         SGraphSectionCount &outGraphCounter)
{
    IDOMReader::Scope __graphScope(inReader);

    for (bool theResult = inReader.MoveToFirstChild(); theResult;
         theResult = inReader.MoveToNextSibling()) {
        const char *theTypeStr(inReader.GetNarrowElementName());
        if (AreEqual(theTypeStr, "Scene") || AreEqual(theTypeStr, "Component"))
            outGraphCounter.m_ComponentCount++;
        else
            outGraphCounter.m_ElementCount++;

        const char *theIdStr;
        inReader.Att("id", theIdStr);
        const char *theClassStr;
        inReader.Att("class", theClassStr);

        // For Behavior, we are adding a string attribute ATTRIBUTE_BEHAVIORSCRIPTS
        if (AreEqual(theTypeStr, "Behavior") && !IsTrivial(theClassStr)) {
            ++theClassStr; // remove the '#'
            TIdSourcePathMap::iterator theSourcePathIter = m_IdScriptMap.find(theClassStr);
            if (theSourcePathIter != m_IdScriptMap.end()) {
                outGraphCounter.m_AttributeCount++;
                outGraphCounter.m_StringAttrCount++;
            }
        }
        SElementData *theData = m_ParseElementManager.FindElementData(theIdStr);

        if (theData) {
            TAttrMap &thePropertyMap = theData->m_PropertyMap;

            // TODO: Maybe we should make m_IdAttributesMap only stores those valid attributes and
            // also those attributes that already converted to Runtime type (e.g. position to
            // position.x .y .z )
            // then we can really stop calling all the IsValidAttribute all over the place
            // and we can simple just use the size of the vector here.
            for (TAttrMap::iterator theIter = thePropertyMap.begin(), theEnd = thePropertyMap.end();
                 theIter != theEnd; ++theIter) {
                if (theIter->second.m_ElementFlag == false)
                    continue;
                TStrType thePropertyName(theIter->first);
                outGraphCounter.m_AttributeCount += theIter->second.m_Arity;
                if (IsStringType(theIter->second.m_DataType))
                    outGraphCounter.m_StringAttrCount += 1;
            }
        }

        DoGraphSectionCount(inReader, outGraphCounter);
    }
}

void CUIPParserImpl::DoLogicSectionCount(qt3dsdm::IDOMReader &inReader,
                                         SLogicSectionCount &outLogicCounter)
{
    IDOMReader::Scope __logicCountScope(inReader);

    if (inReader.MoveToFirstChild("Logic")) {
        for (bool theResult = inReader.MoveToFirstChild("State"); theResult;
             theResult = inReader.MoveToNextSibling("State")) {
            DoStateSectionCount(inReader, 0, outLogicCounter);
        }
    }
}

void CUIPParserImpl::DoStateSectionCount(qt3dsdm::IDOMReader &inReader, Q3DStudio::INT32 inStateIndex,
                                         SLogicSectionCount &outLogicCounter)
{
    IDOMReader::Scope __slideCountScope(inReader);

    outLogicCounter.m_SlideCount++;

    INT32 theChildSlideCount = inReader.CountChildren("State");
    outLogicCounter.m_SlideElementCount++; // this slide is also an element
    Q3DStudio::INT32 theChildStateIndex(inStateIndex + 1);

    for (bool theResult = inReader.MoveToFirstChild(); theResult;
         theResult = inReader.MoveToNextSibling()) {
        if (AreEqual(inReader.GetNarrowElementName(), "State")) {
            DoStateSectionCount(inReader, theChildStateIndex, outLogicCounter);
            ++theChildStateIndex;
        } else if (AreEqual(inReader.GetNarrowElementName(), "Add")
                   || AreEqual(inReader.GetNarrowElementName(), "Set")) {
            // SlideMultipler is used as follows:
            // For every element, animation or action, the slide is used to turn on/off them
            // Hence, if it is inside master, it will have an entry in the master and all the child
            // slides in the SlideManager ( hence, theChildSlideCount + 1 )
            // if it is local, it will have an entry in the master and it's slide, ( thus 2 )
            // the assumption made here is that State in uip is definitely 2 level, hence if it has
            // no child slides, it is definitely master
            INT32 theSlideMultiplier;
            if (theChildSlideCount == 0)
                theSlideMultiplier = 2;
            else
                theSlideMultiplier = theChildSlideCount + 1;
            DoRefSectionCount(inReader, theSlideMultiplier, inStateIndex, outLogicCounter);
        }
    }
}

//==============================================================================
/**
 *	@ param inNumSlideMultiplier		To know how this is used, see comments on the place
 *where SlideMultipler is calculated
 */
void CUIPParserImpl::DoRefSectionCount(qt3dsdm::IDOMReader &inReader, INT32 inNumSlideMultiplier,
                                       INT32, SLogicSectionCount &outLogicCounter)
{
    IDOMReader::Scope __refCountScope(inReader);
    const char8_t *theElementRef;
    inReader.UnregisteredAtt("ref", theElementRef);
    SElementData *theData(m_ParseElementManager.FindElementData(theElementRef));
    if (theData == NULL) {
        QT3DS_ASSERT(false);
        return;
    }

    if (AreEqual(inReader.GetNarrowElementName(), "Add"))
        outLogicCounter.m_SlideElementCount += inNumSlideMultiplier;

    // There are cases where some attributes only appear in the local slide element, but not at the
    // master slide. e.g. endTime.
    // For these attributes, we must make sure that it is added to the slide element attribute of
    // the master, so that everything will work correctly.
    // This is done via the slide force flag on the element property object.
    for (TAttrMap::iterator theIter = theData->m_PropertyMap.begin(),
                            theEnd = theData->m_PropertyMap.end();
         theIter != theEnd; ++theIter) {
        const char8_t *theAttValue = "";
        bool hasAtt = inReader.UnregisteredAtt(theIter->first.c_str(), theAttValue);
        if (theIter->second.m_SlideFlag == true
            && (theIter->second.m_SlideForceFlag == true || hasAtt == true)) {
            outLogicCounter.m_SlideAttributeCount += theIter->second.m_Arity * inNumSlideMultiplier;
        }
        // Strings we may still load regardless
        if (hasAtt && IsStringType(theIter->second.m_DataType))
            outLogicCounter.m_StringAttrCount += 1;
    }
    // Always add padding for now till I can figure out exactly why padding is needed
    // and find a minimal way to handle it.
    outLogicCounter.m_PaddingCount += inNumSlideMultiplier;
}

qt3ds::runtime::SSlidePlayInformation CUIPParserImpl::GetPlayMode(qt3dsdm::IDOMReader &inReader)
{
    using qt3ds::runtime::PlayMode;

    qt3ds::runtime::SSlidePlayInformation thePlayInformation;
    // Setup with the default value from the metadata
    eastl::string thePlayMode = m_MetaData.GetPropertyValueString(m_MetaData.Register("Slide"),
                                                                  m_MetaData.Register("playmode"));
    PlayMode::Enum theMode = PlayMode::StopAtEnd;

    if (thePlayMode == "Looping") {
        theMode = PlayMode::Looping;
    } else if (thePlayMode == "PingPong") {
        theMode = PlayMode::PingPong;
    } else if (thePlayMode == "Ping") {
        theMode = PlayMode::Ping;
    } else if (thePlayMode == "Play Through To...") {
        theMode = PlayMode::PlayThroughTo;
    }
    thePlayInformation.m_PlayMode = theMode;

    eastl::string theInitialPlayState = m_MetaData.GetPropertyValueString(
        m_MetaData.Register("Slide"), m_MetaData.Register("initialplaystate"));
    if (theInitialPlayState == "Pause")
        thePlayInformation.m_Paused = true;

    const char *theAttribute;
    if (inReader.Att("playmode", theAttribute)) {
        if (AreEqual(theAttribute, "Play Through To...")) {
            thePlayInformation.m_PlayMode = PlayMode::PlayThroughTo;
        } else if (AreEqual(theAttribute, "Stop at end")) {
            thePlayInformation.m_PlayMode = PlayMode::StopAtEnd;
        } else if (AreEqual(theAttribute, "Looping")) {
            thePlayInformation.m_PlayMode = PlayMode::Looping;
        } else if (AreEqual(theAttribute, "PingPong")) {
            thePlayInformation.m_PlayMode = PlayMode::PingPong;
        } else if (AreEqual(theAttribute, "Ping")) {
            thePlayInformation.m_PlayMode = PlayMode::Ping;
        }
    }

    if (inReader.Att("initialplaystate", theAttribute)) {
        if (AreEqual(theAttribute, "Play"))
            thePlayInformation.m_Paused = false;
        else
            thePlayInformation.m_Paused = true;
    }

    return thePlayInformation;
}

INT32 CUIPParserImpl::GetPlayThroughTo(INT32 inCurrentSlideIndex, qt3dsdm::IDOMReader &inReader)
{
    // Just hardcode it to Next since we know from the metadata the default is Next.
    // If we want to query from MetaData, need to write accessor to get back SStringOrInt
    INT32 thePlayThroughTo = inCurrentSlideIndex + 1;

    const char *theAttribute;
    if (inReader.Att("playthroughto", theAttribute) && *theAttribute) {
        if (AreEqual(theAttribute, "Previous"))
            thePlayThroughTo = inCurrentSlideIndex - 1;
        else if (AreEqual(theAttribute, "Next"))
            thePlayThroughTo = inCurrentSlideIndex + 1;
        else if (theAttribute[0] == '#') {
            theAttribute++;
            SParseSlide *theSlide = m_ParseSlideManager.FindSlide(theAttribute);
            if (theSlide)
                thePlayThroughTo = theSlide->m_SlideIndex;
            else
                Q_ASSERT(!"Slide Not Found");
        }
    }

    return thePlayThroughTo;
}

eastl::string CUIPParserImpl::GetSlideName(qt3dsdm::IDOMReader &inReader)
{
    eastl::string theSlideName;
    if (!inReader.Att("name", theSlideName))
        inReader.Att("id", theSlideName);
    return theSlideName;
}

eastl::string CUIPParserImpl::GetSlideId(qt3dsdm::IDOMReader &inReader)
{
    eastl::string theSlideId;
    if (!inReader.Att("id", theSlideId))
        inReader.Att("component", theSlideId);
    return theSlideId;
}

// In the event we added something to the slide, we return the element data.
SElementData *CUIPParserImpl::AddSlideElement(IPresentation &inPresentation, bool inMaster,
                                              qt3dsdm::IDOMReader &inReader, INT32 *outMaxTime)
{
    ISlideSystem &theBuilder = inPresentation.GetSlideSystem();

    SElementData *theElementData = NULL;
    const char8_t *theElementRef = "";
    if (inReader.UnregisteredAtt("ref", theElementRef))
        theElementData = m_ParseElementManager.FindElementData(theElementRef);

    SElement *theElement = theElementData ? theElementData->m_Element : NULL;
    if (theElement) {
        if (m_SlideElements.find(theElement) == m_SlideElements.end()) {
            const char *theEyeball;
            bool theActiveFlag = !(
                inMaster || (inReader.Att("eyeball", theEyeball) && AreEqual(theEyeball, "False")));

            m_SlideElements.insert(theElement);
            theBuilder.AddSlideElement(*theElement, theActiveFlag);
            m_NumSlideElements++;
            if (outMaxTime) {
                INT32 theElementMaxTime = 0;
                SElement *theParent = theElement->GetParent();
                if (theParent && theParent->IsComponent() && theElement) {
                    // Ignore material durations (in practice this is always a material container)
                    qt3dsdm::ComposerObjectTypes::Enum composerType(
                        qt3dsdm::ComposerObjectTypes::Convert(theElementData->m_Type.c_str()));
                    if (composerType != qt3dsdm::ComposerObjectTypes::Material
                            && inReader.Att("endtime", theElementMaxTime) == false) {
                        theElementMaxTime = m_MetaData.GetPropertyValueLong(
                                    theElementData->m_Type, m_MetaData.Register("endtime"),
                                    theElementData->m_Class);
                    }
                    *outMaxTime = NVMax(*outMaxTime, theElementMaxTime);
                }
            }
        } else
            theElementData = NULL;
    }

    return theElementData;
}

BOOL CUIPParserImpl::LoadSlideElements(IPresentation &inPresentation, qt3dsdm::IDOMReader &inReader,
                                       bool inMaster, SElement *inComponent, INT32 *outMaxTime)
{
    BOOL theSuccess = true;

    {
        IDOMReader::Scope __childScope(inReader);
        eastl::string theSlideId = GetSlideId(inReader);
        eastl::string theActionIdStr;
        // Setup the add/set commands for this slide.
        {
            IDOMReader::Scope __slideScope(inReader);
            // Load all the animations for this slide right now:

            SParseSlide *theSlide = m_ParseSlideManager.FindSlide(theSlideId.c_str());
            // All animations and actions added to the master slide are deactivated.
            QT3DSU32 animActions = 0;
            if (theSlide) {
                bool canActivateFlag = !inMaster;
                ISlideSystem &theBuilder = inPresentation.GetSlideSystem();
                for (TInstanceIdAnimationMap::iterator
                         theInstAnimIter = theSlide->m_Animations.begin(),
                         theInstAnimEnd = theSlide->m_Animations.end();
                     theInstAnimIter != theInstAnimEnd; ++theInstAnimIter) {
                    const TAnimationList &theAnimList(theInstAnimIter->second);
                    for (QT3DSU32 theAnimIdx = 0, theAnimEnd = theAnimList.size();
                         theAnimIdx < theAnimEnd; ++theAnimIdx) {
                        theBuilder.AddSlideAnimAction(
                            true, theAnimList[theAnimIdx].m_Animation->m_AnimationIndex,
                            theAnimList[theAnimIdx].m_Active && canActivateFlag);
                        ++animActions;
                    }
                }
                // Build the actions for this slide.
                for (TActionList::iterator theActionIter = theSlide->m_ActionList.begin(),
                                           theActionEnd = theSlide->m_ActionList.end();
                     theActionIter != theActionEnd; ++theActionIter) {
                    SParseSlideActionEntry &theActionEntry(*theActionIter);
                    theActionIdStr.assign(theActionEntry.m_ActionId.c_str());
                    qt3ds::QT3DSI32 theActionIndex = m_ActionHelper->GetActionIndex(theActionIdStr);
                    for (qt3ds::QT3DSI32 idx = 0; idx < theActionEntry.m_ActionCount; ++idx) {
                        qt3ds::runtime::SSlideAnimAction *theSlideData =
                            theBuilder.AddSlideAnimAction(false, theActionIndex + idx,
                                                          theActionEntry.m_Active
                                                              && canActivateFlag);
                        theActionEntry.m_Actions[idx] = theSlideData;
                        ++animActions;
                    }
                }

                // qt3ds::NVFoundationBase& fnd(
                // inPresentation.GetApplication().GetRuntimeFactory().GetFoundation() );
                // fnd.error( QT3DS_TRACE, "Loading slide %s, %d anim actions", theSlideId.c_str(),
                // animActions );
            }
            for (BOOL theResult = inReader.MoveToFirstChild(); theResult;
                 theResult = inReader.MoveToNextSibling()) {
                if (AreEqual(inReader.GetNarrowElementName(), "Add")
                    || AreEqual(inReader.GetNarrowElementName(), "Set")) {
                    SElementData *theAddedElement =
                        AddSlideElement(inPresentation, inMaster, inReader, outMaxTime);
                    if (theAddedElement && theAddedElement->m_Element) {
                        LoadSlideElementAttrs(inPresentation, inMaster, *theAddedElement, inReader,
                                              inComponent);
                        // This can define actions or set the active
                        ProcessSlideAnimAction(inPresentation, theSlideId, inMaster, inReader);
                    }
                }
            }
        }
        // Pull add commands from child slides into master slide with active=false
        // else things won't get reset when we switch slides.
        {
            // All child state adds go into this object with active set to false
            IDOMReader::Scope __masterScope(inReader);
            for (BOOL theResult = inReader.MoveToFirstChild("State"); theResult;
                 theResult = inReader.MoveToNextSibling("State")) {
                IDOMReader::Scope __childScope(inReader);
                for (BOOL theResult = inReader.MoveToFirstChild("Add"); theResult;
                     theResult = inReader.MoveToNextSibling("Add")) {
                    // Add the slide element, setting the active flag to false.
                    AddSlideElement(inPresentation, inMaster, inReader, outMaxTime);
                }
            }
        }
        // Pull add commands from master slide for objects in this slide.
        // We do this after we process the add/set commands for this slide so that objects already
        // added
        // get ignored.  If we do pull an object forward then we have to set all of the force-set
        // properties as well
        // as account for a possible end-time difference.
        {
            IDOMReader::Scope __childScope(inReader);
            inReader.Leave();
            if (AreEqual(inReader.GetNarrowElementName(), "State")) {
                TPropertyDescAndValueList theProperties;
                for (BOOL theResult = inReader.MoveToFirstChild("Add"); theResult;
                     theResult = inReader.MoveToNextSibling("Add")) {
                    theProperties.clear();
                    SElementData *theData =
                        AddSlideElement(inPresentation, inMaster, inReader, NULL);
                    if (theData) {
                        // Add all forced attributes to this slide.
                        for (TAttrMap::iterator theIter = theData->m_PropertyMap.begin(),
                                                theEnd = theData->m_PropertyMap.end();
                             theIter != theEnd; ++theIter) {
                            if (theIter->second.m_SlideForceFlag) {
                                GetMetaAttribute(inPresentation, theProperties, theData->m_Type,
                                                 theIter->first, theData->m_Class,
                                                 theIter->second.m_PropertyNames);
                            }
                        }
                        ISlideSystem &theBuilder = inPresentation.GetSlideSystem();
                        for (TPropertyDescAndValueList::iterator theIter = theProperties.begin();
                             theIter != theProperties.end(); ++theIter) {
                            theBuilder.AddSlideAttribute(theIter->first.GetAttributeKey(),
                                                         theIter->second);
                            m_NumSlideAttributes++;
                        }

                        // Ensure the actual end time is accounted for.
                        if (outMaxTime && theData->m_Element) {
                            SElement *theParent = theData->m_Element->GetParent();
                            if (theParent && theParent->IsComponent()) {
                                // Ignore material durations
                                // (in practice this is always a material container)
                                qt3dsdm::ComposerObjectTypes::Enum composerType(
                                    qt3dsdm::ComposerObjectTypes::Convert(theData->m_Type.c_str()));
                                if (composerType != qt3dsdm::ComposerObjectTypes::Material) {
                                    long theoutMaxTime = (long)*outMaxTime;
                                    long theMaxTime = m_MetaData.GetPropertyValueLong(
                                        theData->m_Type, m_MetaData.Register("endtime"),
                                        theData->m_Class);
                                    *outMaxTime = NVMax(theoutMaxTime, theMaxTime);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return theSuccess;
}

static inline bool IsDynamicObject(TStrType type)
{
    return strcmp(type.c_str(), "CustomMaterial") == 0 || strcmp(type.c_str(), "Effect") == 0;
}

BOOL CUIPParserImpl::LoadSlideElementAttrs(IPresentation &inPresentation, bool masterSlide,
                                           SElementData &inElementData, IDOMReader &inReader,
                                           SElement *inComponent)
{
    ISlideSystem &theBuilder = inPresentation.GetSlideSystem();

    TPropertyDescAndValueList theAttributeList;

    if (masterSlide)
        inElementData.m_Element->m_OnMaster = true;

    const char8_t *theRef = "";
    inReader.Att("ref", theRef);
    if (theRef && *theRef && theRef[0] == '#')
        ++theRef;

    bool isInsideMaterialContainer = m_materialContainerIds.contains(theRef);

    bool isSet = AreEqual(inReader.GetNarrowElementName(), "Set");
    const char8_t *sourcepath;
    if (inReader.UnregisteredAtt("sourcepath", sourcepath)) {
        const char8_t *mapping;
        bool ibl = false;
        if (inReader.UnregisteredAtt("mappingmode", mapping)) {
            if (QString::fromUtf8(mapping) == QLatin1String("Light Probe"))
                ibl = true;
        }
        if (!IsTrivial(sourcepath) && sourcepath[0] != '#') {
            m_sourcePathsById[QLatin1Char('#') + QString::fromUtf8(theRef)].append(
                        QString::fromUtf8(sourcepath));
            // Don't add material container assets to the Master Slide source path list
            if (!isInsideMaterialContainer) {
                AddSourcePath(sourcepath, ibl);
                theBuilder.AddSourcePath(sourcepath);
            }
            // Add the material container assets to the list of source paths used by slides
            // other than the Master Slide, so that the assets are not loaded at startup
            if (!masterSlide || isInsideMaterialContainer)
                m_slideSourcePaths.push_back(QString::fromLatin1(sourcepath));
            if (AreEqual(inElementData.m_Type.c_str(), "Layer"))
                theBuilder.AddSubPresentation(sourcepath);
        }
    }

    const char8_t *subpres;
    if (inReader.UnregisteredAtt("subpresentation", subpres)) {
        if (!IsTrivial(subpres))
            theBuilder.AddSubPresentation(subpres);
    }

    const char8_t *referencedmaterial;
    if (inReader.UnregisteredAtt("referencedmaterial", referencedmaterial)) {
        if (!IsTrivial(referencedmaterial))
            m_currentSlideReferencedMaterials.append(referencedmaterial);
    }

    if (strcmp(inElementData.m_Type, "Material") == 0) {
        eastl::vector<eastl::string> propertyList;
        m_MetaData.GetInstanceProperties(inElementData.m_Type, inElementData.m_Class,
                                         propertyList, false);
        for (int i = 0; i < propertyList.size(); ++i) {
            ERuntimeDataModelDataType type = m_MetaData.GetPropertyType(inElementData.m_Type,
                                       m_MetaData.Register(propertyList[i].c_str()),
                                       inElementData.m_Class);
            if (type == ERuntimeDataModelDataTypeLong4) {
                const char8_t *attValue = "";
                bool hasAtt = inReader.UnregisteredAtt(propertyList[i].c_str(), attValue);
                if (hasAtt) {
                    m_imagesByMaterial[QLatin1Char('#') + inElementData.m_Id.c_str()]
                            .append(QString::fromUtf8(attValue));
                }
            }
        }
    }

    const bool dyn = IsDynamicObject(inElementData.m_Type);
    Option<qt3dsdm::SMetaDataCustomMaterial> mat;
    Option<qt3dsdm::SMetaDataEffect> eff;
    SMetaDataDynamicObject *dynMetaData = nullptr;
    if (dyn) {
        // Dynamic object do not list all properties in the elementData property map for some reason
        if (strcmp(inElementData.m_Type, "CustomMaterial") == 0) {
            mat = m_MetaData.GetMaterialMetaDataByName(inElementData.m_Class);
            if (mat.hasValue())
                dynMetaData = &mat.getValue();
        } else if (strcmp(inElementData.m_Type, "Effect") == 0) {
            eff = m_MetaData.GetEffectMetaDataByName(inElementData.m_Class);
            if (eff.hasValue())
                dynMetaData = &eff.getValue();
        }
    }

    // We don't force set attributes when a given component has a set command within one of its
    // child states.  This happens in the case of actions (but nothing else).
    bool shouldForce = inElementData.m_Element != inComponent;
    for (TAttrMap::iterator theIter = inElementData.m_PropertyMap.begin(),
                            theEnd = inElementData.m_PropertyMap.end();
         theIter != theEnd; ++theIter) {
        const char8_t *theAttValue = "";
        bool hasAtt = inReader.UnregisteredAtt(theIter->first, theAttValue);
        QT3DSU32 previousListSize = theAttributeList.size();
        if (hasAtt == false) {
            // StartTime and EndTime in Master Slide are useless, so set it to the value in the
            // metadata.
            // if we look at the uip file, if the element doesn't have endTime, it means is 10000ms,
            // and not really the value in the master slide
            // this make it special from other attributes
            if (theIter->second.m_SlideForceFlag && shouldForce)
                GetMetaAttribute(inPresentation, theAttributeList, inElementData.m_Type,
                                 theIter->first, inElementData.m_Class,
                                 theIter->second.m_PropertyNames);
        } else {
            // Only use the attribute list from set blocks to set values into the slide, not from
            // add attributes
            // This isn't completely correct
            GetAttributeList(inPresentation, theAttributeList, inElementData.m_Type, theIter->first,
                             inElementData.m_Class, theAttValue, theIter->second.m_PropertyNames);
        }
        // Handle dynamic object texture source paths
        if (hasAtt && !IsTrivial(theAttValue) && IsDynamicObject(inElementData.m_Type)
                && theIter->second.m_AdditionalType == ERuntimeAdditionalMetaDataTypeTexture) {
            m_sourcePathsById[QLatin1Char('#') + QString::fromUtf8(theRef)].append(
                        QString::fromUtf8(theAttValue));
            if (!isInsideMaterialContainer) {
                AddSourcePath(theAttValue, false);
                theBuilder.AddSourcePath(theAttValue);
            }
            if (!masterSlide || isInsideMaterialContainer)
                m_slideSourcePaths.push_back(QString::fromLatin1(theAttValue));
        }
        if (isSet == false && theIter->second.m_SlideForceFlag == false) {
            if (inElementData.m_Element != NULL && theIter->second.m_ElementFlag) {
                for (QT3DSU32 idx = previousListSize, end = theAttributeList.size(); idx < end;
                     ++idx) {
                    UVariant *theValue = inElementData.m_Element->FindPropertyValue(
                        theAttributeList[idx].first.name());
                    if (theValue) {
                        *theValue = theAttributeList[idx].second;
                    }
                }
            }
            theAttributeList.resize(previousListSize);
        }
    }
    if (dynMetaData) {
        for (int i = 0; i < dynMetaData->m_Properties.size(); ++i) {
            qt3ds::render::dynamic::SPropertyDefinition prop = dynMetaData->m_Properties[i];
            if (prop.m_DataType == qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                const char8_t *theAttValue = "";
                bool hasAtt = inReader.UnregisteredAtt(prop.m_Name, theAttValue);
                if (hasAtt && !IsTrivial(theAttValue)) {
                    m_sourcePathsById[QLatin1Char('#') + QString::fromUtf8(theRef)].append(
                                QString::fromUtf8(theAttValue));
                    if (!isInsideMaterialContainer) {
                        AddSourcePath(theAttValue, false);
                        theBuilder.AddSourcePath(theAttValue);
                    }
                    if (!masterSlide || isInsideMaterialContainer)
                        m_slideSourcePaths.push_back(QString::fromLatin1(theAttValue));
                }
            }
        }
    }
    for (TPropertyDescAndValueList::iterator theIter = theAttributeList.begin();
         theIter != theAttributeList.end(); ++theIter) {
        theBuilder.AddSlideAttribute(theIter->first.GetAttributeKey(), theIter->second);
        m_NumSlideAttributes++;
    }

    return TRUE;
}

void CUIPParserImpl::LoadAnimationTrack(IPresentation &inPresentation, SElementData &inElementData,
                                        SParseSlideAnimationEntry &inAnimation)
{
    SElementPropertyInfo *theInfo =
        m_ParseElementManager.FindProperty(inElementData, inAnimation.m_PropertyName);
    if (theInfo == NULL)
        return;
    UINT32 theHashedKey = inAnimation.m_PropertyHash;
    bool theIsDynamic = inAnimation.m_IsDynamic;
    INT32 theIndex = inPresentation.GetAnimationSystem().CreateAnimationTrack(
        *inElementData.m_Element, theHashedKey, theIsDynamic);
    inAnimation.m_AnimationIndex = theIndex;
    m_NumAnimationTracks++;
    bool isRotation = theInfo->m_AdditionalType == ERuntimeAdditionalMetaDataTypeRotation;
    LoadAnimationKeys(inPresentation, inAnimation, isRotation);
}

void CUIPParserImpl::LoadAnimationKeys(IPresentation &inPresentation,
                                       const SParseSlideAnimationEntry &inAnimation,
                                       bool inIsRotation)
{

    NVConstDataRef<qt3ds::QT3DSF32> theFloatValues(inAnimation.m_KeyframeData.data(),
                                             (qt3ds::QT3DSU32)inAnimation.m_KeyframeData.size());
    switch (inAnimation.m_AnimationType) {
    case SParseSlideAnimationTypes::Linear:
        LoadLinearKeys(inPresentation, theFloatValues, inIsRotation);
        break;
    case SParseSlideAnimationTypes::Bezier:
        LoadBezierKeys(inPresentation, theFloatValues, inIsRotation);
        break;
    default:
        QT3DS_ASSERT(false);
    case SParseSlideAnimationTypes::EaseInOut:
        LoadEaseInOutKeys(inPresentation, theFloatValues, inIsRotation);
        break;
    }
}

BOOL CUIPParserImpl::LoadBezierKeys(IPresentation &inPresentation, NVConstDataRef<float> &inValues,
                                    bool inIsRotation)
{
    QT3DS_ASSERT(inValues.size() % 6 == 0);
    if (inValues.size() % 6 != 0)
        return FALSE;

    IAnimationSystem &theBuilder = inPresentation.GetAnimationSystem();

    float theTime;
    float theValue;
    float theC1Time;
    float theC1Value;
    float theC2Time;
    float theC2Value;

    const float *theIter = inValues.begin();

    long theNumKeys = inValues.size() / 6;
    QT3DS_ASSERT(theNumKeys > 0);
    for (long theIndex = 0; theIndex < theNumKeys; ++theIndex) {
        theTime = *theIter * 1000.0f;
        ++theIter;
        theValue = *theIter;
        ++theIter;
        ++theIter; // just increment the iterator, C2Time is set below
        ++theIter; // just increment the iterator, C2Value is set below
        theC1Time = *theIter * 1000.0f;
        ++theIter;
        theC1Value = *theIter;
        ++theIter;

        if (theIndex == theNumKeys - 1) {
            // Last keyframe
            theC2Time = 0.0f;
            theC2Value = 0.0f;
        } else {
            theC2Time = *(theIter + 2) * 1000.0f; // access the next inTangentTime
            theC2Value = *(theIter + 3); // access the next inTangentValue
        }

        if (inIsRotation) {
            TORAD(theValue);
            TORAD(theC1Value);
            TORAD(theC2Value);
        }

        theBuilder.AddKey(theTime, theValue, theC1Time, theC1Value, theC2Time, theC2Value);
        m_NumAnimationKeys++;
    }
    return TRUE;
}

BOOL CUIPParserImpl::LoadLinearKeys(IPresentation &inPresentation, NVConstDataRef<float> &inValues,
                                    bool inIsRotation)
{
    QT3DS_ASSERT(inValues.size() % 2 == 0);
    if (inValues.size() % 2 != 0)
        return FALSE;

    const float *theIter = inValues.begin();

    long theNumKeys = inValues.size() / 2;
    float *theEaseInOutValues = new float[theNumKeys * 4];
    float *theEaseInOutPtr = theEaseInOutValues;
    for (long theKeyIndex = 0; theKeyIndex < theNumKeys; ++theKeyIndex) {
        *theEaseInOutPtr++ = *theIter++;
        *theEaseInOutPtr++ = *theIter++;
        *theEaseInOutPtr++ = 0.0f;
        *theEaseInOutPtr++ = 0.0f;
    }

    NVConstDataRef<float> theFloatValues =
        NVConstDataRef<float>(theEaseInOutValues, theNumKeys * 4);
    BOOL theStatus = LoadEaseInOutKeys(inPresentation, theFloatValues, inIsRotation);

    delete[] theEaseInOutValues;

    return theStatus;
}

CUIPParserImpl::SEaseInEaseOutKeyframe CUIPParserImpl::ParseEaseInOutKey(const float *&inFloatIter)
{
    CUIPParserImpl::SEaseInEaseOutKeyframe theKeyframe;
    theKeyframe.m_Time = *inFloatIter;
    ++inFloatIter;
    theKeyframe.m_Value = *inFloatIter;
    ++inFloatIter;
    theKeyframe.m_EaseIn = *inFloatIter;
    ++inFloatIter;
    theKeyframe.m_EaseOut = *inFloatIter;
    ++inFloatIter;
    return theKeyframe;
}

BOOL CUIPParserImpl::LoadEaseInOutKeys(IPresentation &inPresentation,
                                       NVConstDataRef<float> &inValues, bool inIsRotation)
{
    QT3DS_ASSERT(inValues.size() % 4 == 0);
    if (inValues.size() % 4 != 0)
        return FALSE;

    long theNumKeys = inValues.size() / 4;

    float *theBezierValues = new float[theNumKeys * 6];
    float *theBezierPtr = theBezierValues;

    SEaseInEaseOutKeyframe prevKeyframe;
    SEaseInEaseOutKeyframe currKeyframe;
    SEaseInEaseOutKeyframe nextKeyframe;

    const float *theIter = inValues.begin();

    if (theNumKeys > 0) {
        currKeyframe = ParseEaseInOutKey(theIter);
        float nextKfTime = -1.f; // -1 = no value provided

        if (theNumKeys > 1) {
            nextKeyframe = ParseEaseInOutKey(theIter);
            nextKfTime = nextKeyframe.m_Time;
        }
        CreateBezierKeyframeFromEaseInEaseOutKeyframe(-1.f, currKeyframe, nextKfTime, theBezierPtr);
    }

    for (long theKeyIndex = 1; theKeyIndex < theNumKeys; ++theKeyIndex) {
        prevKeyframe = currKeyframe;
        currKeyframe = nextKeyframe;

        float prevKfTime = prevKeyframe.m_Time;
        float nextKfTime = -1.f; // -1 = no value provided

        if (theKeyIndex + 1 < theNumKeys) {
            nextKeyframe = ParseEaseInOutKey(theIter);
            nextKfTime = nextKeyframe.m_Time;
        }
        CreateBezierKeyframeFromEaseInEaseOutKeyframe(prevKfTime, currKeyframe, nextKfTime,
                                                      theBezierPtr);
    }

    NVConstDataRef<float> theFloatValues = NVConstDataRef<float>(theBezierValues, theNumKeys * 6);
    BOOL theStatus = LoadBezierKeys(inPresentation, theFloatValues, inIsRotation);

    delete[] theBezierValues;
    return theStatus;
}

void CUIPParserImpl::CreateBezierKeyframeFromEaseInEaseOutKeyframe(
        float prevTime, SEaseInEaseOutKeyframe &keyframe, float nextTime, float *&outBezierValues)
{
    float kfValue = keyframe.m_Value;
    float kfTime = keyframe.m_Time; // seconds
    float timeIn = kfTime;
    float valueIn = kfValue;
    float timeOut = kfTime;
    float valueOut = kfValue;

    // at 100 ease, the control point will be midway between the 2 keyframes
    float maxEasePerc = .5f;

    if (prevTime != -1.f) {
        float easeInPerc = qBound(0.f, keyframe.m_EaseIn / 100.f, 1.f) * maxEasePerc;
        timeIn = prevTime + (kfTime - prevTime) * (1 - easeInPerc);
    }

    if (nextTime != -1.f) {
        float easeOutPerc = qBound(0.f, keyframe.m_EaseOut / 100.f, 1.f) * maxEasePerc;
        timeOut = kfTime + (nextTime - kfTime) * easeOutPerc;
    }

    *outBezierValues++ = kfTime;
    *outBezierValues++ = kfValue;
    *outBezierValues++ = timeIn;
    *outBezierValues++ = valueIn;
    *outBezierValues++ = timeOut;
    *outBezierValues++ = valueOut;
}

BOOL CUIPParserImpl::ProcessSlideAnimAction(IPresentation &inPresentation, eastl::string &inSlideId,
                                            bool inMaster, qt3dsdm::IDOMReader &inReader)
{
    IDOMReader::Scope __animationScope(inReader);

    BOOL theStatus = true;

    for (BOOL theResult = inReader.MoveToFirstChild("Action"); theResult;
         theResult = inReader.MoveToNextSibling("Action")) {
        // Get the active flag
        const char *theEyeball;
        bool theActiveFlag =
            !(inMaster || (inReader.Att("eyeball", theEyeball) && AreEqual(theEyeball, "False")));
        theStatus &= AddSlideAction(inPresentation, inSlideId, theActiveFlag, inReader);
    }

    return theStatus;
}

BOOL CUIPParserImpl::AddSlideAction(IPresentation &, eastl::string &inSlideId, bool inActive,
                                    qt3dsdm::IDOMReader &inReader)
{
    SParseSlide *theSlide = m_ParseSlideManager.FindSlide(inSlideId.c_str());
    if (theSlide == NULL) {
        QT3DS_ASSERT(false);
        return false;
    }

    // Get the Action id
    const char *theRef;
    if (inReader.Att("ref", theRef)) {
        theRef++; // remove the '#'
        SParseSlideActionEntry *theEntry = m_ParseSlideManager.FindAction(*theSlide, theRef);
        if (theEntry == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }
        for (qt3ds::QT3DSI32 actIdx = 0; actIdx < theEntry->m_ActionCount; ++actIdx) {
            if (theEntry->m_Actions[actIdx])
                theEntry->m_Actions[actIdx]->m_Active = inActive ? 1 : 0;
        }
    } else {
        const char8_t *theActionId;
        inReader.Att("id", theActionId);
        SParseSlideActionEntry *theEntry = m_ParseSlideManager.FindAction(*theSlide, theActionId);
        QT3DS_ASSERT(theEntry);
        // Check to ensure this action was created.
        (void)theEntry;
    }

    return TRUE;
}

bool CUIPParserImpl::IsStringType(ERuntimeDataModelDataType inDataType)
{
    if (inDataType == ERuntimeDataModelDataTypeStringRef
        || inDataType == ERuntimeDataModelDataTypeString
        || inDataType == ERuntimeDataModelDataTypeObjectRef)
        return true;
    return false;
}

SElementData *CUIPParserImpl::GetElementData(TStrType inElementName)
{
    return m_ParseElementManager.FindElementData(inElementName);
}

//==============================================================================
/**
 *	Helper function to to get the SElement object from the element name
 */
SElementData *CUIPParserImpl::GetElementData(const char8_t *inElementName)
{
    const char8_t *theElementName(inElementName);
    if (!IsTrivial(inElementName) && inElementName[0] == '#')
        ++theElementName;
    return GetElementData(m_MetaData.Register(theElementName));
}

SElement *CUIPParserImpl::GetElement(const char8_t *inElementName)
{
    SElementData *theData(GetElementData(inElementName));
    if (theData)
        return theData->m_Element;
    return NULL;
}

SElementData *CUIPParserImpl::ParseObjectRef(const eastl::string &inObjectPath,
                                             const char8_t *inOwnerId)
{
    TStrType theId(ParseObjectRefId(inObjectPath, inOwnerId));
    return GetElementData(theId);
}

TStrType CUIPParserImpl::ParseObjectRefId(const eastl::string &inObjectPath,
                                          const char8_t *inOwnerId)
{
    return m_ObjectRefHelper->ParseObjectRefId(inObjectPath, inOwnerId);
}

SElementAndType CUIPParserImpl::GetElementForID(const char *inElementName)
{
    SElementData *theData = m_ParseElementManager.FindElementData(inElementName);
    if (theData == NULL)
        return SElementAndType(UIPElementTypes::Unknown, NULL);

    qt3dsdm::ComposerObjectTypes::Enum theComposerType(
        qt3dsdm::ComposerObjectTypes::Convert(theData->m_Type.c_str()));
    UIPElementTypes::Enum theUIPType(UIPElementTypes::Unknown);
    switch (theComposerType) {
    case qt3dsdm::ComposerObjectTypes::Scene:
        theUIPType = UIPElementTypes::Scene;
        break;
    case qt3dsdm::ComposerObjectTypes::Layer:
        theUIPType = UIPElementTypes::Layer;
        break;
    case qt3dsdm::ComposerObjectTypes::Group:
        theUIPType = UIPElementTypes::Group;
        break;
    case qt3dsdm::ComposerObjectTypes::Component:
        theUIPType = UIPElementTypes::Component;
        break;
    case qt3dsdm::ComposerObjectTypes::Camera:
        theUIPType = UIPElementTypes::Camera;
        break;
    case qt3dsdm::ComposerObjectTypes::Light:
        theUIPType = UIPElementTypes::Light;
        break;
    case qt3dsdm::ComposerObjectTypes::Model:
        theUIPType = UIPElementTypes::Model;
        break;
    case qt3dsdm::ComposerObjectTypes::Material:
        theUIPType = UIPElementTypes::Material;
        break;
    case qt3dsdm::ComposerObjectTypes::Image:
        theUIPType = UIPElementTypes::Image;
        break;
    case qt3dsdm::ComposerObjectTypes::Behavior:
        theUIPType = UIPElementTypes::Behavior;
        break;
    case qt3dsdm::ComposerObjectTypes::Text:
        theUIPType = UIPElementTypes::Text;
        break;
    case qt3dsdm::ComposerObjectTypes::Effect:
        theUIPType = UIPElementTypes::Effect;
        break;
    case qt3dsdm::ComposerObjectTypes::CustomMaterial:
        theUIPType = UIPElementTypes::CustomMaterial;
        break;
    case qt3dsdm::ComposerObjectTypes::ReferencedMaterial:
        theUIPType = UIPElementTypes::ReferencedMaterial;
        break;
    case qt3dsdm::ComposerObjectTypes::RenderPlugin:
        theUIPType = UIPElementTypes::RenderPlugin;
        break;
    case qt3dsdm::ComposerObjectTypes::Path:
        theUIPType = UIPElementTypes::Path;
        break;
    case qt3dsdm::ComposerObjectTypes::PathAnchorPoint:
        theUIPType = UIPElementTypes::PathAnchorPoint;
        break;
    case qt3dsdm::ComposerObjectTypes::SubPath:
        theUIPType = UIPElementTypes::PathSubPath;
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }
    return SElementAndType(theUIPType, theData->m_Element);
}

eastl::string CUIPParserImpl::ResolveReference(const char *inStringId, const char *inReference)
{
    SElementData *theData = ParseObjectRef(inReference, inStringId);
    if (theData) {
        return theData->m_Id.c_str();
    }
    return eastl::string();
}

IRuntimeMetaData &CUIPParserImpl::GetMetaData()
{
    return m_MetaData;
}

IUIPParser &IUIPParser::Create(const QString &inFileName, IRuntimeMetaData &inMetaData,
                               IInputStreamFactory &inFactory,
                               qt3ds::foundation::IStringTable &inStrTable)
{
    CUIPParserImpl &retval = *new CUIPParserImpl(inFileName, inMetaData, inFactory, inStrTable);
    return retval;
}
}
