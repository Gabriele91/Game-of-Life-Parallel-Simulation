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
#include <modulo.h>
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
        //cornes
        UID             m_left_top;
        UID             m_right_top;
        UID             m_left_bottom;
        UID             m_right_bottom;
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
    server_gol(const grid::point_g& cluster_size,
               const grid::point_g&  n_rows_columns,
               size_t n_steps = 0,
               bool circle = false)
    {
        init_cluster(cluster_size, n_rows_columns,n_steps,circle);
    }
    
    
    void init_cluster(const grid::point_g& cluster_size,
                      const grid::point_g& n_rows_columns,
                      size_t n_steps,
                      bool circle = false)
    {
        //is a MCD
        assert((cluster_size.x % n_rows_columns.x)==0);
        assert((cluster_size.y % n_rows_columns.y)==0);
        //
        grid::point_g size(cluster_size.x / n_rows_columns.x,
                           cluster_size.y / n_rows_columns.y);
        //number of clients
        m_max_clients  = static_cast<int>(n_rows_columns.x*n_rows_columns.y);
        //count steps
        m_n_steps = n_steps;
        //uid
		UID uid { 0 };
        
        for(grid::point_g::type y = 0; y != n_rows_columns.y; ++y)
        for(grid::point_g::type x = 0; x != n_rows_columns.x; ++x)
        {
            
            grid::point_g::type t_startx = (y-1)*n_rows_columns.x;
            grid::point_g::type t_endx   = (y  )*n_rows_columns.x;
            
            grid::point_g::type startx   = y    *n_rows_columns.x;
            grid::point_g::type   endx   = (y+1)*n_rows_columns.x;
            
            grid::point_g::type b_startx = (y+1)*n_rows_columns.x;
            grid::point_g::type b_endx   = (y+2)*n_rows_columns.x;
            
            grid::point_g::type cell_left         = uid - 1;
            grid::point_g::type cell_right        = uid + 1;
            grid::point_g::type cell_top          = uid - n_rows_columns.x;
            grid::point_g::type cell_bottom       = uid + n_rows_columns.x;
            grid::point_g::type cell_left_top     = cell_top - 1;
            grid::point_g::type cell_right_top    = cell_top + 1;
            grid::point_g::type cell_left_bottom  = cell_bottom - 1;
            grid::point_g::type cell_right_bottom = cell_bottom + 1;
            
            //top
            if(cell_left_top  < t_startx || cell_left_top  >= t_endx) cell_left_top  = -1;
            if(cell_top       < t_startx || cell_top       >= t_endx) cell_top       = -1;
            if(cell_right_top < t_startx || cell_right_top >= t_endx) cell_right_top = -1;
            
            if(t_endx<=0)
            {
                cell_left_top  = -1;
                cell_top       = -1;
                cell_right_top = -1;
            }
            
            //mid
            if(cell_left  < startx || cell_left  >= endx) cell_left  = -1;
            if(cell_right < startx || cell_right >= endx) cell_right = -1;
            
            //bottom
            if(cell_left_bottom  < b_startx || cell_left_bottom  >= b_endx) cell_left_bottom  = -1;
            if(cell_bottom       < b_startx || cell_bottom       >= b_endx) cell_bottom       = -1;
            if(cell_right_bottom < b_startx || cell_right_bottom >= b_endx) cell_right_bottom = -1;
            
            if(b_startx >= m_max_clients)
            {
                cell_left_bottom  = -1;
                cell_bottom       = -1;
                cell_right_bottom = -1;
            }
            
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
                static_cast<UID>(cell_bottom),
                //cornes
                static_cast<UID>(cell_left_top),
                static_cast<UID>(cell_right_top),
                static_cast<UID>(cell_left_bottom),
                static_cast<UID>(cell_right_bottom)
            };
        }
        //create cicle
        if(circle)
        {
            //..
            for(auto& it:m_clients_grid_map)
            {
                const long uid = (long)it.first-1;
                //w
                const long   w = n_rows_columns.x;
                //h
                const long   h = n_rows_columns.y;
                //y
                const long   y = uid / w;
                //
                grid::point_g::type startx   =     y * n_rows_columns.x;
                grid::point_g::type   endx   = (y+1) * n_rows_columns.x -1;
                //
                if(it.second.m_left   < 0) it.second.m_left   = endx;
                if(it.second.m_right  < 0) it.second.m_right  = startx;
                //x
                const long  x = uid % w;
                //
                grid::point_g::type starty   = x;
                grid::point_g::type   endy   = x + (h-1) * n_rows_columns.x;
                //..
                if(it.second.m_top    < 0) it.second.m_top    = endy;
                if(it.second.m_bottom < 0) it.second.m_bottom = starty;

            }
            //bottom left uid
            long uid_b_left =  1+ (n_rows_columns.y-1) *  n_rows_columns.x;
            //...
            //circle borders
            m_clients_grid_map[1]               .m_left_top     = m_max_clients    -1;
            m_clients_grid_map[n_rows_columns.x].m_right_top    = uid_b_left       -1;
            m_clients_grid_map[uid_b_left]      .m_left_bottom  = n_rows_columns.x -1;
            m_clients_grid_map[m_max_clients]   .m_right_bottom = 0;

        }
        //correction, uid start from 1, not 0.
        for(auto& it:m_clients_grid_map)
        {
            if(it.second.m_left >= 0)         it.second.m_left+=1;
            if(it.second.m_right >= 0)        it.second.m_right+=1;
            if(it.second.m_top >= 0)          it.second.m_top+=1;
            if(it.second.m_bottom >= 0)       it.second.m_bottom+=1;
            if(it.second.m_left_top >= 0)     it.second.m_left_top+=1;
            if(it.second.m_right_top >= 0)    it.second.m_right_top+=1;
            if(it.second.m_left_bottom >= 0)  it.second.m_left_bottom+=1;
            if(it.second.m_right_bottom >= 0) it.second.m_right_bottom+=1;
        }
        //init global time
        m_global_time  = 0;
        //init acks map
        m_clients_ack  = clients_ack_map();
        //save cluster size
        m_cluster_size = cluster_size;
        //is cicle
        m_circle = circle;
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
				 unsigned char filter,
                 bool equals = false)
	{
		if (edges_history.m_edges & filter)
		{
			//applay filter
			auto edges_history_filtered = edges_history.applay_filter(filter,equals);
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
    
    void cirlce_edges_history(grid::edges_history& edges_history)
    {
        #if 0
        for (auto& edge_action : edges_history.m_edges_actions)
        {
            //ref to position of action
            auto& position = edge_action.m_action.m_position;
            //old point
            MESSAGE( position.to_string()<< " - > old point" );
            //apply circle
            position.x = (position.x == 0)                  ? m_cluster_size.x : position.x;
            position.x = (position.x == m_cluster_size.x-1) ? -1               : position.x;
            
            position.y = (position.y == 0)                  ? m_cluster_size.y : position.y;
            position.y = (position.y == m_cluster_size.y-1) ? -1               : position.y;
            //
            MESSAGE( position.to_string()<< " - > new point" );
        }
        #else
        //,,,
        grid::edges_actions new_messages;
        //,,,
        for (grid::edges_action& edge_action : edges_history.m_edges_actions)
        {
            //ref to position of action
            grid::point_g& edge_position = edge_action.m_action.m_position;
            //new action
            grid::edges_action new_action = edge_action;
            //new pos
            grid::point_g&  position = new_action.m_action.m_position;
            //apply circle
            position.x = (position.x == 0)                  ? m_cluster_size.x : position.x;
            position.x = (position.x == m_cluster_size.x-1) ? -1               : position.x;
            position.y = (position.y == 0)                  ? m_cluster_size.y : position.y;
            position.y = (position.y == m_cluster_size.y-1) ? -1               : position.y;
            //push case: pos == pos or pos.x == pos.y or pos.y ==pos.y
            new_messages.push_back(new_action);
            //other messages
            if(position != edge_position)
            {
                //...
                grid::point_g new_pos = position;
                //reset x
                position.x = edge_position.x;
                position.y = new_pos.y;
                new_messages.push_back(new_action);
                //reset y
                position.x = new_pos.x;
                position.y = edge_position.y;
                new_messages.push_back(new_action);
            }
        }
        //save new messages
        edges_history.m_edges_actions = new_messages;
        //..
        #endif
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
                //applay circle
                if(m_circle) cirlce_edges_history(edges_history);
                //sent to ...
                MESSAGE( "from: "<< client.m_uid )
                if(send_to(info.m_top, time, edges_history, grid::TOP))       m_clients_ack[info.m_top]    = false;
                if(send_to(info.m_bottom, time, edges_history, grid::BOTTOM)) m_clients_ack[info.m_bottom] = false;
                if(send_to(info.m_left, time, edges_history, grid::LEFT))     m_clients_ack[info.m_left]   = false;
                if(send_to(info.m_right, time, edges_history, grid::RIGHT))   m_clients_ack[info.m_right]  = false;
                //cornes
                if(send_to(info.m_left_top,    time, edges_history, grid::TOP|grid::LEFT,true))     m_clients_ack[info.m_left_top]      = false;
                if(send_to(info.m_right_top,   time, edges_history, grid::TOP|grid::RIGHT,true))    m_clients_ack[info.m_right_top]     = false;
                if(send_to(info.m_left_bottom, time, edges_history, grid::BOTTOM|grid::LEFT,true))  m_clients_ack[info.m_left_bottom]   = false;
                if(send_to(info.m_right_bottom,time, edges_history, grid::BOTTOM|grid::RIGHT,true)) m_clients_ack[info.m_right_bottom]  = false;
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
                    if(m_global_time >= m_n_steps) break;
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
    grid::point_g         m_cluster_size;
    grid::time_g          m_global_time      { 0      };
    state                 m_server_state     { S_WAIT };
    int                   m_max_clients      { 0      };
    int                   m_msg_init         { 0      };
    bool                  m_circle           { false  };
    size_t                m_n_steps          { 0      };
    clients_grid_map      m_clients_grid_map;
    
};

#endif /* server_gol_h */
