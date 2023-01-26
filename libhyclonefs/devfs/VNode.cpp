#include "VNode.h"

#include <iostream>

VNode::VNode(Volume* volume, fssh_ino_t id, fssh_ino_t parentID)
    : fVolume(volume)
    , fID(id)
    , fParentID(parentID)
{
}