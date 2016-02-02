//
//  grid.h
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef grid_h
#define grid_h
#include <map>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <array>
#include <vector>
#include <string>
#include <point.h>
#include <assert.h>



class grid
{
public:

    using time_g   = long;
    using point_g  = point_l;
    using value    = short;
    using row      = std::vector< value >;
    using matrix   = std::vector< row >;
    
    struct point_hash_equal
    {
        size_t operator () (const point_g& key) const
        {
        #if 0
            //64/32 bit machine
            if(sizeof(size_t) == 8)
            {
                return (key.x  & 0xFFFFFFFF00000000) | (key.y >> (4*8)) ;
            }
            if(sizeof(size_t) == 4)
            {
                return (key.x  & 0xFFFF0000) | (key.y >> (2*8)) ;
            }
            else
            {
                assert(0);
            }
        #else
            const long max = std::numeric_limits<long>::max();
            return (max-key.x) * (max-key.y);
        #endif
            //hash
        }
        
        bool operator () (const point_g& left,const point_g& right) const
        {
            return left == right;
        }
    };
    

    struct action
    {
        point_g m_position;
        value   m_old;
        value   m_new;
        
        bool operator == (const action& action) const
        {
            return m_position == action.m_position &&
                   m_old      == action.m_old      &&
                   m_new      == action.m_new;;
        }
        
        bool operator != (const action& action) const
        {
            return m_position != action.m_position ||
                   m_old      != action.m_old      ||
                   m_new      != action.m_new;;
        }
    };

    using actions  = std::unordered_map< point_g,
                                         action,
                                         point_hash_equal,
                                         point_hash_equal >;
    using history  = std::map< time_g,  actions >;

	enum edge
	{
		LEFT   = 0b0001,
		RIGHT  = 0b0010,
		TOP    = 0b0100,
		BOTTOM = 0b1000,
		ALL    = 0b1111,
	};

	struct edges_action
	{
		unsigned char  m_edge;
		action         m_action;
	};

	using edges_actions = std::vector< edges_action >;

    struct edges_history
    {
        unsigned char m_edges;
        edges_actions m_edges_actions;
		edges_history applay_filter(unsigned char filter,bool equals = false) const
		{
			edges_history output;
			output.m_edges = m_edges & filter;

			//applay filter..
			if (output.m_edges)
			{
				for (auto& edge_action : m_edges_actions)
                {
					if (!equals && edge_action.m_edge & output.m_edges)
					{
						output.m_edges_actions.push_back(edge_action);
					}
                    else if (equals && edge_action.m_edge == output.m_edges)
                    {
                        output.m_edges_actions.push_back(edge_action);
                    }
                }
			}
			//return
			return output;
		}
    };
    
    grid(const point_g& position,
         const point_g& size)
    {
        //size
        m_position = position;
        m_size     = size;
        //alloc rows
        m_matrix.resize( m_size.y +2 );
        //alloc colunm
        for(size_t row = 0 ; row != m_matrix.size(); ++row)
        {
            m_matrix[row].resize( m_size.x +2 );
        }
    }

	void go_to(time_g new_time)
	{
		if (new_time < m_time) go_back(new_time);
		if (new_time > m_time) go_next(new_time);
	}

	void go_next(time_g n_time)
	{
		//if n_time < m_time fail...
		assert(n_time >= m_time);
		//no change state
		if (n_time == m_time) return;
		//for each update
		for (time_g time = m_time; time != n_time ; ++time)
		{
			update();
		}
	}
    
