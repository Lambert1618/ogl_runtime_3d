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
#pragma warning(disable : 4100) // std::variant
#pragma warning(disable : 4396) // specializer warning nonsense
#endif

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSUIPParserObjectRefHelper.h"

#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSMetadata.h"
using namespace qt3dsdm;

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	 Constants
//==============================================================================
static const char PATH_DELIMITER = '.'; // can only be single char!
static const char PATH_LINE_END = '\n';

//==============================================================================
/**
 * Constructor
 */
CUIPParserObjectRefHelper::CUIPParserObjectRefHelper(IRuntimeMetaData &inMetaData)
    : m_MetaData(inMetaData)
    , m_MaterialStr(m_MetaData.Register("Material"))
{
    BuildImageNamePropertyList();

    if (m_LayerImageIdToNameMap.empty()) {
        {
            TLightProbeAndNamePair theNamePair =
                eastl::make_pair<eastl::string, eastl::string>("Layer_lightprobe", "lightprobe");
            m_LayerImageIdToNameMap.insert(theNamePair);
        }
        {
            TLightProbeAndNamePair theNamePair =
                eastl::make_pair<eastl::string, eastl::string>("Layer_lightprobe2", "lightprobe2");
            m_LayerImageIdToNameMap.insert(theNamePair);
        }
    }
}

//==============================================================================
/**
 * Destructor
 */
CUIPParserObjectRefHelper::~CUIPParserObjectRefHelper()
{
    // Delete SGraphNode
    for (TGraphNodeMap::iterator theIter = m_GraphNodeMap.begin(); theIter != m_GraphNodeMap.end();
         ++theIter)
        delete theIter->second;
    m_GraphNodeMap.clear();
    m_RootNode = NULL;
}

namespace {
    void StoreTopLevelAliasAttributes(
        IDOMReader &inReader,
        eastl::vector<eastl::pair<eastl::string, eastl::string>> &copiedAttributes)
    {
        for (eastl::pair<const char8_t *, const char8_t *> att = inReader.GetNarrowFirstAttribute();
             !isTrivial(att.first); att = inReader.GetNarrowNextAttribute()) {
            if (!AreEqual(att.first, "scale") && !AreEqual(att.first, "position")
                && !AreEqual(att.first, "rotation") && !AreEqual(att.first, "pivot")
                && !AreEqual(att.first, "endtime") && !AreEqual(att.first, "eyeball")
                && !AreEqual(att.first, "ref") && !AreEqual(att.first, "id")
                && !AreEqual(att.first, "name") && !AreEqual(att.first, "orientation")
                && !AreEqual(att.first, "rotationorder")) // note that sourcepath does come through.
            {
                copiedAttributes.push_back(eastl::make_pair(att.first, att.second));
            }
        }
    }
}

