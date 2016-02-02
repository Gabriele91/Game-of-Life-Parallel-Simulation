//
//  main.cpp
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <grid.h>
#include <rak_listener.h>
#include <server_gol.h>
#include <json11.hpp>

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

int main(int argc, const char * argv[])
{
    
    //error message
    if(argc < 2){ std::cout << "Not valid input" << std::endl; return -1; }
    //load json
    std::ifstream file(argv[1]);
    //test file
    if(!file.is_open()){ std::cout << "Can't open json file" << std::endl; return -1; }
    //get text
    std::string json_text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    //errors
    std::string err_comment;
    //get json
    auto json_object = json11::Json::parse(json_text, err_comment, json11::JsonParse::COMMENTS);
    //errors
    if (!err_comment.empty())
    {
        std::cout << "Failed: " << err_comment.c_str() << std::endl;
        return -1;
    }
    //get json
    auto cluster = json_object["cluster"];
    //client
    server_gol server
    {
        {
            cluster["size"][0].int_value(),
            cluster["size"][1].int_value(),
        },
        {
            cluster["clients"][0].int_value(),
            cluster["clients"][1].int_value(),
        },
        (size_t)cluster["steps"].int_value(),
                cluster["cicle"].bool_value()
    };
    //state 0
    server.set_state0(get_state(cluster["state0"].array_items()));
    //set state
    server.open(json_object["port"].int_value() ,
                json_object["time"].int_value() );
    server.loop();
    //end
    return 0;
}
