/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QT3DS_FOUNDATION_INDEXABLE_LINKED_LIST_H
#define QT3DS_FOUNDATION_INDEXABLE_LINKED_LIST_H
#include "foundation/Qt3DSPool.h"

namespace qt3ds {
namespace foundation {
    // Set of helper functions for manipulation of lists that are
    // somewhat indexable but also amenable to using a pool structure.
    template <typename TNodeType, typename TObjType, QT3DSU32 TObjCount>
    struct IndexableLinkedList
    {
        typedef TNodeType SNode;

        typedef Pool<SNode, ForwardingAllocator> TPoolType;

        static TObjType &Create(SNode *&inInitialNode, QT3DSU32 &ioCurrentCount, TPoolType &ioPool)
        {
            QT3DSU32 idx = ioCurrentCount;
            ++ioCurrentCount;
            QT3DSU32 numGroups = (ioCurrentCount + TObjCount - 1) / TObjCount;
            QT3DSU32 localIdx = idx % TObjCount;

            SNode *theCurrentNode = inInitialNode;
            for (QT3DSU32 idx = 0, end = numGroups; idx < end; ++idx) {
                if (idx == 0) {
                    if (theCurrentNode == NULL)
                        inInitialNode = ioPool.construct(__FILE__, __LINE__);

                    theCurrentNode = inInitialNode;
                } else {
                    if (theCurrentNode->m_NextNode == NULL)
                        theCurrentNode->m_NextNode = ioPool.construct(__FILE__, __LINE__);
                    theCurrentNode = theCurrentNode->m_NextNode;
                }
            }
            return theCurrentNode->m_Data[localIdx];
        }

        static void CreateAll(SNode *&inInitialNode, QT3DSU32 inCount, TPoolType &ioPool)
        {
            QT3DSU32 numGroups = (inCount + TObjCount - 1) / TObjCount;
            SNode *lastNode = NULL;
            for (QT3DSU32 idx = 0, end = numGroups; idx < end; ++idx) {
                SNode *nextNode = ioPool.construct(__FILE__, __LINE__);
                if (idx == 0)
                    inInitialNode = nextNode;
                else
                    lastNode->m_NextNode = nextNode;
                lastNode = nextNode;
            }
        }

        static void DeleteList(SNode *&inInitialNode, TPoolType &ioPool)
        {
            SNode *theNode = inInitialNode;
            inInitialNode = NULL;
            while (theNode) {
                SNode *theNext = theNode->m_NextNode;
                theNode->m_NextNode = NULL;
                ioPool.deallocate(theNode);
                theNode = theNext;
            }
        }

        // Performs no checking, so you need to have already allocated what you need.
        static const TObjType &GetObjAtIdx(const SNode *inInitialNode, QT3DSU32 inIdx)
        {
            QT3DSU32 groupIdx = inIdx / TObjCount;
            QT3DSU32 localIdx = inIdx % TObjCount;
            for (QT3DSU32 idx = 0; idx < groupIdx; ++idx)
                inInitialNode = inInitialNode->m_NextNode;
            return inInitialNode->m_Data[inIdx];
        }

        static TObjType &GetObjAtIdx(SNode *inInitialNode, QT3DSU32 inIdx)
        {
            QT3DSU32 groupIdx = inIdx / TObjCount;
            QT3DSU32 localIdx = inIdx % TObjCount;
            for (QT3DSU32 idx = 0; idx < groupIdx; ++idx)
                inInitialNode = inInitialNode->m_NextNode;
            return inInitialNode->m_Data[localIdx];
        }

        struct SIteratorType
        {
            QT3DSU32 m_Idx;
            QT3DSU32 m_Count;
            SNode *m_Node;

            SIteratorType()
                : m_Idx(0)
                , m_Count(0)
                , m_Node(NULL)
            {
            }
            SIteratorType(const SIteratorType &iter)
                : m_Idx(iter.m_Idx)
                , m_Count(iter.m_Count)
                , m_Node(iter.m_Node)
            {
            }
            SIteratorType(SNode *inNode, QT3DSU32 inCount, QT3DSU32 inIdx)
                : m_Idx(inIdx)
                , m_Count(inCount)
                , m_Node(inNode)
            {
            }

            bool operator==(const SIteratorType &iter) const { return m_Idx == iter.m_Idx; }

            bool operator!=(const SIteratorType &iter) const { return m_Idx != iter.m_Idx; }

            TObjType &operator*()
            {
                QT3DSU32 localIdx = m_Idx % TObjCount;
                return m_Node->m_Data[localIdx];
            }

