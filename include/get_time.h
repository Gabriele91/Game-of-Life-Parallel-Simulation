//
//  get_time.h
//  Simulazione
//
//  Created by Gabriele on 11/02/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef get_time_h
#define get_time_h


#if defined(_MSC_VER)
    typedef __int64    bit64;
    typedef __int64    byte16;
#else
    typedef long long  bit64;
    typedef long long  byte16;
#endif

extern bit64 get_time_ticks();
extern double get_time();


#endif /* get_time_h */