void CUIPParserObjectRefHelper::CacheGraph(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inWriter)
{
    {
        // First, we parse Graph section to build the scene graph
        IDOMReader::Scope __graphScope(inReader);
        inReader.MoveToFirstChild("Graph");
        CacheSceneGraph(inReader);
    }
    void *logicScopeItem = NULL;
    {
        // Next, we parse Logic section to update the node name and image information
        IDOMReader::Scope __logicScope(inReader);
        inReader.MoveToFirstChild("Logic");
        logicScopeItem = inReader.GetScope();
        CacheStateGraph(inReader);
    }
    if (m_SceneGraphAliasList.empty() == false) {
        IDOMReader::Scope __aliasScope(inReader);
        // Now we expand aliases
        eastl::hash_map<TStrType, TStrType> oldToNewIdMap;
        std::shared_ptr<qt3dsdm::IDOMFactory> theFactory = inWriter.GetFactory();
        std::shared_ptr<qt3dsdm::IStringTable> theStrTable = inReader.GetStringTable();
        qt3dsdm::SDOMElement *theSlidesRoot = theFactory->NextElement("AliasSlides");
        eastl::pair<std::shared_ptr<qt3dsdm::IDOMWriter>, std::shared_ptr<qt3dsdm::IDOMReader>>
            slideWriterReaderPair =
                qt3dsdm::IDOMWriter::CreateDOMWriter(theFactory, *theSlidesRoot, theStrTable);
        std::shared_ptr<qt3dsdm::IDOMWriter> slideWriter = slideWriterReaderPair.first;

        {
            IDOMReader::Scope __loopScope(inReader);
            for (QT3DSU32 idx = 0, end = m_SceneGraphAliasList.size(); idx < end; ++idx) {
                inReader.SetScope(m_SceneGraphAliasList[idx]);
                const char8_t *reference;
                SGraphNode *referencedNode = NULL;
                const char8_t *theIdStr;
                inReader.Att("id", theIdStr);
                SGraphNode *aliasNode = NULL;
                TGraphNodeMap::iterator iter = m_GraphNodeMap.find(m_MetaData.Register(theIdStr));
                if (iter != m_GraphNodeMap.end())
                    aliasNode = iter->second;

                if (inReader.UnregisteredAtt("referencednode", reference)) {
                    TStrType theSourceElemId = ParseObjectRefId(reference, theIdStr);
                    iter = m_GraphNodeMap.find(theSourceElemId);
                    if (iter != m_GraphNodeMap.end())
                        referencedNode = iter->second;
                }
                if (referencedNode == NULL || aliasNode == NULL) {
                    QT3DS_ASSERT(false);
                    continue;
                }
                inReader.SetScope(referencedNode->m_ReaderContext);
                oldToNewIdMap.clear();
                std::shared_ptr<qt3dsdm::IDOMFactory> theFactory = inWriter.GetFactory();
                std::shared_ptr<qt3dsdm::IStringTable> theStrTable = inReader.GetStringTable();
                qt3dsdm::SDOMElement *theRoot = theFactory->NextElement(inReader.GetElementName());
                std::shared_ptr<qt3dsdm::IDOMWriter> copyWriter =
                    qt3dsdm::IDOMWriter::CreateDOMWriter(theFactory, *theRoot, theStrTable).first;

                // Step one is just to copy the scene graph generating ids.
                SGraphNode *theParent = aliasNode->m_Parent;
                aliasNode->m_Class = referencedNode->m_Class;
                aliasNode->m_Type = referencedNode->m_Type;
                // Copy the alias id
                copyWriter->Att("id", theIdStr);
                eastl::vector<eastl::pair<eastl::string, eastl::string>> copiedAttributes;
                StoreTopLevelAliasAttributes(inReader, copiedAttributes);
                for (QT3DSU32 idx = 0, end = copiedAttributes.size(); idx < end; ++idx) {
                    copyWriter->Att(copiedAttributes[idx].first.c_str(),
                                    copiedAttributes[idx].second.c_str());
                }
                for (bool success = inReader.MoveToFirstChild(); success;
                     success = inReader.MoveToNextSibling()) {
                    if (AreEqual(inReader.GetNarrowElementName(), "Alias"))
                        continue;
                    qt3dsdm::IDOMReader::Scope __loopScoper(inReader);
                    qt3dsdm::IDOMWriter::Scope writerScope(*copyWriter,
                                                         inReader.GetNarrowElementName());
                    CopySceneGraph(inReader, *copyWriter, *slideWriter, oldToNewIdMap,
                                   aliasNode->m_Name.c_str(), theIdStr, *aliasNode);
                }
                if (referencedNode->m_MasterSlide) {
                    inReader.SetScope(referencedNode->m_MasterSlide);
                    CopyStates(inReader, *slideWriter, oldToNewIdMap, aliasNode->m_Name.c_str(),
                               theIdStr);
                }
                // Set the scope to point at the alias node.
                inReader.SetScope(aliasNode->m_ReaderContext);
                inWriter.ReplaceCurrent(*theRoot);
                // Now find the owning component.
                SGraphNode *theComponent = theParent;
                while (theComponent && theComponent->m_MasterSlide == NULL)
                    theComponent = theComponent->m_Parent;

                if (theComponent && theComponent->m_MasterSlide) {
                    inReader.SetScope(theComponent->m_MasterSlide);
                    // Copy any state commands that have entries in the old to new map.
                    CopyStateCommands(inReader, inWriter, oldToNewIdMap, aliasNode->m_Name.c_str(),
                                      referencedNode->m_Id, theIdStr);
                } else {
                    QT3DS_ASSERT(false);
                }
            }
        } // End of loop scope.

        {
            // Next, we parse Logic section to update the node name and image information
            {
                IDOMReader::Scope __logicScope(inReader);
                inReader.SetScope(theSlidesRoot);
                // Cache the new items and their names and such.
                CacheStateGraph(inReader);
            }

            inReader.MoveToFirstChild("Logic");
            inWriter.AppendChildren(*theSlidesRoot);
        }

        /*
#if defined _DEBUG && defined _WIN32
        {
                qt3dsdm::SDOMElement* theElem = inWriter.GetTopElement();
                {
                        qt3ds::foundation::CFileSeekableIOStream theWriter( "c:\\temp.xml",
FileWriteFlags() );
                        qt3dsdm::CDOMSerializer::Write( *theElem, theWriter );
                }
        }
#endif
        */
    }

}

void CUIPParserObjectRefHelper::CacheSceneGraph(IDOMReader &inReader, SGraphNode *inParent)
{
    IDOMReader::Scope __childScope(inReader);

    // build out the graph.
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        if (AreEqual(inReader.GetNarrowElementName(), "Alias")) {
            CacheSceneGraphNode(inReader, inParent);
            m_SceneGraphAliasList.push_back(inReader.GetScope());
        } else {
            SGraphNode &theNode(CacheSceneGraphNode(inReader, inParent));
            CacheSceneGraph(inReader, &theNode);
        }
    }
}

