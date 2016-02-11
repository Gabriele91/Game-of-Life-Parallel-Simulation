//
//  get_time.cpp
//  Simulazione
//
//  Created by Gabriele on 11/02/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//

#include <stdio.h>
#include <get_time.h>
#if !defined(_MSC_VER)
    #include <sys/time.h>
#else
    #include <windows.h>
#endif
extern bit64 get_time_ticks()
{
    bit64 val;
#if !defined(_MSC_VER)
    timeval timeVal;
    
    gettimeofday( &timeVal, NULL );
    
    val = (bit64)timeVal.tv_sec * (1000*1000) + (bit64)timeVal.tv_usec;
#else
    QueryPerformanceCounter( (LARGE_INTEGER *)&val );
#endif
    return val;
}
//time
extern double get_time()
{
    static double	coe;
#if !defined(_MSC_VER)
    coe = 1.0 / 1000.0;
#else
    static bit64 freq;
    
    if ( freq == 0 )
    {
        QueryPerformanceFrequency( (LARGE_INTEGER *)&freq );
        coe = 1000.0 / freq;
    }
#endif
    
    return get_time_ticks() * coe;
}