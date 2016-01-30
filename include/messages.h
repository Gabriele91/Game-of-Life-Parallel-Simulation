//
//  messages.h
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef messages_h
#define messages_h

#include <grid.h>
#include <grid.h>

enum type_msg
{
    T_MSG_INIT,
    T_MSG_START,
    T_MSG_EDGES
};

struct grid_in_cluster
{
    grid::point_g m_pos;
    grid::point_g m_size;
};

inline void build_actions_message(byte_vector_stream& stream_vector,grid::edges_actions& edges_actions)
{
    //push values
    for(const grid::edges_action& edge_action : edges_actions)
    {
        stream_vector.add(edge_action);
    }
}

#endif /* messages_h */
