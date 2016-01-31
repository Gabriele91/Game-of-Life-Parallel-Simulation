//
//  main_client.cpp
//  Simulazione
//
//  Created by Gabriele on 30/01/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//

#include <stdio.h>
#include <client_gol.h>
int main(int argc, const char * argv[])
{
#if 1
    client_gol client;
    client.connect("127.0.0.1", 3456, 1000000);
    client.loop();
#else
    grid::point_g::type x=10;
    //grid value
    grid l_grid({x,0},{4*8,4*8});
    //state 0...
    
                       l_grid(1+x,0) = 1;
                                          l_grid(2+x,1) = 1;
    l_grid(0+x,2) = 1; l_grid(1+x,2) = 1; l_grid(2+x,2) = 1;
    //state 0...
    std::cout << l_grid.to_string_borders();
    
    for(int i=0; i< 17; ++i)
    {
        l_grid.update();
        std::cout << l_grid.to_string_borders();
    }
#endif
    return 0;
}
