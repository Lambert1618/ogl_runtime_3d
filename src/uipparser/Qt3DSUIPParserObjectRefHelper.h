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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSUIPParser.h"
#include "Qt3DSUIPParserImpl.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	@class	CUIPParserObjectRefHelper
 *	@brief	Class for parsing UIP file - Object Reference Helper
 */
class CUIPParserObjectRefHelper
{
    typedef qt3ds::foundation::CRegisteredString TStrType;

public:
    typedef eastl::hash_map<TStrType, TStrType> TStrToStrMap;

    /// Tree structure to cache scene graph information. This is used to resolve relative object
    /// reference
    struct SGraphNode
    {
        typedef eastl::vector<SGraphNode *> TGraphNodeList;

        TStrType m_Id; // node id
        TStrType m_Name; // name of the node. the name can be default from metadata, specified in
                         // graph section, or specified in logic section.
        TStrType m_Type; // for example: Layer, Camera, Model
        TStrType
            m_Class; // class id. usually used by Behavior or objects that have custom properties.
        void *m_ReaderContext; // Context to warp back to this node.
        void *m_MasterSlide;
        SGraphNode *m_Parent;
        TGraphNodeList m_Children;
        SGraphNode()
            : m_ReaderContext(NULL)
            , m_MasterSlide(NULL)
            , m_Parent(NULL)
        {
        }
    };
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    typedef eastl::hash_set<TStrType> TImageNamePropertyList;
    IRuntimeMetaData &m_MetaData; ///< Reference to Metadata
    typedef eastl::hash_map<TStrType, SGraphNode *> TGraphNodeMap;
    SGraphNode *m_RootNode;
    TGraphNodeMap m_GraphNodeMap;
    TImageNamePropertyList m_SlideIdSet;

    // List of Image property names, for example diffusemap, normalmap, etc
    // This is to handle material and images. Material's Image properties point to instances.
    TImageNamePropertyList m_ImageNamePropertyList;
    TStrType m_MaterialStr;
    eastl::vector<void *> m_SceneGraphAliasList;

    typedef eastl::pair<eastl::string, eastl::string> TLightProbeAndNamePair;
    typedef eastl::map<eastl::string, eastl::string> TLightProbeIdToNameMap;
    TLightProbeIdToNameMap m_LayerImageIdToNameMap;
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CUIPParserObjectRefHelper(IRuntimeMetaData &inMetaData);
    virtual ~CUIPParserObjectRefHelper();

    void CacheGraph(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inWriter);
    SGraphNode *GetNode(const char *inId);
    SGraphNode *GetNode(eastl::string inId);
    TStrType GetName(const eastl::string &inId); // return the node name given the node id
    TStrType GetType(const eastl::string &inId); // return the node type given the node id
    TStrType GetClass(const eastl::string &inId); // return the node class given the node id

    TStrType ParseObjectRefId(const eastl::string &inObjectPath, const char8_t *inOwnerId);
    eastl::string BuildReferenceString(eastl::string inObjectPath);
    void MarkAllReferencedAttributes(eastl::string inId,
                                     const eastl::vector<eastl::string> &inReferences,
                                     qt3dsdm::IDOMReader &inReader,
                                     SParseElementManager &outIdAttributesMap);

private:
    void CacheSceneGraph(qt3dsdm::IDOMReader &inReader, SGraphNode *inParent = NULL);
    SGraphNode &CacheSceneGraphNode(qt3dsdm::IDOMReader &inReader, SGraphNode *inParent = NULL);
    void CacheStateGraph(qt3dsdm::IDOMReader &inReader);

    void CopySceneGraph(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inWriter,
                        qt3dsdm::IDOMWriter &inSlideWriter, TStrToStrMap &inMap,
                        const char *inAliasName, const char *inAliasId, SGraphNode &inParent);
    void CopyStates(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inSlideWriter,
                    TStrToStrMap &inMap, const char *inAliasName, const char *inAliasId);
    void CopyAttributes(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inSlideWriter,
                        TStrToStrMap &inMap, const char *inAliasName, const char *inAliasId);
    void CopyHierarchy(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inSlideWriter,
                       TStrToStrMap &inMap, const char *inAliasName, const char *inAliasId);
    void CopyStateCommands(qt3dsdm::IDOMReader &inReader, qt3dsdm::IDOMWriter &inWriter,
                           TStrToStrMap &oldToNewIdMap, const char *inAliasName,
                           const char *inOldId, const char *inNewId);
    // Helper method for Preseve Attributes
    SGraphNode *GetReferenceNode(SGraphNode *inInstance, eastl::string &inAssetName,
                                 eastl::vector<SGraphNode *> &outInstanceList,
                                 qt3dsdm::IDOMReader &inReader);
    bool MarkAttributeAsReferenced(SGraphNode *inBaseInstance,
                                   eastl::vector<SGraphNode *> &inInstanceList,
                                   eastl::string &inAttributeName, qt3dsdm::IDOMReader &inReader,
                                   SParseElementManager &outIdAttributesMap);
    void MarkPreserveFlag(SGraphNode *inInstance, eastl::string inProperty,
                          SParseElementManager &outIdAttributesMap);
    eastl::pair<const char8_t *, const char8_t *>
    ProcessAliasAttribute(const char8_t *inObjId,
                          eastl::pair<const char8_t *, const char8_t *> inAttribute,
                          eastl::string &ioStrBuilder, TStrToStrMap &inMap);

    eastl::string BuildAbsoluteReferenceString(eastl::string inId);
    eastl::string BuildAbsoluteReferenceString(SGraphNode *inNode);

    // Special case for material and images. Material's Image properties point to instances.
    void BuildImageNamePropertyList();
};

} // namespace Q3DStudio