CUIPParserObjectRefHelper::SGraphNode &
CUIPParserObjectRefHelper::CacheSceneGraphNode(qt3dsdm::IDOMReader &inReader, SGraphNode *inParent)
{
    SGraphNode *theNode = new SGraphNode();
    const char8_t *theIdStr;
    inReader.Att("id", theIdStr);
    theNode->m_Id = m_MetaData.Register(theIdStr);
    theNode->m_Type = m_MetaData.Register(inReader.GetNarrowElementName());
    const char8_t *theClassStr;
    inReader.Att("class", theClassStr);
    theNode->m_Class = m_MetaData.Register(theClassStr);
    const char *theNameStr;
    theNode->m_ReaderContext = inReader.GetScope();

    if (inParent && inParent->m_Type == m_MetaData.Register("Layer")
        && theNode->m_Type == m_MetaData.Register("Image")) {
        TLightProbeIdToNameMap::iterator theFind = m_LayerImageIdToNameMap.find(theIdStr);
        if (theFind != m_LayerImageIdToNameMap.end())
            theNode->m_Name = m_MetaData.Register(theFind->second.c_str());
    } else if (!inReader.Att("name", theNameStr)) {
        Option<eastl::string> theNameStrOpt = m_MetaData.GetPropertyValueString(
            theNode->m_Type, m_MetaData.Register("name"), theNode->m_Class);
        if (theNameStrOpt.hasValue())
            theNode->m_Name = m_MetaData.Register(theNameStrOpt->c_str());
    } else
        theNode->m_Name = m_MetaData.Register(theNameStr);

    if (inParent) {
        theNode->m_Parent = inParent;
        inParent->m_Children.push_back(theNode);
    } else {
        m_RootNode = theNode;
    }

    m_GraphNodeMap[theNode->m_Id] = theNode;
    return *theNode;
}

void CUIPParserObjectRefHelper::CacheStateGraph(IDOMReader &inReader)
{
    IDOMReader::Scope __childScope(inReader);
    const char8_t *component;
    if (inReader.UnregisteredAtt("component", component)) {
        TGraphNodeMap::iterator iter = m_GraphNodeMap.find(m_MetaData.Register(component + 1));
        if (iter != m_GraphNodeMap.end()) {
            iter->second->m_MasterSlide = inReader.GetScope();
        }
    }

    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        if (AreEqual(inReader.GetNarrowElementName(), "State"))
            CacheStateGraph(inReader);
        else if (AreEqual(inReader.GetNarrowElementName(), "Add")
                 || AreEqual(inReader.GetNarrowElementName(), "Set")) {
            const char *theRef;
            const char *theName;
            if (inReader.Att("ref", theRef)) {
                theRef++; // remove the '#'
                SGraphNode *theNode = GetNode(theRef);
                if (theNode) {
                    // Overwrite the name. This assumes that "name" property is always linked.
                    if (inReader.Att("name", theName)) {
                        // Do not touch the name of an image instance that was renamed in the code
                        // below. i.e. an image that is referred to from a material.
                        if (!(theNode->m_Type == m_MetaData.Register("Image")
                              && m_ImageNamePropertyList.find(theNode->m_Name)
                                 != m_ImageNamePropertyList.end())) {
                            theNode->m_Name = m_MetaData.Register(theName);
                        }
                    }

                    // Special case for material. Image properties refer to instances.
                    // For example: Scene.Layer.Rectangle.Material.diffusemap.rotationuv ->
                    // diffusemap refers to Image instance
                    // So if we find image instance, overwrite the name of the image with the
                    // property name.
                    // This assumes that user can't rename image property
                    if (theNode->m_Type == m_MaterialStr && !theNode->m_Children.empty()) {
                        for (TImageNamePropertyList::iterator theProp =
                                 m_ImageNamePropertyList.begin();
                             theProp != m_ImageNamePropertyList.end(); ++theProp) {
                            const char *theImageInstance;
                            if (inReader.Att(theProp->c_str(), theImageInstance)) {
                                ++theImageInstance; // remove the '#'
                                SGraphNode *theImageNode = GetNode(theImageInstance);
                                QT3DS_ASSERT(theImageNode && theImageNode->m_Parent == theNode);
                                if (theImageNode)
                                    theImageNode->m_Name = *theProp;
                            }
                        }
                    }

                }
            }
        }
    }
}

void CUIPParserObjectRefHelper::CopySceneGraph(qt3dsdm::IDOMReader &inReader,
                                               qt3dsdm::IDOMWriter &inWriter,
                                               qt3dsdm::IDOMWriter &inSlideWriter,
                                               TStrToStrMap &inMap, const char *inAliasName,
                                               const char *inAliasId, SGraphNode &inParent)
{
    qt3dsdm::IDOMReader::Scope __graphScope(inReader);
    // Assume the element itself is already copied.
    // Run through attributes and duplicate them, but for any id attributes generate a new id.
    const char8_t *tempItem;
    if (!inReader.Att("id", tempItem)) {
        QT3DS_ASSERT(false);
        return;
    }
    SGraphNode *theParent = &inParent;
    eastl::string idGenerator(inAliasName);
    idGenerator.append("-");
    idGenerator.append(tempItem);
    while (m_GraphNodeMap.find(m_MetaData.Register(idGenerator.c_str())) != m_GraphNodeMap.end())
        idGenerator.append("_");
    inWriter.Att("id", idGenerator.c_str());
    TStrType srcId = m_MetaData.Register(tempItem);
    TStrType newId = m_MetaData.Register(idGenerator.c_str());
    inMap[srcId] = newId;
    SGraphNode *srcNode = m_GraphNodeMap.find(srcId)->second;
    SGraphNode *newNode = new SGraphNode();
    newNode->m_Class = srcNode->m_Class;
    newNode->m_Id = newId;
    newNode->m_Name = srcNode->m_Name;
    newNode->m_Type = srcNode->m_Type;
    newNode->m_Parent = &inParent;
    theParent = newNode;
    m_GraphNodeMap[newId] = newNode;
    inParent.m_Children.push_back(newNode);

    {
        qt3dsdm::IDOMReader::Scope __childrenScope(inReader);

        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            if (AreEqual(inReader.GetNarrowElementName(), "Alias"))
                continue;
            qt3dsdm::IDOMWriter::Scope __newNode(inWriter, inReader.GetNarrowElementName());
            CopySceneGraph(inReader, inWriter, inSlideWriter, inMap, inAliasName, inAliasId,
                           *theParent);
        }
    }
    TGraphNodeMap::iterator iter = m_GraphNodeMap.find(m_MetaData.Register(tempItem));
    if (iter == m_GraphNodeMap.end()) {
        QT3DS_ASSERT(false);
        return;
    }
    SGraphNode *targetNode = iter->second;

    for (eastl::pair<const char8_t *, const char8_t *> att = inReader.GetNarrowFirstAttribute();
         !isTrivial(att.first); att = inReader.GetNarrowNextAttribute()) {
        if (AreEqual(att.first, "id"))
            continue;
        Q3DStudio::ERuntimeDataModelDataType theDataType = m_MetaData.GetPropertyType(
            targetNode->m_Type, m_MetaData.Register(att.first), targetNode->m_Class);
        // Ensure we use new ids for this datatype.  This ensures that we go
        if (theDataType == ERuntimeDataModelDataTypeLong4) {
            TStrToStrMap::iterator aliasIdName = inMap.find(m_MetaData.Register(att.second));
            if (aliasIdName != inMap.end())
                inWriter.Att(att.first, aliasIdName->second.c_str());
            else {
                QT3DS_ASSERT(false);
            }
        } else
            inWriter.Att(att.first, att.second);
    }
    if (targetNode->m_MasterSlide) {
        inReader.SetScope(targetNode->m_MasterSlide);
        CopyStates(inReader, inSlideWriter, inMap, inAliasName, inAliasId);
    }
}