            SIteratorType &operator++()
            {
                ++m_Idx;
                if ((m_Idx % TObjCount) == 0)
                    m_Node = m_Node->m_NextNode;

                return *this;
            }
        };

        struct SConstIteratorType
        {
            QT3DSU32 m_Idx;
            QT3DSU32 m_Count;
            const SNode *m_Node;

            SConstIteratorType()
                : m_Idx(0)
                , m_Count(0)
                , m_Node(NULL)
            {
            }
            SConstIteratorType(const SIteratorType &iter)
                : m_Idx(iter.m_Idx)
                , m_Count(iter.m_Count)
                , m_Node(iter.m_Node)
            {
            }
            SConstIteratorType(const SConstIteratorType &iter)
                : m_Idx(iter.m_Idx)
                , m_Count(iter.m_Count)
                , m_Node(iter.m_Node)
            {
            }
            SConstIteratorType(const SNode *inNode, QT3DSU32 inCount, QT3DSU32 inIdx)
                : m_Idx(inIdx)
                , m_Count(inCount)
                , m_Node(inNode)
            {
            }

            bool operator==(const SConstIteratorType &iter) const { return m_Idx == iter.m_Idx; }

            bool operator!=(const SConstIteratorType &iter) const { return m_Idx != iter.m_Idx; }

            const TObjType &operator*()
            {
                QT3DSU32 localIdx = m_Idx % TObjCount;
                return m_Node->m_Data[localIdx];
            }

            SConstIteratorType &operator++()
            {
                ++m_Idx;
                if ((m_Idx % TObjCount) == 0)
                    m_Node = m_Node->m_NextNode;

                return *this;
            }
        };

        typedef SIteratorType iterator;
        typedef SConstIteratorType const_iterator;

        static iterator begin(SNode *inInitialNode, QT3DSU32 inItemCount)
        {
            return SIteratorType(inInitialNode, inItemCount, 0);
        }

        static iterator end(SNode * /*inInitialNode*/, QT3DSU32 inItemCount)
        {
            return SIteratorType(NULL, inItemCount, inItemCount);
        }

        static const_iterator begin(const SNode *inInitialNode, QT3DSU32 inItemCount)
        {
            return SConstIteratorType(inInitialNode, inItemCount, 0);
        }

        static const_iterator end(const SNode * /*inInitialNode*/, QT3DSU32 inItemCount)
        {
            return SConstIteratorType(NULL, inItemCount, inItemCount);
        }

        struct SNodeIteratorType
        {
            SNode *m_Node;

            SNodeIteratorType()
                : m_Node(NULL)
            {
            }
            SNodeIteratorType(const SNodeIteratorType &iter)
                : m_Node(iter.m_Node)
            {
            }
            SNodeIteratorType(SNode *inNode)
                : m_Node(inNode)
            {
            }

            bool operator==(const SNodeIteratorType &iter) const { return m_Node == iter.m_Node; }

            bool operator!=(const SNodeIteratorType &iter) const { return m_Node != iter.m_Node; }

            TNodeType &operator*() { return *m_Node; }

            SNodeIteratorType &operator++()
            {
                if (m_Node)
                    m_Node = m_Node->m_NextNode;

                return *this;
            }
        };

        typedef SNodeIteratorType node_iterator;
        static node_iterator node_begin(SNode *inFirstNode) { return node_iterator(inFirstNode); }
        static node_iterator node_end(SNode * /*inFirstNode*/) { return node_iterator(NULL); }

        // Iterates through the number of nodes required for a given count of objects
        struct SCountIteratorType
        {
            QT3DSU32 m_Idx;
            QT3DSU32 m_Count;

            SCountIteratorType()
                : m_Idx(0)
                , m_Count(0)
            {
            }
            SCountIteratorType(const SCountIteratorType &iter)
                : m_Idx(iter.m_Idx)
                , m_Count(iter.m_Count)
            {
            }
            SCountIteratorType(QT3DSU32 idx, QT3DSU32 count)
                : m_Idx(idx)
                , m_Count(count)
            {
            }

            bool operator==(const SCountIteratorType &iter) const { return m_Idx == iter.m_Idx; }

            bool operator!=(const SCountIteratorType &iter) const { return m_Idx != iter.m_Idx; }

            SCountIteratorType &operator++()
            {
                m_Idx = NVMin(m_Idx + TObjCount, m_Count);
                return *this;
            }
        };

        typedef SCountIteratorType count_iterator;
        static count_iterator count_begin(QT3DSU32 count) { return SCountIteratorType(0, count); }
        static count_iterator count_end(QT3DSU32 count) { return SCountIteratorType(count, count); }
    };
}
}

#endif