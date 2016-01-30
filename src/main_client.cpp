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
    client_gol client;
    client.connect("127.0.0.1", 3456, 10000);
    client.loop();
    return 0;
}
