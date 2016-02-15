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
#include <grid_json.h>
#include <file_os.h>
#include <get_time.h>

class server_gol : server_listener
{
public:
    
    enum state
    {
        S_WAIT,
        S_SEND_UPDATE,
        S_ACK_UPDATE,
        S_WAIT_RES,
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
    
    struct client_res
    {
        bool m_arrived { false };
        grid m_grid;
        
        client_res()
        {
        }
        
        client_res(bool arrived,const grid& l_grid)
        {
            m_arrived = arrived;
            m_grid    = l_grid;
        }
    };
    
    using clients_grid_res = std::unordered_map< UID, client_res >;
    
    server_gol() { }
    server_gol(const std::string& path,
               const grid::point_g&  cluster_size,
               const grid::point_g&  n_rows_columns,
               size_t n_steps = 0,
               bool circle = false)
    {
        m_path = path;
        init_cluster(cluster_size, n_rows_columns,n_steps,circle);
    }
    
    void set_state0(const grid::matrix& state)
    {
        if(state.size()    == m_state0.size() &&
           state[0].size() == m_state0[0].size())
        {
            m_state0 = state;
            return;
        }
        //else
        m_state0.resize(m_cluster_size.y);
        //alloc
        for(size_t row = 0 ; row != m_cluster_size.y; ++row)
            m_state0[row].resize(m_cluster_size.x);
        //for all
        for(size_t y = 0 ; y != m_cluster_size.y; ++y)
        {
            //copy values
            for(size_t x=0;
                y < state.size() &&
                x < state[y].size() &&
                x < m_cluster_size.x;
              ++x)
            {
                m_state0[y][x] = state[y][x];
            }
        }
        
    }
    
    class uid_cluster_grid
    {
        
        using uid_row    = std::vector < UID >;
        using uid_matrix = std::vector < uid_row >;
        
    public:
        
        uid_cluster_grid(){}
        uid_cluster_grid(const grid::point_g& size,
                         bool enable_circle = false)
        {
            init(size,enable_circle);
        }
        
        void init(const grid::point_g& size,
                  bool enable_circle = false)
        {
            m_size          = size;
            m_enable_circle = enable_circle;
            //uid counter
            UID uid = 0;
            //alloc ..
            m_matrix.resize(size.y);
            //
            for(grid::point_g::type y = 0; y!=size.y; ++y)
            {
                //alloc
                m_matrix[y].resize(size.x);
                //init values
                for(grid::point_g::type x = 0; x!=size.x; ++x)
                {
                    m_matrix[y][x] = ++uid;
                }
            }
        }
        
        UID operator () (grid::point_g::type  x,grid::point_g::type  y) const
        {
            return operator[]({x,y});
        }
        
        UID operator [] (const grid::point_g& pos) const
        {
            if(m_enable_circle)
            {
                grid::point_g::type x = positive::mod(pos.x, m_size.x);
                grid::point_g::type y = positive::mod(pos.y, m_size.y);
                //return cirlce uid
                return m_matrix[y][x];
            }
            else if(pos.x < 0 ||
                    pos.y < 0 ||
                    pos.x >= m_size.x ||
                    pos.y >= m_size.y)
            {
                return -1;
            }
            
            return m_matrix[pos.y][pos.x];
        }
        
    
    private:
        bool          m_enable_circle { false };
        uid_matrix    m_matrix;
        grid::point_g m_size;
    };
    
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
        //init uid grid
        uid_cluster_grid uid_grid ( n_rows_columns, circle );
        //get uids
        
