#include "VNode.h"

#include <iostream>

fssh_ino_t VNode::GetID() const
{
    std::cerr << "VNode::GetID() not implemented." << std::endl;
    return 0;
}

const StatData* VNode::GetStatData() const
{
    std::cerr << "VNode::GetStatData() not implemented." << std::endl;
    return NULL;
}