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

class server_gol : server_listener
{
public:
    
    enum state
    {
        S_WAIT,
        S_SEND_UPDATE,
        S_ACK_UPDATE,
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
    
    using clients_ack_map  = std::map< UID, bool >;
    using clients_grid_map = std::unordered_map< UID, client_grid >;
    
    server_gol() { }
    server_gol(const grid::point_g& cluster_size,const grid::point_g&  n_rows_columns)
    {
        init_cluster(cluster_size, n_rows_columns);
    }
    
    void init_cluster(const grid::point_g& cluster_size,const grid::point_g& n_rows_columns)
    {
        //is a MCD
        assert((cluster_size.x % n_rows_columns.x)==0);
        assert((cluster_size.y % n_rows_columns.y)==0);
        //
        grid::point_g size(cluster_size.x / n_rows_columns.x,
                           cluster_size.y / n_rows_columns.y);
        //uid
		UID uid { 0 };
        //make custer grid
        for(grid::point_g::type y = 0; y != n_rows_columns.y; ++y)
        for(grid::point_g::type x = 0; x != n_rows_columns.x; ++x)
        {
            //left
            grid::point_g::type cell_left = x == 0 ? -1 : x-1+y*n_rows_columns.y;
            //right
            grid::point_g::type cell_right   = x == n_rows_columns.x-1 ? -1 : x+1+y*n_rows_columns.y;
            //top
            grid::point_g::type cell_top = y == 0 ? -1 : x+(y-1)*n_rows_columns.x;
            //bottom
            grid::point_g::type cell_bottom = y == n_rows_columns.y-1 ? -1 : x+(y+1)*n_rows_columns.x;
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
        m_max_clients = static_cast<int>(n_rows_columns.x*n_rows_columns.y);
        //init global time
        m_global_time = 0;
        //init acks map
        m_clients_ack = clients_ack_map();
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
        byte_vector_stream init_message;
        init_message.add(T_MSG_INIT);
        init_message.add(info.grid_in_cluster);
        init_message.add(info.filter());
        send(client.m_uid,init_message);
        //inc count inits
        ++m_msg_init;
    }

	bool send_to(UID uid,
				 long time,
			     const grid::edges_history& edges_history,
				 unsigned char filter)
	{
		if (edges_history.m_edges & filter)
		{
			//applay filter
			auto edges_history_filtered = edges_history.applay_filter(filter);
			//no messages?
			if (!edges_history_filtered.m_edges_actions.size()) return false;
			//message
			byte_vector_stream message;
			//put type
			message.add(T_MSG_EDGES);
			//message buffer
			build_history_message(message, time, edges_history_filtered);
			//send
			switch (filter)
			{
				case grid::TOP:    MESSAGE("send time: " << time << " to TOP");    break;
				case grid::BOTTOM: MESSAGE("send time: " << time << " to BOTTOM"); break;
				case grid::LEFT:   MESSAGE("send time: " << time << " to LEFT");   break;
				case grid::RIGHT:  MESSAGE("send time: " << time << " to RIGHT");  break;
				default: break;
			}
			send(uid, message);
            //is sended
            return true;
		}
        //not sended
        return false;
	}
    
    virtual void on_message(client& client, bit_stream& stream)
    {
        //get info about client
        auto& info = m_clients_grid_map[client.m_uid];
        //get message
        auto msg = byte_vector_stream::from_bit_stream(stream);
        //get type
        type_msg type;
        msg.get(type);
        //cases
        switch (type)
        {
            //ack update
            case T_MSG_ACK_UPDATE:
            {
                //count clients
                m_clients_ack[client.m_uid] = true;
            }
            break;
            //get hitory diff
            case T_MSG_HISTORY:
            {
                //time of message
                grid::time_g time = 0;
                //edges_history
                grid::edges_history edges_history;
                //read message
                get_history_message(msg, time, edges_history);
                //sent to ...
                MESSAGE( "from: "<< client.m_uid )
                if(send_to(info.m_top, time, edges_history, grid::TOP))       m_clients_ack[info.m_top]    = false;
                if(send_to(info.m_bottom, time, edges_history, grid::BOTTOM)) m_clients_ack[info.m_bottom] = false;
                if(send_to(info.m_left, time, edges_history, grid::LEFT))     m_clients_ack[info.m_left]   = false;
                if(send_to(info.m_right, time, edges_history, grid::RIGHT))   m_clients_ack[info.m_right]  = false;
                //count clients
                m_clients_ack[client.m_uid] = true;
            }
            break;
            default: assert(0); break;
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
            //states
            switch (m_server_state)
            {
                case S_WAIT:
                   if (m_msg_init == m_max_clients &&
                       count_clients() == m_max_clients)
                   {
                       for(auto& l_client : m_clients)
                       {
                           byte_vector_stream stream;
                           stream.add(T_MSG_START);
                           send(l_client.first,stream);
                       }
                       m_server_state = S_SEND_UPDATE;
                   }
                    break;
                case S_SEND_UPDATE:
                    //if ...
                    if(m_global_time > 36) break;
                    //next time stemp
                    ++m_global_time;
                    //send to all
                    for(auto& l_client : m_clients)
                    {
                        byte_vector_stream stream;
                        stream.add(T_MSG_UPDATE);
                        stream.add(m_global_time);
                        send(l_client.first,stream);
                        //wait ack
                        m_clients_ack[l_client.second.m_uid] = false;
                    }
                    //wait ack
                    m_server_state = S_ACK_UPDATE;
                    //..
                    break;
                case S_ACK_UPDATE:
                    //next state?
                    m_server_state = S_SEND_UPDATE;
                    //all acks?
                    for(auto& l_client : m_clients)
                    {
                        if(!m_clients_ack[l_client.second.m_uid])
                        {
                           m_server_state = S_ACK_UPDATE;
                           break;
                        }
                    }
                    //..
                    break;
                default: assert(0); break;
            }
            //update raknet
            update();
        }
        //good bye
        for(auto& client_grid : m_clients_grid_map)
        {
            close_client_connection(client_grid.first);
        }
    }
    
private:
    clients_ack_map       m_clients_ack;
    grid::time_g          m_global_time    { 0      };
    state                 m_server_state   { S_WAIT };
    int                   m_max_clients    { 0      };
    int                   m_msg_init       { 0      };
    clients_grid_map m_clients_grid_map;
    
};

#endif /* server_gol_h */
