//
// Created by m-chichikalov on 8/21/2019.
//
#ifndef __CW_GEN_H
#define __CW_GEN_H

#include "arm_math.h"

// Exports
void    CwGen_Init();
void    CwGen_PrepareTx();
void    CwGen_SetSpeed();
bool    CwGen_TimersActive();
bool    CwGen_Process( float32_t *i_buffer, float32_t *q_buffer, uint32_t size );
void    CwGen_DahIRQ();
void    CwGen_DitIRQ();
uint8_t CwGen_CharacterIdFunc( uint32_t );

#endif