        for(grid::point_g::type y = 0; y != n_rows_columns.y; ++y)
        for(grid::point_g::type x = 0; x != n_rows_columns.x; ++x)
        {
            m_clients_grid_map[ uid_grid(x,y) ] =
            client_grid
            {
                grid_in_cluster
                {
                    grid::point_g(x*size.x,y*size.y),
                    grid::point_g(size)
                },
                static_cast<UID>(uid_grid(x-1,y)),
                static_cast<UID>(uid_grid(x+1,y)),
                static_cast<UID>(uid_grid(x,y-1)),
                static_cast<UID>(uid_grid(x,y+1)),
                //cornes
                static_cast<UID>(uid_grid(x-1,y-1)),
                static_cast<UID>(uid_grid(x+1,y-1)),
                static_cast<UID>(uid_grid(x-1,y+1)),
                static_cast<UID>(uid_grid(x+1,y+1))
            };
        }
        //init global time
        m_global_time  = 0;
        //save rows and columns
        m_n_rows_columns = n_rows_columns;
        //init acks map
        m_clients_ack  = clients_ack_map();
        //save cluster size
        m_cluster_size = cluster_size;
        //is cicle
        m_circle = circle;
        //debug output
        //print_client_grid_all();
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
        //flag
        bool have_state0 = m_state0.size();
        //have state 0?
        init_message.add(have_state0);
        //push true/false
        if(have_state0)
        build_grid_message(init_message,
                           m_state0,
                           info.grid_in_cluster.m_pos,
                           info.grid_in_cluster.m_size);
        //...
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
                default:
                    if( filter == (grid::TOP|grid::LEFT))     {  MESSAGE("send time: " << time << " to TOP-LEFT"); }
                    if( filter == (grid::TOP|grid::RIGHT))    {  MESSAGE("send time: " << time << " to TOP-RIGHT"); }
                    if( filter == (grid::BOTTOM|grid::LEFT))  {  MESSAGE("send time: " << time << " to BOTTOM-LEFT"); }
                    if( filter == (grid::BOTTOM|grid::RIGHT)) {  MESSAGE("send time: " << time << " to BOTTOM-RIGHT"); }
                break;
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
            if(position.x != edge_position.x &&
               position.y != edge_position.y)
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
                //
                send_to(info.m_top, time, edges_history, grid::TOP);
                send_to(info.m_bottom, time, edges_history, grid::BOTTOM);
                send_to(info.m_left, time, edges_history, grid::LEFT);
                send_to(info.m_right, time, edges_history, grid::RIGHT);
                //cornes
                send_to(info.m_left_top,    time, edges_history, grid::TOP   |grid::LEFT,true);
                send_to(info.m_right_top,   time, edges_history, grid::TOP   |grid::RIGHT,true);
                send_to(info.m_left_bottom, time, edges_history, grid::BOTTOM|grid::LEFT,true);
                send_to(info.m_right_bottom,time, edges_history, grid::BOTTOM|grid::RIGHT,true);
                //send  end
                byte_vector_stream message;
                //put type
                message.add(T_MSG_END_EDGES);
                //end
                send(client.m_uid, message);
            }
            break;
            case T_MSG_GET_RESULT:
            {
                //read message
                m_clients_res_map[client.m_uid] =
                client_res
                (
                    true,
                    get_grid_and_history(msg)
                 );
                MESSAGE("---------------------------------------------------------------");
                MESSAGE(client.m_uid);
                MESSAGE("---------------------------------------------------------------");
                //print res
                for(grid::time_g time = 1; time != m_global_time + 1; ++time)
                {
                    //...
                    m_clients_res_map[client.m_uid].m_grid.go_to(time);
                    //...
                    MESSAGE(m_clients_res_map[client.m_uid].m_grid.to_string_borders())
                }
                MESSAGE("---------------------------------------------------------------");
                MESSAGE("---------------------------------------------------------------");
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
                    if(m_global_time >= m_n_steps)
                    {
                        for(auto& l_client : m_clients)
                        {
                            m_clients_res_map[l_client.first].m_arrived = false;
                            //send "get res"
                            byte_vector_stream stream;
                            stream.add(T_MSG_SEND_RESULT);
                            send(l_client.first,stream);
                            //..
                        }
                        //change state
                        m_server_state = S_WAIT_RES;
                        //break
                        break;
                    }
                    //append time
                    m_times.push_back(get_time());
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
                case S_WAIT_RES:
                {
                    //do end?
                    bool b_end = true;
                    //all res
                    for(auto& m_res : m_clients_res_map)
                    {
                        if(!m_res.second.m_arrived)
                        {
                            b_end = false;
                            break;
                        }
                    }
                    //else end
                    if(b_end)
                    {
                        save_all_res(m_path);
                        m_server_state = S_END;
                    }
                }
                break;
                default: assert(0); break;
            }
            //update raknet
            update();
        }
        //send good bye
        for(auto& l_client : m_clients)
        {
            byte_vector_stream stream;
            stream.add(T_MSG_SAY_GOOD_BYE);
            send(l_client.first,stream);
        }
        //update raknet
        update();
        //good bye
        for(auto& client_grid : m_clients_grid_map)
        {
            close_client_connection(client_grid.first);
        }
        //update raknet
        update();
    }
    
    bool save_all_res(const std::string& path)
    {
        //create ouput dir
        if(!create_dir(path, 0664))
        {
            MESSAGE("Can't crate directory: " << path);
            return false;
        }
        //build outputs
        for(auto it : m_clients_res_map)
        {
            //client path
            std::string client_path = path+"/"+std::to_string(it.first);
            //make dir
            if(!create_dir(client_path, 664))
            {
                MESSAGE("Can't crate directory: " << client_path);
                return false;
            }
            //for all times
            for(grid::time_g time = 1; time != m_global_time + 1; ++time)
            {
                json11::Json json;
                json11::Json::object jobject;
                //apply temp
                it.second.m_grid.go_to(time);
                //serialize
                jobject["data"]=get_jarray_relative_col(it.second.m_grid.get_matrix());
                json = jobject;
                //save
                std::string time_path = client_path + "/" + std::to_string(time-1) + ".json";
                //..
                if(!save_string(json.dump(), time_path))
                {
                    MESSAGE("Can't save file: " << time_path);
                    return false;
                }
            }
        }
        //create conf file
        std::string path_conf = path +"/conf.json";
        json11::Json::object jobject;
        jobject["cols"] = (int)m_cluster_size.x;
        jobject["rows"] = (int)m_cluster_size.y;
        jobject["steps"] = (int)m_global_time - 1;
#if 0
        jobject["workers_x_column"] = (int)m_n_rows_columns.x;
        jobject["workers_x_row"] = (int)m_n_rows_columns.y;
#else
        jobject["workers_x_column"] = (int)m_n_rows_columns.y;
        jobject["workers_x_row"] = (int)m_n_rows_columns.x;
#endif
        jobject["time_elapsed"] = (m_times.size() ? ((m_times[m_times.size()-1])-(m_times[0])) : 0.0) / 1000.0;
        //json times
        json11::Json::array jarray_times;
        for(double value:m_times) jarray_times.push_back((value - m_times[0])/ 1000.0);
        jobject["times"] = jarray_times;
        //append
        json11::Json json = jobject;
        //..
        if(!save_string(json.dump(), path_conf))
        {
            MESSAGE("Can't save file: " << path_conf);
            return false;
        }
        //return result saved...
        return true;
    }
    
    
    void print_client_grid_all() const
    {
        for(UID uid = 1; uid <= m_clients_grid_map.size(); ++uid)
        {
            print_client_grid(uid);
            MESSAGE( "" );
        }
    }
    
    void print_client_grid(UID uid) const
    {
        //get grid
        const client_grid& c_grid = m_clients_grid_map.find(uid)->second;
        //print..
        printf("\t-\t-\t-\t-\t-\n"
               "\t%d\t|\t%d\t|\t%d\t\n"
               "\t-\t-\t-\t-\t-\n"
               "\t%d\t|\t%d\t|\t%d\t\n"
               "\t-\t-\t-\t-\t-\t\n"
               "\t%d\t|\t%d\t|\t%d\t\n"
               "\t-\t-\t-\t-\t-\n"
               ,c_grid.m_left_top
               ,c_grid.m_top
               ,c_grid.m_right_top
               
               ,c_grid.m_left
               ,uid
               ,c_grid.m_right
               
               ,c_grid.m_left_bottom
               ,c_grid.m_bottom
               ,c_grid.m_right_bottom);
        
    }
    
private:
    std::vector <double>  m_times;
    std::string           m_path;
    grid::matrix          m_state0;
    clients_ack_map       m_clients_ack;
    grid::point_g         m_n_rows_columns;
    grid::point_g         m_cluster_size;
    grid::time_g          m_global_time      { 0      };
    state                 m_server_state     { S_WAIT };
    int                   m_max_clients      { 0      };
    int                   m_msg_init         { 0      };
    bool                  m_circle           { false  };
    size_t                m_n_steps          { 0      };
    clients_grid_map      m_clients_grid_map;
    clients_grid_res      m_clients_res_map;
    
};

#endif /* server_gol_h */
