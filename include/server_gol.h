//
//  server_gol.h
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef server_gol_h
#define server_gol_h

#include <unordered_map>
#include <iostream>
#include <grid.h>
#include <assert.h>
#include <rak_listener.h>
#include <messages.h>

#define MESSAGE( x ) std::cout << x << std::endl;

class server_gol : server_listener
{
public:
    
    enum state
    {
        S_WAIT,
        S_START,
        S_END
    };
    
    struct client_grid
    {
        grid_in_cluster grid_in_cluster;
        UID             m_left;
        UID             m_right;
        UID             m_top;
        UID             m_bottom;
        //build filter
        unsigned char filter() const
        {
            unsigned char type;
            type |= m_left   < 0 ? 0 : grid::LEFT;
            type |= m_right  < 0 ? 0 : grid::RIGHT;
            type |= m_top    < 0 ? 0 : grid::TOP;
            type |= m_bottom < 0 ? 0 : grid::BOTTOM;
            return type;
        }
    };
    
    using clients_grid_map = std::unordered_map< UID, client_grid >;
    
    server_gol() { }
    server_gol(const grid::point_g& cluster_size,grid::point_g::type n_rows_columns)
    {
        init_cluster(cluster_size, n_rows_columns);
    }
    
    void init_cluster(const grid::point_g& cluster_size,grid::point_g::type n_rows_columns)
    {
        //is a MCD
        assert((cluster_size.x % n_rows_columns)==0);
        assert((cluster_size.y % n_rows_columns)==0);
        //
        grid::point_g size(cluster_size.x / n_rows_columns,
                           cluster_size.y / n_rows_columns);
        //fake uid
        grid::point_g::type uid { 0 };
        //make custer grid
        for(grid::point_g::type y = 0; y != n_rows_columns; ++y)
        for(grid::point_g::type x = 0; x != n_rows_columns; ++x)
        {
            //left
            grid::point_g::type cell_left = x == 0 ? -1 : x-1+y*n_rows_columns;
            //right
            grid::point_g::type cell_right   = x == n_rows_columns-1 ? -1 : x+1+y*n_rows_columns;
            //top
            grid::point_g::type cell_top = y == 0 ? -1 : x+(y-1)*n_rows_columns;
            //bottom
            grid::point_g::type cell_bottom = y == n_rows_columns-1 ? -1 : x+(y+1)*n_rows_columns;
            //...
            m_clients_grid_map[ ++uid ] =
            client_grid
            {
                grid_in_cluster
                {
                    grid::point_g(x*size.x,y*size.y),
                    grid::point_g(size)
                },
                static_cast<UID>(cell_left),
                static_cast<UID>(cell_right),
                static_cast<UID>(cell_top),
                static_cast<UID>(cell_bottom)
            };
        }
        //correction, uid start from 1, not 0.
        for(auto& it:m_clients_grid_map)
        {
            if(it.second.m_left >= 0) it.second.m_left+=1;
            if(it.second.m_right >= 0) it.second.m_right+=1;
            if(it.second.m_top >= 0) it.second.m_top+=1;
            if(it.second.m_bottom >= 0) it.second.m_bottom+=1;
        }
        //number of clients
        m_max_clients = static_cast<int>(n_rows_columns*2);
    }
    
    void open(unsigned short port, double time_out)
    {
        server_listener::open(port,time_out,m_max_clients);
    }
    
    virtual void on_connected(client& client)
    {
        MESSAGE( "connected: " << client.m_uid );
        //get info about client
        auto& info = m_clients_grid_map[client.m_uid];
        //init message
        bit_stream init_message;
        init_message.Write(T_MSG_INIT);
        init_message.Write(info.grid_in_cluster);
        init_message.Write(info.filter());
        send(client.m_uid,init_message);
        //inc count inits
        ++m_msg_init;
    }
    
    virtual void on_message(client& client, bit_stream& stream)
    {
        //get info about client
        auto& info = m_clients_grid_map[client.m_uid];
        //read int
        int filter = 0;
        stream.Read(filter);
        //alloc
        bit_size_t bytes = stream.GetNumberOfUnreadBits() / 8;
        std::unique_ptr< unsigned char > buffer(new  unsigned char[bytes]);
        //read remaning
        stream.ReadBits(buffer.get(), bytes*8);
        //init message
        bit_stream resend_message;
        resend_message.Write(T_MSG_EDGES);
        //message buffer
        resend_message.WriteBits(buffer.get(), bytes*8);
        //sent to ...
        if(filter & grid::TOP)
        {
            send(info.m_top, resend_message);
        }
        if(filter & grid::BOTTOM)
        {
            send(info.m_bottom, resend_message);
        }
        if(filter & grid::LEFT)
        {
            send(info.m_left, resend_message);
        }
        if(filter & grid::LEFT)
        {
            send(info.m_right, resend_message);
        }
    }
    
    virtual void on_disconnected(client& client)
    {
        MESSAGE( "wrong: disconnected: " << client.m_uid );
        m_server_state = S_END;
    }
    
    virtual void loop()
    {
        while(m_server_state != S_END)
        {
            //update raknet
            update();
            //states
            switch (m_server_state)
            {
                case S_WAIT:
                   if (m_msg_init == m_max_clients &&
                       count_clients() == m_max_clients)
                   {
                       for(auto& l_client : m_clients)
                       {
                           bit_stream stream;
                           stream.Write(T_MSG_START);
                           send(l_client.first,stream);
                       }
                       m_server_state = S_START;
                   }
                break;
                case S_START:
                    //to do
                    break;
                default: assert(0); break;
            }
        }
        //good bye
        for(auto& client_grid : m_clients_grid_map)
        {
            close_client_connection(client_grid.first);
        }
    }
    
private:
    state            m_server_state     { S_WAIT };
    int              m_max_clients      { 0      };
    int              m_msg_init         { 0      };
    clients_grid_map m_clients_grid_map;
    
};

#endif /* server_gol_h */
