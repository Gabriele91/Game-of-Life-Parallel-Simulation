//
//  main_client.cpp
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//

#include <stdio.h>
#include <istream>
#include <fstream>
#include <client_gol.h>
#include <json11.hpp>
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
    //client
    client_gol client;
    client.connect(json_object["ip"].string_value(),
                   json_object["port"].int_value() ,
                   json_object["time"].int_value() );
    client.loop();
    //end
    return 0;
}
