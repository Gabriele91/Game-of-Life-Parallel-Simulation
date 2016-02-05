//
//  grid_json.h
//  Simulazione
//
//  Created by Gabriele on 04/02/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once

#include <json11.hpp>
#include <grid.h>

inline grid::matrix get_state(const json11::Json::array& j_state0)
{
    //get matrix
    grid::matrix g_state0;
    //add rows
    for( auto& j_row : j_state0 )
    {
        //add row
        grid::row row;
        //for all
        for( auto& j_value : j_row.array_items() )
        {
            row.push_back(j_value.int_value());
        }
        //...
        g_state0.push_back(row);
    }
    //return
    return std::move(g_state0);
}

inline json11::Json::array get_jarray(const grid::matrix& matrix)
{
    //get matrix
    json11::Json::array j_array;
    //add rows
    for(const grid::row& row : matrix )
    {
        //add row
        json11::Json::array j_row;
        //for all
        for(grid::value value : row)
        {
            j_row.push_back(value);
        }
        //...
        j_array.push_back(j_row);
    }
    //return
    return std::move(j_array);
}

inline json11::Json::array get_jarray_relative_row(const grid::matrix& matrix)
{
    //get matrix
    json11::Json::array j_array;
    //size y
    size_t size_y  = matrix.size();
    //...
    if(!size_y) return j_array;
    //size x
    size_t size_x  = matrix[0].size();
    //add col
    for(size_t x = 1; x != size_x-1 ; ++x )
    {
        //add col
        json11::Json::array j_col;
        //for all
        for(size_t y = 1; y != size_y-1 ; ++y )
        {
            j_col.push_back(matrix[y][x]);
        }
        //...
        j_array.push_back(j_col);
    }
    //return
    return std::move(j_array);
}
inline json11::Json::array get_jarray_relative_col(const grid::matrix& matrix)
{
    //get matrix
    json11::Json::array j_array;
    //size y
    size_t size_y  = matrix.size();
    //...
    if(!size_y) return j_array;
    //size x
    size_t size_x  = matrix[0].size();
    //add rows
    for(size_t y = 1; y != size_y-1 ; ++y )
    {
        //add row
        json11::Json::array j_row;
        //for all
        for(size_t x = 1; x != size_x-1 ; ++x )
        {
            j_row.push_back(matrix[y][x]);
        }
        //...
        j_array.push_back(j_row);
    }
    //return
    return std::move(j_array);
}
