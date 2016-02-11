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
#if defined(DEBUG) || defined(_DEBUG)
    #define MESSAGE(x) std::cout << x << std::endl;
#else
    #define MESSAGE(x)
#endif
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
    T_MSG_END_EDGES,
    T_MSG_UPDATE,
    T_MSG_ACK_UPDATE,
    T_MSG_HISTORY,
    T_MSG_SEND_RESULT,
    T_MSG_GET_RESULT,
    T_MSG_SAY_GOOD_BYE
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

inline void build_grid_and_history_message(byte_vector_stream& stream_vector,grid& a_grid, grid::time_g time)
{
    //push
    stream_vector.add(a_grid.position());
    stream_vector.add(a_grid.size());
    stream_vector.add(time);
    //state 0
    a_grid.go_to(time);
    //send state
    for(grid::point_g::type y=0; y!=a_grid.size().y; ++y )
    for(grid::point_g::type x=0; x!=a_grid.size().x; ++x )
    {
        grid::value value = a_grid.global(a_grid.position()+grid::point_g(x,y));
        stream_vector.add(value);
    }
    //send hitory
    const grid::history& l_history = a_grid.get_history();
    //...
    long h_size = l_history.size();
    stream_vector.add(h_size);
    //for all times
    for(auto& actions:l_history)
    {
        //send time
        stream_vector.add(actions.first);
        //n actions
        long a_size = actions.second.size();
        stream_vector.add(a_size);
        //send actions
        for(auto& action : actions.second)
        {
            //send a action
            stream_vector.add(action.second);
        }
    }
}

inline grid get_grid_and_history(byte_vector_stream& stream_vector)
{
    //time state 0
    grid::time_g time = 0;
    //..
    grid::point_g position;
    grid::point_g size;
    //push
    stream_vector.get(position);
    stream_vector.get(size);
    stream_vector.get(time);
    //..
    grid l_grid(position,size);
    //state 0
    l_grid.go_to(time);
    //send state
    for(grid::point_g::type y=0; y!=l_grid.size().y; ++y )
    for(grid::point_g::type x=0; x!=l_grid.size().x; ++x )
    {
        //
        grid::value value;
        stream_vector.get(value);
        //
        l_grid.global(l_grid.position()+grid::point_g(x,y)) = value;
    }
    //send hitory
    grid::history& l_history = l_grid.get_history();
    //...
    long h_size;
    //
    stream_vector.get(h_size);
    //for all times
    for(long h = 0; h!=h_size; ++h)
    {
        //get time
        grid::time_g time = 0;
        stream_vector.get(time);
        //get n actions
        long a_size = 0;
        stream_vector.get(a_size);
        //..put all actions
        if(a_size)
        {
            //alloc vector
            grid::actions& actions = l_history[time];
            //get alla actions
            for(long a = 0; a!=a_size; ++a)
            {
                //..
                grid::action action;
                //..
                stream_vector.get(action);
                //..
                actions[action.m_position] = action;
            }
        }
    }
    //return
    return l_grid;
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
