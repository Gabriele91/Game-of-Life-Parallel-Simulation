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

#include <iostream>
#include <grid.h>
#include <grid.h>

#define MESSAGE(x) std::cout << x << std::endl;

#ifdef _MSC_VER
    #define ASPACKED( __Declaration__ ) __pragma( pack(push,1) ) __Declaration__   __pragma( pack(pop) )
	#define MSLEEP(x) Sleep(x)
#else
    #define ASPACKED( __Declaration__ ) __Declaration__ __attribute__((packed))
	#define MSLEEP(x) usleep((x)*1000)
#endif

enum type_msg
{
    T_MSG_INIT,
    T_MSG_START,
    T_MSG_EDGES,
    T_MSG_UPDATE,
    T_MSG_ACK_UPDATE,
    T_MSG_HISTORY
};

ASPACKED(struct grid_in_cluster
{
    grid::point_g m_pos;
    grid::point_g m_size;
});

inline void build_grid_message(byte_vector_stream & stream_vector,
                               const grid::matrix & mat_grid,
                               const grid::point_g& pos,
                               const grid::point_g& size)
{
    //push
    stream_vector.add(pos);
    stream_vector.add(size);
    //for all
    for(grid::point_g::type y=0; y!=size.y; ++y )
    for(grid::point_g::type x=0; x!=size.x; ++x )
    {
        stream_vector.add(mat_grid[y+pos.y][x+pos.x]);
    }
}
inline grid::matrix get_grid_message(byte_vector_stream& stream_vector,
                                     grid::point_g& pos,
                                     grid::point_g& size)
{
    //push
    stream_vector.get(pos);
    stream_vector.get(size);
    //alloc
    grid::matrix mat_grid;
    //alloc rows
    mat_grid.resize( size.y );
    //alloc colunm
    for(size_t row = 0 ; row != size.y; ++row)
    {
        mat_grid[row].resize( size.x );
    }
    //for all
    for(grid::point_g::type y=0; y!=size.y; ++y )
    for(grid::point_g::type x=0; x!=size.x; ++x )
    {
        stream_vector.get(mat_grid[y][x]);
    }
    //return
    return std::move(mat_grid);
}

inline void get_history_message(byte_vector_stream& stream_vector,
                                grid::time_g& time,
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
