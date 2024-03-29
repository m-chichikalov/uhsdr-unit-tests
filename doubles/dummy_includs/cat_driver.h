//
// Created by m-chichikalov on 8/21/2019.
//
#ifndef __CAT_DRIVER_H
#define __CAT_DRIVER_H

// #include "uhsdr_types.h"

typedef enum
{
    CAT_DISCONNECTED = 0,
    CAT_CONNECTED
} CatInterfaceState;

typedef enum
{
    UNKNOWN = 0,
    FT817 = 1
} CatInterfaceProtocol;

CatInterfaceState CatDriver_GetInterfaceState();
int CatDriver_InterfaceBufferAddData(uint8_t c);
void CatDriver_HandleProtocol();
bool CatDriver_CloneOutStart();
bool CatDriver_CloneInStart();
bool CatDriver_CWKeyPressed();
bool CatDriver_CatPttActive();

#endif