//
//  point.h
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef point_h
#define point_h

template< class T >
class point
{
public:
    
    typedef T type;
    
    union
    {
        T x;
        T m_x { static_cast<T>(0.0) };
    };
    
    union
    {
        T y;
        T m_y { static_cast<T>(0.0) };
    };
    
    point(T x = static_cast<T>(0.0),
          T y = static_cast<T>(0.0))
    {
        m_x=x;
        m_y=y;
    }
    
    bool operator ==  (const point& point) const
    {
        return x == point.x && y == point.y;
    }
    
    bool operator !=  (const point& point) const
    {
        return x != point.x || y != point.y;
    }
    
    point operator + (const point& point) const
    {
        return
        {
            x+point.x,
            y+point.y
        };
    }
    
    point operator - (const point& point) const
    {
        return
        {
            x-point.x,
            y-point.y
        };
    }
    
    point operator * (const point& point) const
    {
        return
        {
            x*point.x,
            y*point.y
        };
    }
    
    point operator / (const point& point) const
    {
        return
        {
            x/point.x,
            y/point.y
        };
    }
    
    point& operator += (const point& point)
    {
        x+=point.x;
        y+=point.y;
        return (*this);
    }
    
    point& operator -= (const point& point)
    {
        x-=point.x;
        y-=point.y;
        return (*this);
    }
    
    point& operator *= (const point& point)
    {
        x*=point.x;
        y*=point.y;
        return (*this);
    }
    
    point& operator /= (const point& point)
    {
        x/=point.x;
        y/=point.y;
        return (*this);
    }
    
    std::string to_string() const
    {
        return std::to_string(x)+ ", " + std::to_string(y);
    }
};

using point_d = point< double > ;
using point_f = point< float > ;
using point_s = point< short > ;
using point_i = point< int > ;
using point_l = point< long > ;

#endif /* point_h */