void CUIPParserObjectRefHelper::CopyStates(qt3dsdm::IDOMReader &inReader,
                                           qt3dsdm::IDOMWriter &inSlideWriter, TStrToStrMap &inMap,
                                           const char *inAliasName, const char *inAliasId)
{
    qt3dsdm::IDOMReader::Scope __stateScope(inReader);
    qt3dsdm::IDOMWriter::Scope stateScope(inSlideWriter, "State");
    CopyAttributes(inReader, inSlideWriter, inMap, inAliasName, inAliasId);
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        if (AreEqual(inReader.GetNarrowElementName(), "State"))
            CopyStates(inReader, inSlideWriter, inMap, inAliasName, inAliasId);
        else {
            CopyHierarchy(inReader, inSlideWriter, inMap, inAliasName, inAliasId);
        }
    }
}

eastl::pair<const char8_t *, const char8_t *> CUIPParserObjectRefHelper::ProcessAliasAttribute(
    const char8_t *inObjId, eastl::pair<const char8_t *, const char8_t *> inAttribute,
    eastl::string &ioStrBuilder, TStrToStrMap &inMap)
{
    inObjId = nonNull(inObjId);
    if (inObjId[0] == '#')
        ++inObjId;
    SGraphNode *theNode = GetNode(nonNull(inObjId));
    if (theNode == NULL)
        return inAttribute;
    const char8_t *propName = inAttribute.first;
    const char8_t *propValue = inAttribute.second;
    ERuntimeAdditionalMetaDataType thePropertyType = m_MetaData.GetAdditionalType(
        theNode->m_Type, m_MetaData.Register(propName), theNode->m_Class);
    if (thePropertyType == ERuntimeAdditionalMetaDataTypeImage
        || thePropertyType == ERuntimeAdditionalMetaDataTypeObjectRef) {
        propValue = nonNull(propValue);
        if (propValue[0] == '#')
            ++propValue;
        TStrToStrMap::iterator iter = inMap.find(m_MetaData.Register(propValue));
        if (iter != inMap.end()) {
            ioStrBuilder.assign("#");
            ioStrBuilder.append(iter->second);
            propValue = ioStrBuilder.c_str();
        } else
            propValue = inAttribute.second;
    }
    return eastl::make_pair(propName, propValue);
}

void CUIPParserObjectRefHelper::CopyAttributes(qt3dsdm::IDOMReader &inReader,
                                               qt3dsdm::IDOMWriter &inSlideWriter,
                                               TStrToStrMap &inMap, const char *inAliasName,
                                               const char *inAliasId)
{
    eastl::string builder;
    const char8_t *elemId = "";
    for (eastl::pair<const char8_t *, const char8_t *> att = inReader.GetNarrowFirstAttribute();
         !isTrivial(att.first); att = inReader.GetNarrowNextAttribute()) {
        if (AreEqual(att.first, "component")) {
            TStrToStrMap::iterator iter = inMap.find(m_MetaData.Register(att.second + 1));
            builder.assign("#");
            if (iter != inMap.end()) {
                builder.append(iter->second);
            } else {
                builder.append(inAliasId);
            }
            inSlideWriter.Att(att.first, builder.c_str());
        } else if (AreEqual(att.first, "id")) {
            builder.assign(inAliasName);
            builder.append("-");
            builder.append(att.second);
            // cannot be
            while (m_SlideIdSet.find(m_MetaData.Register(builder.c_str())) != m_SlideIdSet.end())
                builder.append("_");
            inSlideWriter.Att(att.first, builder.c_str());
            m_SlideIdSet.insert(m_MetaData.Register(builder.c_str()));
            inMap[m_MetaData.Register(att.second)] = m_MetaData.Register(builder.c_str());
        } else if (AreEqual(att.first, "ref")) {
            const char8_t *refItem = att.second;
            if (!isTrivial(refItem)) {
                ++refItem;
                elemId = refItem;
                TStrToStrMap::iterator iter = inMap.find(m_MetaData.Register(refItem));
                if (iter != inMap.end()) {
                    builder.assign("#");
                    builder.append(iter->second.c_str());
                    inSlideWriter.Att("ref", builder.c_str());
                } else {
                    QT3DS_ASSERT(false);
                }
            }
        } else {
            att = ProcessAliasAttribute(elemId, att, builder, inMap);
            inSlideWriter.Att(att.first, att.second);
        }
    }
}

