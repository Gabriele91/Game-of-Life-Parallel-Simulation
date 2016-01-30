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
#include <unordered_map>
#include <array>
#include <vector>
#include <string>
#include <point.h>
#include <assert.h>
#define DEALLOC_HISTORY(x) x

class grid
{
public:


    using point_g  = point_l;
    using value    = unsigned char;
    using row      = std::vector< value >;
    using matrix   = std::vector< row >;
    
    struct action
    {
        point_g m_position;
        value   m_old;
        value   m_new;
    };

    using actions  = std::vector< action >;
    using history  = std::unordered_map< size_t, actions >;

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

    grid(const point_g& position,
         const point_g& size)
    {
        //size
        m_position = position;
        m_size     = size;
        //alloc rows
        m_matrix.resize( m_size.y +2 );
        //alloc colunm
        for(row& a_row:m_matrix)
        {
            a_row.resize( m_size.x +2 );
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

	edges_actions get_history_edges(size_t time, unsigned char filter = ALL)
	{
		//boder type
		unsigned char type;
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
				type |= LEFT & filter;
			}
			if (relative.x == m_size.x - 1)
			{
				type |= RIGHT & filter;
			}
			if (relative.y == 0)
			{
				type |= TOP & filter;
			}
			if (relative.y == m_size.y - 1)
			{
				type |= BOTTOM & filter;
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
		return output;
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
        //
        for(point_g::type y = 0; y != m_size.y; ++y)
        for(point_g::type x = 0; x != m_size.x; ++x)
        {
            //next state
            point_g point(x,y);
            value    next = automa_next(point);
            //add action?
            if(next != global(point))
            {
                m_history[m_time].push_back(
                action
                {
                    point,
                    global(point),
                    next
                });
            }
        }
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
        return m_matrix[p.y-m_position.y+1][p.x+m_position.x+1];
    }
    
    value& global(point_g::type x, point_g::type y)
    {
        return global({x,y});
    }
    
    value& local(const point_g& p)
    {
        return m_matrix[p.y][p.x];
    }
    
    value& local(point_g::type x, point_g::type y)
    {
        return local({x,y});
    }
    
    value& operator ()(const point_g& p)
    {
        return global(p);
    }
    
    value& operator ()(point_g::type x, point_g::type y)
    {
        return global({x, y});
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
                outstring += std::to_string(global({x, y})) +" ";
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
        //get actions
        const actions& l_actions = m_history[time];
        //applay
        for(const action&  l_action: l_actions)
        {
            global(l_action.m_position) = l_action.m_new;
        }
    }
    
    void applay_old(size_t time)
    {
        //get actions
		const actions& l_actions = m_history[time];
        //applay
        for(const action&  l_action: l_actions)
        {
            global(l_action.m_position) = l_action.m_old;
        }
    }
    
    std::string history_to_string(size_t time)
    {
        std::string outstring;
        //get actions
		const actions& l_actions = m_history[m_time];
        //applay
        for(const action&  l_action: l_actions)
        {
            outstring+= "position: " + l_action.m_position.to_string() + "\n";
            outstring+= "old state:" + std::to_string(l_action.m_old) + "\n";
            outstring+= "new state:" + std::to_string(l_action.m_new) + "\n";
        }
        
        return outstring;
    }
    
    std::string line_to_string()
    {
        std::string outstring;
        //compute line
        for(point_g::type y = 0; y != m_size.y; ++y)
        {
            outstring += "- ";
        }
        return outstring + "\n";
    }
    
private:
    
    size_t   m_time { 0 };
    history  m_history;
    point_g  m_position;
    point_g  m_size;
    matrix   m_matrix;
    
};

#endif /* grid_h */
