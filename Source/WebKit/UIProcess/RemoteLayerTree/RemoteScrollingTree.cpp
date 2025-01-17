/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RemoteScrollingTree.h"

#if ENABLE(UI_SIDE_COMPOSITING)

#include "RemoteLayerTreeHost.h"
#include "RemoteScrollingCoordinatorProxy.h"
#include <WebCore/ScrollingTreeFixedNodeCocoa.h>
#include <WebCore/ScrollingTreeFrameHostingNode.h>
#include <WebCore/ScrollingTreeOverflowScrollProxyNode.h>
#include <WebCore/ScrollingTreePositionedNodeCocoa.h>
#include <WebCore/ScrollingTreeStickyNodeCocoa.h>

#if PLATFORM(IOS_FAMILY)
#include "ScrollingTreeFrameScrollingNodeRemoteIOS.h"
#include "ScrollingTreeOverflowScrollingNodeIOS.h"
#else
#include "ScrollingTreeFrameScrollingNodeRemoteMac.h"
#include "ScrollingTreeOverflowScrollingNodeRemoteMac.h"
#endif

namespace WebKit {
using namespace WebCore;

Ref<RemoteScrollingTree> RemoteScrollingTree::create(RemoteScrollingCoordinatorProxy& scrollingCoordinator)
{
    return adoptRef(*new RemoteScrollingTree(scrollingCoordinator));
}

RemoteScrollingTree::RemoteScrollingTree(RemoteScrollingCoordinatorProxy& scrollingCoordinator)
    : m_scrollingCoordinatorProxy(scrollingCoordinator)
{
}

RemoteScrollingTree::~RemoteScrollingTree() = default;

void RemoteScrollingTree::invalidate()
{
    Locker locker { m_treeLock };
    removeAllNodes();
}

#if PLATFORM(MAC)
void RemoteScrollingTree::handleWheelEventPhase(ScrollingNodeID, PlatformWheelEventPhase)
{
    // FIXME: hand off to m_scrollingCoordinatorProxy?
}
#endif

#if PLATFORM(IOS_FAMILY)
void RemoteScrollingTree::scrollingTreeNodeWillStartPanGesture(ScrollingNodeID nodeID)
{
    m_scrollingCoordinatorProxy.scrollingTreeNodeWillStartPanGesture(nodeID);
}

void RemoteScrollingTree::scrollingTreeNodeWillStartScroll(ScrollingNodeID nodeID)
{
    m_scrollingCoordinatorProxy.scrollingTreeNodeWillStartScroll(nodeID);
}

void RemoteScrollingTree::scrollingTreeNodeDidEndScroll(ScrollingNodeID nodeID)
{
    m_scrollingCoordinatorProxy.scrollingTreeNodeDidEndScroll(nodeID);
}
#endif

void RemoteScrollingTree::scrollingTreeNodeDidScroll(ScrollingTreeScrollingNode& node, ScrollingLayerPositionAction scrollingLayerPositionAction)
{
    std::optional<FloatPoint> layoutViewportOrigin;
    if (is<ScrollingTreeFrameScrollingNode>(node))
        layoutViewportOrigin = downcast<ScrollingTreeFrameScrollingNode>(node).layoutViewport().location();

    m_scrollingCoordinatorProxy.scrollingTreeNodeDidScroll(node.scrollingNodeID(), node.currentScrollPosition(), layoutViewportOrigin, scrollingLayerPositionAction);
}

void RemoteScrollingTree::scrollingTreeNodeDidStopAnimatedScroll(ScrollingTreeScrollingNode& node)
{
    m_scrollingCoordinatorProxy.scrollingTreeNodeDidStopAnimatedScroll(node.scrollingNodeID());
}

bool RemoteScrollingTree::scrollingTreeNodeRequestsScroll(ScrollingNodeID nodeID, const RequestedScrollData& request)
{
    return m_scrollingCoordinatorProxy.scrollingTreeNodeRequestsScroll(nodeID, request);
}

Ref<ScrollingTreeNode> RemoteScrollingTree::createScrollingTreeNode(ScrollingNodeType nodeType, ScrollingNodeID nodeID)
{
    switch (nodeType) {
    case ScrollingNodeType::MainFrame:
    case ScrollingNodeType::Subframe:
#if PLATFORM(IOS_FAMILY)
        return ScrollingTreeFrameScrollingNodeRemoteIOS::create(*this, nodeType, nodeID);
#else
        return ScrollingTreeFrameScrollingNodeRemoteMac::create(*this, nodeType, nodeID);
#endif
    case ScrollingNodeType::FrameHosting:
        return ScrollingTreeFrameHostingNode::create(*this, nodeID);
    case ScrollingNodeType::Overflow:
#if PLATFORM(IOS_FAMILY)
        return ScrollingTreeOverflowScrollingNodeIOS::create(*this, nodeID);
#else
        return ScrollingTreeOverflowScrollingNodeRemoteMac::create(*this, nodeID);
#endif
    case ScrollingNodeType::OverflowProxy:
        return ScrollingTreeOverflowScrollProxyNode::create(*this, nodeID);
    case ScrollingNodeType::Fixed:
        return ScrollingTreeFixedNodeCocoa::create(*this, nodeID);
    case ScrollingNodeType::Sticky:
        return ScrollingTreeStickyNodeCocoa::create(*this, nodeID);
    case ScrollingNodeType::Positioned:
        return ScrollingTreePositionedNodeCocoa::create(*this, nodeID);
    }
    ASSERT_NOT_REACHED();
    return ScrollingTreeFixedNodeCocoa::create(*this, nodeID);
}

void RemoteScrollingTree::currentSnapPointIndicesDidChange(ScrollingNodeID nodeID, std::optional<unsigned> horizontal, std::optional<unsigned> vertical)
{
    m_scrollingCoordinatorProxy.currentSnapPointIndicesDidChange(nodeID, horizontal, vertical);
}

void RemoteScrollingTree::handleMouseEvent(const WebCore::PlatformMouseEvent& event)
{
#if PLATFORM(MAC)
    if (!rootNode())
        return;
    static_cast<ScrollingTreeFrameScrollingNodeRemoteMac&>(*rootNode()).handleMouseEvent(event);
#else
    UNUSED_PARAM(event);
#endif
}

} // namespace WebKit

#endif // ENABLE(UI_SIDE_COMPOSITING)
