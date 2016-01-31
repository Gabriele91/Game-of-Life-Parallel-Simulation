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

#ifdef _MSC_VER
    #define ASPACKED( __Declaration__ ) __pragma( pack(push,1) ) __Declaration__   __pragma( pack(pop) )
#else
    #define ASPACKED( __Declaration__ ) __Declaration__ __attribute__((packed))
#endif

enum type_msg
{
    T_MSG_INIT,
    T_MSG_START,
    T_MSG_EDGES
};

ASPACKED(struct grid_in_cluster
{
    grid::point_g m_pos;
    grid::point_g m_size;
});

inline void get_history_message(byte_vector_stream& stream_vector,
                                long& time,
                                grid::edges_history& edges_history)
{
    //add time
    stream_vector.get(time);
    //add filter
    stream_vector.get(edges_history.m_edges);
    //count
    long count = 0;
    //add size
    stream_vector.get(count);
    //push values
    for (long x = 0; x!=count; ++x)
    {
        grid::edges_action edge_action;
        stream_vector.get(edge_action);
        edges_history.m_edges_actions.push_back(edge_action);
    }
}

inline void build_history_message(byte_vector_stream& stream_vector,
                                  long time,
                                  const grid::edges_history& edges_history)
{
    //add time
    stream_vector.add(time);
    //add filter
    stream_vector.add(edges_history.m_edges);
    //count of actions
    long count = (long)edges_history.m_edges_actions.size();
    //add size
    stream_vector.add(count);
    //push values
    for(const grid::edges_action& edge_action : edges_history.m_edges_actions)
    {
        stream_vector.add(edge_action);
    }
}

#endif /* messages_h */
