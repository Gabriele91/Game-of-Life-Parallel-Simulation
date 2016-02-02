//
//  modulo.h
//  Simulazione
//
//  Created by Gabriele on 01/02/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#ifndef modulo_h
#define modulo_h

namespace positive
{
    inline static float mod(float dividend,float divider)
    {
        float  mod = std::fmod(dividend,divider);
        return mod < 0 ? mod+divider : mod;
    }
    
    inline static float mod(double dividend,double divider)
    {
        double mod = std::fmod(dividend,divider);
        return mod < 0 ? mod+divider : mod;
    }
    
    inline static float mod(char dividend,char divider)
    {
        double mod = dividend % divider;
        return mod < 0 ? mod+divider : mod;
    }
    
    inline static float mod(short dividend,short divider)
    {
        double mod = dividend % divider;
        return mod < 0 ? mod+divider : mod;
    }
    
    inline static float mod(int dividend,int divider)
    {
        double mod = dividend % divider;
        return mod < 0 ? mod+divider : mod;
    }
    
    inline static float mod(long dividend,long divider)
    {
        double mod = std::fmod(dividend,divider);
        return mod < 0 ? mod+divider : mod;
    }
}

#endif /* config_h */