void CUIPParserObjectRefHelper::CopyHierarchy(qt3dsdm::IDOMReader &inReader,
                                              qt3dsdm::IDOMWriter &inSlideWriter, TStrToStrMap &inMap,
                                              const char *inAliasName, const char *inAliasId)
{
    qt3dsdm::IDOMReader::Scope __commandScope(inReader);
    qt3dsdm::IDOMWriter::Scope __writerScope(inSlideWriter, inReader.GetNarrowElementName());
    CopyAttributes(inReader, inSlideWriter, inMap, inAliasName, inAliasId);
    const char8_t *childData;
    if (inReader.Value(childData)) {
        inSlideWriter.Value(childData);
    }
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling())
        CopyHierarchy(inReader, inSlideWriter, inMap, inAliasName, inAliasId);
}

void CUIPParserObjectRefHelper::CopyStateCommands(qt3dsdm::IDOMReader &inReader,
                                                  qt3dsdm::IDOMWriter &inWriter,
                                                  TStrToStrMap &oldToNewIdMap,
                                                  const char *inAliasName, const char *inOldId,
                                                  const char *inNewId)
{
    qt3dsdm::IDOMReader::Scope __commandScope(inReader);
    qt3dsdm::SDOMElement *theSlidesRoot = NULL;
    eastl::pair<std::shared_ptr<qt3dsdm::IDOMWriter>, std::shared_ptr<qt3dsdm::IDOMReader>>
        slideWriterReaderPair;
    std::shared_ptr<qt3dsdm::IDOMWriter> commandWriter;
    {
        qt3dsdm::IDOMReader::Scope __loopScope(inReader);
        eastl::vector<eastl::pair<eastl::string, eastl::string>> copiedAttributes;
        void *destCommand = NULL;
        eastl::string strBuilder;
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            qt3dsdm::IDOMReader::Scope childScope(inReader);
            if (AreEqual(inReader.GetNarrowElementName(), "State"))
                CopyStateCommands(inReader, inWriter, oldToNewIdMap, inAliasName, inOldId, inNewId);
            else {
                const char8_t *refItem = "";
                if (inReader.Att("ref", refItem) && !isTrivial(refItem)) {
                    ++refItem;
                    TStrToStrMap::iterator iter = oldToNewIdMap.find(m_MetaData.Register(refItem));
                    if (iter != oldToNewIdMap.end()) {
                        if (theSlidesRoot == NULL) {
                            std::shared_ptr<qt3dsdm::IDOMFactory> theFactory =
                                inWriter.GetFactory();
                            std::shared_ptr<qt3dsdm::IStringTable> theStrTable =
                                inReader.GetStringTable();
                            theSlidesRoot = theFactory->NextElement("AliasSlides");
                            slideWriterReaderPair = qt3dsdm::IDOMWriter::CreateDOMWriter(
                                theFactory, *theSlidesRoot, theStrTable);
                            commandWriter = slideWriterReaderPair.first;
                        }
                        qt3dsdm::IDOMWriter::Scope elemScope(*commandWriter,
                                                           inReader.GetNarrowElementName());
                        for (eastl::pair<const char8_t *, const char8_t *> att =
                                 inReader.GetNarrowFirstAttribute();
                             !isTrivial(att.first); att = inReader.GetNarrowNextAttribute()) {
                            if (AreEqual(att.first, "ref")) {
                                strBuilder.assign("#");
                                strBuilder.append(iter->second);
                                commandWriter->Att("ref", strBuilder.c_str());
                            } else {

                                att =
                                    ProcessAliasAttribute(refItem, att, strBuilder, oldToNewIdMap);
                                commandWriter->Att(att.first, att.second);
                            }
                        }
                        for (bool commandChild = inReader.MoveToFirstChild(); commandChild;
                             commandChild = inReader.MoveToNextSibling()) {
                            qt3dsdm::IDOMReader::Scope commandChildScope(inReader);
                            CopyHierarchy(inReader, *commandWriter, oldToNewIdMap, inAliasName, "");
                        }
                    } else if (AreEqual(refItem, inOldId)) {
                        StoreTopLevelAliasAttributes(inReader, copiedAttributes);
                    } else if (AreEqual(refItem, inNewId))
                        destCommand = inReader.GetScope();
                }
            }
        }
        if (copiedAttributes.size() && destCommand) {
            inReader.SetScope(destCommand);
            for (QT3DSU32 idx = 0, end = copiedAttributes.size(); idx < end; ++idx) {
                eastl::pair<eastl::string, eastl::string> &theItem(copiedAttributes[idx]);
                inWriter.Att(theItem.first.c_str(), theItem.second.c_str());
            }
        }
    }
    if (commandWriter)
        inWriter.AppendChildren(*commandWriter->GetTopElement());
}

