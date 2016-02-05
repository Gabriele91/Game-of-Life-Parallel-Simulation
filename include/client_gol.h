//
//  client_gol.h
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef client_gol_h
#define client_gol_h

#include <memory>
#include <unordered_map>
#include <iostream>
#include <grid.h>
#include <assert.h>
#include <rak_listener.h>
#include <messages.h>
#include <iostream>

class client_gol : client_listener
{
    
public:
    
    bool connect(const std::string& addr,unsigned short port,  double time_out)
    {
        return client_listener::connect(addr, port, time_out);
    }
    
    
    virtual void on_connected()
    {
        //none
    }
    
    virtual void on_get_uid(UID uid)
    {
        //none
    }
    
    virtual void on_message(bit_stream& stream)
    {
        //create message
        auto msg = byte_vector_stream::from_bit_stream(stream);
        //read type
        type_msg type { T_MSG_INIT  };
        msg.get(type);
        //read message by type
        switch (type)
        {
            case T_MSG_INIT:
            {
                //get info
                msg.get(m_info_gird);
                msg.get(m_filter);
                //init grid
                m_grid = std::unique_ptr<grid>(new grid(m_info_gird.m_pos,
                                                        m_info_gird.m_size));
                //have state?
                bool have_state0 = false;
                msg.get(have_state0);
                //..
                if(have_state0)
                {
                    grid::point_g pos;
                    grid::point_g size;
                    grid::matrix  state0 = get_grid_message(msg, pos, size);
                    m_grid->put_state(state0, pos, size);
                }
                else
                {
                    m_grid->update();
                }
                //first loop
                m_not_firt_loop = false;
                //
                MESSAGE(m_grid->to_string_borders())
            }
            break;
            case T_MSG_START:
                //enable loop
                m_start = true;
                //
            break;
            case T_MSG_UPDATE:
            {
                //
                if(m_not_firt_loop)
                    this_update();
                else
                    m_not_firt_loop = true;
                //
                send_last_time_history();
            }
            break;
            case T_MSG_EDGES:
            {
                //time of message
                grid::time_g time = 0;
                //edges_history
                grid::edges_history edges_history;
                //read message
                get_history_message(msg, time, edges_history);
                //get ...
                #if 0
                MESSAGE(   "get time: " << time
                        << " UID: "     << get_uid()
                        << " actions: " << edges_history.m_edges_actions.size() );
                #endif
                //get edge hitosry
                m_grid->applay_history_edges(edges_history);
            }
            break;
            case T_MSG_END_EDGES:
            {
                //build message
                byte_vector_stream msg;
                //send history
                msg.add(T_MSG_ACK_UPDATE);
                //send
                send(msg);
                //
            }
            break;
            case T_MSG_SEND_RESULT:
            {
                //end time
                grid::time_g end_time = m_grid->time();
                //build message
                byte_vector_stream msg;
                //send history
                msg.add(T_MSG_GET_RESULT);
                //build
                build_grid_and_history_message( msg, *m_grid, 1 );
                //send
                send(msg);
                //end...
                MESSAGE("--------------------------------------------------------")
                MESSAGE("--------------------------------------------------------")
                MESSAGE("--------------------------------------------------------")
                //test print grid
                for(grid::time_g time = 1; time <= end_time; ++time)
                {
                    //...
                    m_grid->go_to(time);
                    //...
                    MESSAGE(m_grid->to_string_borders())
                }
            }
            break;
            default: break;
        }
    }
    
    virtual void on_disconnected()
    {
        m_loop = false;
    }
    
    void send_history(grid::time_g time, const grid::edges_history& edges_history)
    {
        //send?
        if(edges_history.m_edges_actions.size())
        {
            //build message
            byte_vector_stream msg;
            //send history
            msg.add(T_MSG_HISTORY);
            //add history
            build_history_message(msg,time,edges_history);
            //send
            send(msg);
        }
        else
        {
            //build message
            byte_vector_stream msg;
            //send history
            msg.add(T_MSG_ACK_UPDATE);
            //send
            send(msg);
        }
    }
    
    void send_last_time_history()
    {
        //get edge diff
        auto last_history = m_grid->get_last_history_edges(m_filter);
        //send
        send_history(m_grid->time(),last_history);
    }
    
    void this_update()
    {
        if(m_start)
        {
            if(m_grid.get())
            {
                m_grid->update();
                MESSAGE(m_grid->to_string_borders())
            }
            else
            {
                assert(0);
            }
        }
    }
    
    void loop()
    {
        while(m_loop)
        {
            //update raknet
            update();
            //this_update();
        }
    }

protected:
    bool                    m_not_firt_loop { false };
    bool                    m_start{ false };
    bool                    m_loop { true };
    unsigned char           m_filter;
    grid_in_cluster         m_info_gird;
    std::unique_ptr< grid > m_grid{ nullptr };
    
};

#endif /* client_gol_h */