    void delete_history(time_g time)
    {
        //get actions
        actions& l_actions = m_history[time];
        //it not have some actions? return
        if(!l_actions.size()) return;
        //for all point in grid (no borders)
        for (auto it = l_actions.cbegin(); it != l_actions.cend(); )
        {
            if (is_inside_no_borders(it->first))
            {
                l_actions.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }
    
	void go_back(time_g b_time)
    {
        //if b_time > m_time fail...
        assert(b_time <= m_time);
        //no change state
        if(!m_time)          return;
        if(b_time == m_time) return;
        //go back
        for(time_g l_time = m_time; l_time !=b_time; --l_time)
        {
            applay_old(l_time);
			//remove (?)
            delete_history(l_time);
        }
        //new corrent time
        m_time = b_time;
    }
    
    actions applay_history_edges(time_g time,const edges_history& history)
    {
        //save diff
        actions old_actions = m_history[time];
        //go back
        go_to(time-1);
        //add history
        for(auto& action : history.m_edges_actions)
        {
            if(is_inside_borders(action.m_action.m_position))
            {
                m_history[time][action.m_action.m_position] = action.m_action;
            }
            else
            {
                //assert(0);
				std::cout 
					<< "wrong: " 
					<< action.m_action.m_position.to_string() 
					<< std::endl;
            }
        }
        //update..
        update();
        //save new states
        const actions& new_actions = m_history[time];
        //update..
        actions output;
        //for all actions
        for(auto it_new_action : new_actions)
        {
            //search
            auto it_old_action = old_actions.find(it_new_action.first);
            //1 - not exists
            if(it_old_action == old_actions.end())
            {
                output[it_new_action.first] = it_new_action.second;
            }
            //are diff
            else if(it_old_action->second != it_new_action.second)
            {
                output[it_new_action.first] = it_new_action.second;
            }
        }
        //output borders
        return output;
    }
    
    edges_history get_last_history_edges(unsigned char filter = ALL)
    {
        return std::move(get_history_edges(m_time,filter));
    }
    
    
    edges_history get_history_edges(const actions& l_actions, unsigned char filter = ALL)
    {
        //boder type
        unsigned char g_type = 0;
        unsigned char type   = 0;
        //output
        edges_actions output;
        //applay
        for (auto it : l_actions)
        {
            //action
            const action& l_action = it.second;
            //no border
            type = 0;
            //position
            if (l_action.m_position.x == m_position.x)
            {
                type   |= LEFT & filter;
                g_type |= LEFT & filter;
            }
            if (l_action.m_position.x == m_position.x + m_size.x - 1)
            {
                type   |= RIGHT & filter;
                g_type |= RIGHT & filter;
            }
            if (l_action.m_position.y == m_position.y)
            {
                type   |= TOP & filter;
                g_type |= TOP & filter;
            }
            if (l_action.m_position.y == m_position.y + m_size.y - 1)
            {
                type   |= BOTTOM & filter;
                g_type |= BOTTOM & filter;
            }
            //is on edge?
            if (type)
            {
                output.push_back(
                                 edges_action
                                 {
                                     type,
                                     l_action
                                 });
            }
        }
        //return info
        return edges_history
        {
            g_type,
            output
        };
    }
    
	edges_history get_history_edges(time_g time, unsigned char filter = ALL)
	{
		//get actions
		const actions& l_actions = m_history[time];
        return std::move(get_history_edges(l_actions,filter));
	}

	void update()
	{
		//next state
		next();
		//applay
		applay_new(m_time);
	}

    void next()
    {
        //count inc
        ++m_time;
        //get
        actions& l_actions=m_history[m_time];
        //
        for(point_g::type y = 0; y != m_size.y; ++y)
        for(point_g::type x = 0; x != m_size.x; ++x)
        {
            //next state
            point_g point = m_position + point_g(x,y);
            value    next = automa_next(point);
            //add action?
            if(next != global(point))
            {
                l_actions[point]=
                action
                {
                    point,
                    global(point),
                    next
                };
            }
        }
    }
    
    time_g time() const
    {
        return m_time;
    }
    
    point_g real_size() const
    {
        return m_size+point_g(2,2);
    }
    
    const point_g& size() const
    {
        return m_size;
    }
    
    const point_g& position() const
    {
        return m_position;
    }
    
    value& global(const point_g& p)
    {
        //relative
		point_g pos = p - m_position;
        //global
        return m_matrix[pos.y+1][pos.x+1];
    }
    
    value& global(point_g::type x, point_g::type y)
    {
        return global(point_g(x,y));
    }
    
    value& local(const point_g& p)
    {
        return m_matrix[p.y][p.x];
    }
    
    value& local(point_g::type x, point_g::type y)
    {
        return local(point_g(x,y));
    }
    
    value& operator ()(const point_g& p)
    {
        return global(p);
    }
    
    value& operator ()(point_g::type x, point_g::type y)
    {
        return global(point_g(x, y));
    }
    
    bool is_inside(const point_g& global)
    {
        point_g min_p = m_position - point_g(1, 1);
        point_g max_p = m_position + m_size;
        if (global.x < min_p.x) return false;
        if (global.x > max_p.x) return false;
        if (global.y < min_p.y) return false;
        if (global.y > max_p.y) return false;
        return true;
    }
    
    bool is_inside_no_borders(const point_g& global)
    {
        point_g min_p = m_position;
        point_g max_p = m_position + m_size - point_g(1, 1);
        if (global.x < min_p.x) return false;
        if (global.x > max_p.x) return false;
        if (global.y < min_p.y) return false;
        if (global.y > max_p.y) return false;
        return true;
    }
    
    bool is_inside_borders(const point_g& global)
    {
        return is_inside(global) && !is_inside_no_borders(global);
    }
    
    std::string to_string(bool print_actions = false)
    {
        std::string outstring;
        
        outstring += line_to_string();
        outstring += "Time: ";
        outstring += std::to_string(m_time);
        outstring += "\n";
        outstring += line_to_string();
        
        for(point_g::type y = 0; y != m_size.y; ++y)
        {
            for(point_g::type x = 0; x != m_size.x; ++x)
            {
                point_g position = m_position+point_g(x,y);
                outstring       += std::to_string(global(position)) +" ";
            }
            outstring += "\n";
        }
        
        if(print_actions)
        {
            time_g last =  m_time ? m_time -1 : 0;
            outstring  += line_to_string();
            outstring  += history_to_string( last );
            outstring  += "\n";
            outstring  += line_to_string();
        }
        
        return outstring;
    }
    
    
    std::string to_string_borders(bool print_actions = false)
    {
        std::string outstring;
        
        outstring += line_to_string_borders()+"\n";
        outstring += "Time: ";
        outstring += std::to_string(m_time);
        outstring += "\n";
        outstring += line_to_string_borders()+"\n";
        
        for(point_g::type y = 0; y != m_size.y+2; ++y)
        {
            if(y == m_size.y+1) outstring += line_to_string_borders()+"\n";
            
            for(point_g::type x = 0; x != m_size.x+2; ++x)
            {
                outstring       += std::to_string(local(point_g(x, y))) + (x==0 || x==m_size.x ? "|" : " ");
            }
            
            if(y == 0) outstring += "\n" + line_to_string_borders();
            
            outstring += "\n";
        }
        
        if(print_actions)
        {
            outstring  += line_to_string();
            outstring  += history_to_string( m_time );
            outstring  += "\n";
            outstring  += line_to_string();
        }
        
        return outstring;
    }
    
protected:
    
    
    value automa_next(const point_g& p)
    {
        //count near
        size_t count = 0;
        //search near
        for(point_g::type y = p.y-1; y != p.y+2; ++y)
        for(point_g::type x = p.x-1; x != p.x+2; ++x)
        {
            count += global({x, y}) ? 1 : 0;
        }
        //is live
        if(global(p))
        {
            //p is counted...
            --count;
            //case dead:
            if(count<2 || count>3) return 0;
            //live
            return 1;
        }
        else
        {
            //case live:
            if(count==3) return 1;
            //dead
            return 0;
        }
    }
    
    void applay_new(time_g time)
    {
        //search
        auto it = m_history.find(time);
        //
        if(it != m_history.end())
        {
            //applay
            for(auto it : it->second)
            {
                global(it.second.m_position) = it.second.m_new;
            }
        }
    }
    
    void applay_old(time_g time)
    {
        //search
        auto it = m_history.find(time);
        //
        if(it != m_history.end())
        {
            //applay
            for(auto it : it->second)
            {
                global(it.second.m_position) = it.second.m_old;
            }
        }
    }
    
    std::string history_to_string(time_g time)
    {
        std::string outstring;
        //search
        auto it = m_history.find(time);
        //
        if(it != m_history.end())
        {
            //applay
            for(auto it : it->second)
            {
                const action& l_action = it.second;
                outstring+= "position: " + l_action.m_position.to_string() + "\n";
                outstring+= "old state:" + std::to_string(l_action.m_old) + "\n";
                outstring+= "new state:" + std::to_string(l_action.m_new) + "\n";
            }
        }
        return outstring;
    }
    
    std::string line_to_string()
    {
        std::string outstring;
        //compute line
        for(point_g::type x = 0; x != m_size.x; ++x)
        {
            outstring += "- ";
        }
        return outstring + "\n";
    }
    
    std::string line_to_string_borders()
    {
        std::string outstring;
        //compute line
        for(point_g::type x = -1; x != m_size.x+1; ++x)
        {
            outstring += "- ";
        }
        return outstring;
    }
    
private:
    
    time_g   m_time { 0 };
    history  m_history;
    point_g  m_position;
    point_g  m_size;
    matrix   m_matrix;
    
};

#endif /* grid_h */