CUIPParserObjectRefHelper::SGraphNode *CUIPParserObjectRefHelper::GetNode(const char *inId)
{
    if (IsTrivial(inId))
        return NULL;
    if (inId[0] == '#')
        ++inId;
    TGraphNodeMap::iterator theIter = m_GraphNodeMap.find(m_MetaData.Register(inId));
    if (theIter != m_GraphNodeMap.end())
        return theIter->second;
    return NULL;
}

CUIPParserObjectRefHelper::SGraphNode *CUIPParserObjectRefHelper::GetNode(eastl::string inId)
{
    return GetNode(inId.c_str());
}

CUIPParserObjectRefHelper::TStrType CUIPParserObjectRefHelper::GetName(const eastl::string &inId)
{
    SGraphNode *theNode = GetNode(inId.c_str());
    if (theNode)
        return theNode->m_Name;
    QT3DS_ASSERT(false);
    return m_MetaData.Register(inId.c_str());
}

CUIPParserObjectRefHelper::TStrType CUIPParserObjectRefHelper::GetType(const eastl::string &inId)
{
    SGraphNode *theNode = GetNode(inId.c_str());
    if (theNode)
        return theNode->m_Type;
    QT3DS_ASSERT(false);
    return m_MetaData.Register(inId.c_str());
}

CUIPParserObjectRefHelper::TStrType CUIPParserObjectRefHelper::GetClass(const eastl::string &inId)
{
    SGraphNode *theNode = GetNode(inId.c_str());
    if (theNode)
        return theNode->m_Class;
    QT3DS_ASSERT(false);
    return m_MetaData.Register(inId.c_str());
}

CUIPParserObjectRefHelper::TStrType
CUIPParserObjectRefHelper::ParseObjectRefId(const eastl::string &inObjectPath,
                                            const char8_t *inOwnerId)
{
    if (inObjectPath.empty()) {
        QT3DS_ASSERT(false);
        return TStrType();
    } else if (inObjectPath[0] == '#') {
        // Absolute path
        return m_MetaData.Register(inObjectPath.substr(1).c_str());
    } else {
        // Relative path
        SGraphNode *theNode = GetNode(inOwnerId);

        // Split the string based on PATH_DELIMITER
        eastl::string theStr = inObjectPath;
        eastl::string theCurrentPart;
        eastl::string::size_type thePos;
        while (theNode) {
            thePos = theStr.find(PATH_DELIMITER);
            theCurrentPart = theStr.substr(0, thePos);

            if (theCurrentPart == "parent") {
                theNode = theNode->m_Parent;
            } else if (theCurrentPart == "this") {
                // Do nothing because theNode points to itself
            } else if (theCurrentPart == "Scene") {
                theNode = m_RootNode;
            } else {
                SGraphNode *theFoundChild = NULL;
                for (QT3DSU32 childIdx = 0;
                     childIdx < theNode->m_Children.size() && theFoundChild == NULL; ++childIdx) {
                    if (AreEqual(theNode->m_Children[childIdx]->m_Name, theCurrentPart.c_str()))
                        theFoundChild = theNode->m_Children[childIdx];
                }
                theNode = theFoundChild;
            }

            if (thePos == eastl::string::npos)
                break;

            // Move on to the next token
            theStr = theStr.substr(thePos + 1);
        }

        if (theNode)
            return theNode->m_Id;

        return TStrType();
    }
}

eastl::string CUIPParserObjectRefHelper::BuildReferenceString(eastl::string inObjectPath)
{
    if (inObjectPath.empty()) {
        // QT3DS_ASSERT( false );
        return "";
    } else if (inObjectPath[0] == '#') {
        // Absolute path
        return BuildAbsoluteReferenceString(inObjectPath.substr(1));
    } else {
        // Relative path
        return inObjectPath;
    }
}

eastl::string CUIPParserObjectRefHelper::BuildAbsoluteReferenceString(eastl::string inId)
{
    return BuildAbsoluteReferenceString(GetNode(inId));
}

eastl::string CUIPParserObjectRefHelper::BuildAbsoluteReferenceString(SGraphNode *inNode)
{
    if (inNode == NULL)
        return "";

    eastl::string theNameStart;
    eastl::string theNameEnd(inNode->m_Name);

    SGraphNode *theParent = inNode->m_Parent;
    if (theParent) {
        theNameStart.assign(BuildAbsoluteReferenceString(theParent));
        theNameStart.append(".");
    }
    theNameStart += theNameEnd;
    return theNameStart;
}

