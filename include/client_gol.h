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
        //read type
        type_msg type;
        stream.Read(type);
        //read message by type
        switch (type)
        {
            case T_MSG_INIT:
            {
                //get info
                stream.Read(m_info_gird);
                stream.Read(m_filter);
                //init
                m_grid = std::unique_ptr<grid>(new grid(m_info_gird.m_pos,
                                                        m_info_gird.m_size));
            }
            break;
                
            default: break;
        }
    }
    
    virtual void on_disconnected()
    {
    }
    
    void loop()
    {
        while(true)
        {
            //update raknet
            update();
        }
    }

protected:
    
    unsigned char           m_filter;
    grid_in_cluster         m_info_gird;
    std::unique_ptr< grid > m_grid;
    
};

#endif /* client_gol_h */
