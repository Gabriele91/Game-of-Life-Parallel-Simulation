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

#include <unordered_map>
#include <iostream>
#include <grid.h>
#include <assert.h>
#include <rak_listener.h>
#include <messages.h>
#include <iostream>
#define MESSAGE(x) std::cout << x << std::endl;
#define MSLEEP(x) usleep((x)*1000)

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
            }
            break;
            case T_MSG_START:
                //init grid
                m_grid = std::unique_ptr<grid>(new grid(m_info_gird.m_pos,
                                                        m_info_gird.m_size));
                //i'm 1?
                if(get_uid()==1)
                {
                                               m_grid->global({1,0}) = 1;
                                                                          m_grid->global({2,1}) = 1;
                    m_grid->global({0,2}) = 1; m_grid->global({1,2}) = 1; m_grid->global({2,2}) = 1;
                }
            break;
            case T_MSG_EDGES:
            {
                //time of message
                long time = 0;
                //edges_history
                grid::edges_history edges_history;
                //read message
                get_history_message(msg, time, edges_history);
                //get ...
                MESSAGE(   "get time: " << time
                        << " UID: "     << get_uid()
                        << " actions: " << edges_history.m_edges_actions.size() );
                //applay
                m_grid->applay_history_edges(time, edges_history);
                //compute this time
                m_grid->update();
                //update
                this_update();
            }
            break;
            default: break;
        }
    }
    
    virtual void on_disconnected()
    {
        m_loop = false;
    }
    
    void this_update()
    {
        //sleep
        MSLEEP(200);
        //if is started
        if(m_grid.get())
        {
            //print
            if( m_grid->time() <= 24 )
            {
                std::cout <<  get_uid() << "\n" << m_grid->to_string_borders() << std::endl;
            }
            //get edge diff
            auto last_history = m_grid->get_last_history_edges(m_filter);
            //send?
            if(last_history.m_edges_actions.size())
            {
                //get last time
                long last_time = (long)m_grid->time();
                //build message
                byte_vector_stream msg;
                //add history
                build_history_message(msg,last_time,last_history);
                //send
                MESSAGE(     "send time: " << last_time
                           << " UID: "     << get_uid()
                           << " actions: " << last_history.m_edges_actions.size() )
                //send
                send(msg);
            }
            //update grid
            m_grid->update();
        }
        
    }
    
    void loop()
    {
        while(m_loop)
        {
            //update raknet
            update();
            this_update();
        }
    }

protected:
    bool                    m_loop{ true };
    unsigned char           m_filter;
    grid_in_cluster         m_info_gird;
    std::unique_ptr< grid > m_grid{ nullptr };
    
};

#endif /* client_gol_h */