void CUIPParserObjectRefHelper::MarkAllReferencedAttributes(
    eastl::string inId, const eastl::vector<eastl::string> &inReferences,
    qt3dsdm::IDOMReader &inReader, SParseElementManager &outIdAttributesMap)
{
    IDOMReader::Scope __scope(inReader);
    eastl::string theReferencesTokenizer;
    eastl::string theCurrentReference;
    eastl::string theReferenceTokenizer;
    eastl::string theCurrentString;
    eastl::string::size_type startPos = 0;
    eastl::string::size_type endPos = 0;
    eastl::string::size_type theReferencesTokenizerPos = 0;
    eastl::string::size_type theReferenceTokenizerPos = 0;
    SGraphNode *theBaseInstance = GetNode(inId);
    SGraphNode *theCurrentInstance = theBaseInstance;
    eastl::vector<SGraphNode *> theInstanceList;

    for (QT3DSU32 theRefIdx = 0, theRefEnd = inReferences.size(); theRefIdx < theRefEnd; ++theRefIdx) {
        // Split the string based on PATH_LINE_END
        theReferencesTokenizer = inReferences[theRefIdx];
        theReferencesTokenizerPos = 0;
        while (theReferencesTokenizerPos != eastl::string::npos
               && !theReferencesTokenizer.empty()) {
            theCurrentInstance = theBaseInstance;
            theReferencesTokenizerPos = theReferencesTokenizer.find(PATH_LINE_END);
            theCurrentReference = theReferencesTokenizer.substr(0, theReferencesTokenizerPos);

            // Move to the next token
            theReferencesTokenizer = theReferencesTokenizer.substr(theReferencesTokenizerPos + 1);

            // trim whitespace from the beginning and the end of the string
            startPos = theCurrentReference.find_first_not_of("\n\r\t ");
            endPos = theCurrentReference.find_last_not_of("\n\r\t ");
            if (startPos != eastl::string::npos)
                theCurrentReference = theCurrentReference.substr(startPos, endPos - startPos + 1);

            // Split the string based on PATH_DELIMITER
            theReferenceTokenizer = theCurrentReference;
            theReferenceTokenizerPos = 0;
            theInstanceList.clear();
            while (theReferenceTokenizerPos != eastl::string::npos
                   && !theReferenceTokenizer.empty()) {
                theReferenceTokenizerPos = theReferenceTokenizer.find(PATH_DELIMITER);
                theCurrentString = theReferenceTokenizer.substr(0, theReferenceTokenizerPos);

                // Move to the next token
                theReferenceTokenizer = theReferenceTokenizer.substr(theReferenceTokenizerPos + 1);

                if (theReferenceTokenizerPos != eastl::string::npos) {
                    theCurrentInstance = GetReferenceNode(theCurrentInstance, theCurrentString,
                                                          theInstanceList, inReader);
                } else {
                    if (theInstanceList.size() == 0 && theCurrentInstance)
                        theInstanceList.push_back(theCurrentInstance);
                    if (!MarkAttributeAsReferenced(theBaseInstance, theInstanceList,
                                                   theCurrentString, inReader,
                                                   outIdAttributesMap)) {
                        qCCritical(qt3ds::INVALID_OPERATION)
                                << "Unable to parse reference: "
                                << theBaseInstance->m_Id.c_str() << " : "
                                << theCurrentReference.c_str();
                    }
                }
            }
        }
    }
}

//==============================================================================
/**
 * Helper method to find the VAsset via the name which is the child of inAsset.
 */
CUIPParserObjectRefHelper::SGraphNode *
CUIPParserObjectRefHelper::GetReferenceNode(SGraphNode *inInstance, eastl::string &inAssetName,
                                            eastl::vector<SGraphNode *> &outInstanceList,
                                            qt3dsdm::IDOMReader &inReader)
{
    if (!inInstance)
        return NULL;

    SGraphNode *theReturnInstance = NULL;

    if (inAssetName == "Scene") {
        theReturnInstance = m_RootNode;
    } else if (inAssetName == "parent") {
        theReturnInstance = inInstance->m_Parent;
    } else if (inAssetName == "children") {
        outInstanceList.insert(outInstanceList.end(), inInstance->m_Children.begin(),
                               inInstance->m_Children.end());
    } else if (inAssetName == "descendants") {
        for (SGraphNode::TGraphNodeList::iterator theChildren = inInstance->m_Children.begin();
             theChildren != inInstance->m_Children.end(); ++theChildren) {
            outInstanceList.push_back(*theChildren);
            GetReferenceNode(*theChildren, inAssetName, outInstanceList, inReader);
        }
    } else if (inAssetName[0] == '[' && inAssetName[inAssetName.length() - 1] == ']') {
        // For example: [targetElement].position
        // targetElement should be an Object picker specifying which element to preserve.
        eastl::string theAssetName = inAssetName.substr(1, inAssetName.length() - 2);

        // Check if the target element property exists and get the property value
        eastl::string theValue;
        if (!inReader.Att(theAssetName.c_str(), theValue)) {
            // Try to find the parent context and get the element name from there.
            if (AreEqual(inReader.GetElementName(), L"Set")) {
                IDOMReader::Scope __setScope(inReader);
                const char8_t *refValue = "";
                inReader.Att("ref", refValue);
                inReader.Leave();
                inReader.Leave();
                for (bool success = inReader.MoveToFirstChild("Add"); success;
                     success = inReader.MoveToNextSibling("Add")) {
                    const char8_t *newRef;
                    inReader.Att("ref", newRef);
                    if (AreEqual(newRef, refValue)) {
                        inReader.Att(theAssetName.c_str(), theValue);
                        break;
                    }
                }
            }
            if (theValue.empty()) {
                Option<TRuntimeMetaDataStrType> theRef = m_MetaData.GetPropertyValueObjectRef(
                    inInstance->m_Type, m_MetaData.Register(theAssetName.c_str()),
                    inInstance->m_Class);
                if (theRef.hasValue())
                    theValue = *theRef;
            }
        }
        if (!IsTrivial(theValue.c_str())) {
            // Get the target element id
            eastl::string theTargetElement =
                ParseObjectRefId(theValue, inInstance->m_Id.c_str()).c_str();
            if (theTargetElement != "") {
                // Get the corresponding instance
                theReturnInstance = GetNode(theTargetElement);
            }
        }
    } else {
        // Get the child with the specified name.
        // Note that name is not unique and so this will only return the first child that matches
        // the name
        for (SGraphNode::TGraphNodeList::iterator theChildren = inInstance->m_Children.begin();
             theChildren != inInstance->m_Children.end(); ++theChildren) {
            if (AreEqual((*theChildren)->m_Name.c_str(), inAssetName.c_str())) {
                theReturnInstance = *theChildren;
                break;
            }
        }
    }
    return theReturnInstance;
}

