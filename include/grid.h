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
#include <array>
#include <vector>
#include <string>
#include <point.h>
#include <assert.h>

#if 0
    #define DEALLOC_HISTORY(x) x
#else
    #define DEALLOC_HISTORY(x)
#endif

class grid
{
public:


    using point_g  = point_l;
    using value    = short;
    using row      = std::vector< value >;
    using matrix   = std::vector< row >;
    
    struct action
    {
        point_g m_position;
        value   m_old;
        value   m_new;
    };

    using actions  = std::vector< action >;
    using history  = std::map< size_t, actions >;

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
		edges_history applay_filter(unsigned char filter) const
		{
			edges_history output;
			output.m_edges = m_edges & filter;

			//applay filter..
			if (output.m_edges)
			{
				for (auto& edge_action : m_edges_actions)
					if (edge_action.m_edge & output.m_edges)
					{
						output.m_edges_actions.push_back(edge_action);
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

	void go_to(size_t new_time)
	{
		if (new_time < m_time) go_back(new_time);
		if (new_time > m_time) go_next(new_time);
	}

	void go_next(size_t n_time)
	{
		//if n_time < m_time fail...
		assert(n_time >= m_time);
		//no change state
		if (n_time == m_time) return;
		//for each update
		for (size_t time = m_time; time != n_time ; ++time)
		{
			update();
		}
	}
    
	void go_back(size_t b_time)
    {
        //if b_time > m_time fail...
        assert(b_time <= m_time);
        //no change state
        if(!m_time)          return;
        if(b_time == m_time) return;
        //go back
        for(size_t l_time = m_time; l_time !=b_time; --l_time)
        {
            applay_old(l_time);
			//remove (?)
			DEALLOC_HISTORY(m_history.erase( l_time );)
        }
        //new corrent time
        m_time = b_time;
    }
    
    void applay_history_edges(size_t time,const edges_history& history)
    {
        //go back if is necessary
        go_to(time);
        //applay
        for(auto& action : history.m_edges_actions)
        {
            if(is_inside(action.m_action.m_position))
            {
                global(action.m_action.m_position) = action.m_action.m_new;
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
    }
    
    edges_history get_last_history_edges(unsigned char filter = ALL)
    {
        return std::move(get_history_edges(m_time,filter));
    }

	edges_history get_history_edges(size_t time, unsigned char filter = ALL)
	{
        //boder type
        unsigned char g_type = 0;
        unsigned char type   = 0;
		//output
		edges_actions output;
		//get actions
		const actions& l_actions = m_history[time];
		//applay
		for (const action& l_action : l_actions)
		{
			//no border
			type = 0;
			//get relative
			point_g relative = l_action.m_position - m_position;
			//position
			if (relative.x == 0)
			{
				type   |= LEFT & filter;
                g_type |= LEFT & filter;
			}
			if (relative.x == m_size.x - 1)
			{
                type   |= RIGHT & filter;
                g_type |= RIGHT & filter;
			}
			if (relative.y == 0)
			{
                type   |= TOP & filter;
                g_type |= TOP & filter;
			}
			if (relative.y == m_size.y - 1)
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
        actions l_actions;
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
                l_actions.push_back(
                action
                {
                    point,
                    global(point),
                    next
                });
            }
        }
        //add
        m_history[m_time] = l_actions;
    }
    
    size_t time() const
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
		point_g pos = p - m_position;
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
            size_t last =  m_time ? m_time -1 : 0;
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
            size_t last =  m_time ? m_time -1 : 0;
            outstring  += line_to_string();
            outstring  += history_to_string( last );
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
    
    void applay_new(size_t time)
    {
        //search
        auto it = m_history.find(time);
        //
        if(it != m_history.end())
        {
            //applay
            for(action& l_action : it->second)
            {
                global(l_action.m_position) = l_action.m_new;
            }
        }
    }
    
    void applay_old(size_t time)
    {
        //search
        auto it = m_history.find(time);
        //
        if(it != m_history.end())
        {
            //applay
            for(action& l_action : it->second)
            {
                global(l_action.m_position) = l_action.m_old;
            }
        }
    }
    
    std::string history_to_string(size_t time)
    {
        std::string outstring;
        //search
        auto it = m_history.find(time);
        //
        if(it != m_history.end())
        {
            //applay
            for(action& l_action : it->second)
            {
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
    
    size_t   m_time { 0 };
    history  m_history;
    point_g  m_position;
    point_g  m_size;
    matrix   m_matrix;
    
};

#endif /* grid_h */