bool CUIPParserObjectRefHelper::MarkAttributeAsReferenced(
    SGraphNode *inBaseInstance, eastl::vector<SGraphNode *> &inInstanceList,
    eastl::string &inAttributeName, qt3dsdm::IDOMReader &inReader,
    SParseElementManager &outIdAttributesMap)
{
    bool theRet(false);
    eastl::vector<SGraphNode *>::iterator theIter = inInstanceList.begin();
    eastl::vector<SGraphNode *>::iterator theEnd = inInstanceList.end();
    for (; theIter != theEnd; ++theIter) {
        SGraphNode *theCurrentInstance = *theIter;

        if (inAttributeName == "all") {
            eastl::vector<TRuntimeMetaDataStrType> theProperties;
            m_MetaData.GetInstanceProperties(theCurrentInstance->m_Type.c_str(),
                                             theCurrentInstance->m_Class.c_str(), theProperties,
                                             true);

            if (theProperties.size() > 0) {
                SElementData *theData =
                    outIdAttributesMap.FindElementData(theCurrentInstance->m_Id.c_str());
                if (theData) {
                    for (QT3DSU32 theIndex = 0; theIndex < theProperties.size(); ++theIndex)
                        outIdAttributesMap.MarkAttributeAsReferenced(
                            *theData, theProperties[theIndex].c_str());
                }
                theRet = true;
            }
        } else {
            eastl::string theAttributeName = inAttributeName;
            if (inAttributeName[0] == '[' && inAttributeName[inAttributeName.length() - 1] == ']') {
                // For example: parent.[targetProp]
                // targetProp should be a string specifying which property to preserve
                theAttributeName = theAttributeName.substr(1, theAttributeName.length() - 2);

                // Get the targetProp property value from the uip file
                const char *theValue;
                if (inReader.Att(theAttributeName.c_str(), theValue)) {
                    theAttributeName = theValue;
                } else {
                    // Query metadata value of base instance
                    theAttributeName = m_MetaData.GetPropertyValueString(
                        inBaseInstance->m_Type, m_MetaData.Register(theAttributeName.c_str()),
                        inBaseInstance->m_Class);
                }
            }

            // Check if the property exists
            if (m_MetaData.IsPropertyExist(theCurrentInstance->m_Type,
                                           m_MetaData.Register(theAttributeName.c_str()),
                                           theCurrentInstance->m_Class)) {
                MarkPreserveFlag(theCurrentInstance, theAttributeName, outIdAttributesMap);
                theRet = true;
            } else {
                // check whether the attribute name has ".", strip those after that and try again
                eastl::string::size_type theIndex = theAttributeName.rfind('.');
                if (theIndex != eastl::string::npos) {
                    theAttributeName = theAttributeName.substr(0, theIndex);
                    if (m_MetaData.IsPropertyExist(theCurrentInstance->m_Type,
                                                   m_MetaData.Register(theAttributeName.c_str()),
                                                   theCurrentInstance->m_Class)) {
                        MarkPreserveFlag(theCurrentInstance, theAttributeName, outIdAttributesMap);
                        theRet = true;
                    }
                }
            }
        }
    }

    return theRet;
}

void CUIPParserObjectRefHelper::MarkPreserveFlag(SGraphNode *inInstance, eastl::string inProperty,
                                                 SParseElementManager &outIdAttributesMap)
{
    outIdAttributesMap.MarkAttributeAsReferenced(inInstance->m_Id.c_str(), inProperty.c_str());
}

void CUIPParserObjectRefHelper::BuildImageNamePropertyList()
{
    // Special case for material
    // Material's Image properties (such as diffuse map, etc) point to Image instances
    eastl::vector<TRuntimeMetaDataStrType> theProperties;
    m_MetaData.GetInstanceProperties("Material", NULL, theProperties, true);

    size_t thePropertyCount = theProperties.size();
    for (QT3DSU32 thePropertyIndex = 0; thePropertyIndex < thePropertyCount; ++thePropertyIndex) {
        eastl::string theProperty = theProperties[thePropertyIndex];
        ERuntimeAdditionalMetaDataType theAdditionalMetaDataType =
            m_MetaData.GetAdditionalType(m_MaterialStr, m_MetaData.Register(theProperty.c_str()));

        if (theAdditionalMetaDataType == ERuntimeAdditionalMetaDataTypeImage) {
            m_ImageNamePropertyList.insert(m_MetaData.Register(theProperty.c_str()));
        }
    }
}
}
