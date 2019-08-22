//
// Created by m-chichikalov on 8/21/2019.
//
#include "catch.hpp"

/**
 * Dependencies:
 *
 *   softdds_configRunIQ()        - softdds.h
 *   softdds_runIQ()
 *
 *   CW_KEYER_MODE_IAM_B          - uhsdr_board.h
 *   CW_KEYER_MODE_IAM_A
 *   CW_KEYER_MODE_STRAIGHT
 *   CW_KEYER_MODE_ULTIMATE
 *   Board_PttDahLinePressed()
 *   Board_DitLinePressed()
 *   global struct ts
 *   TRX_MODE_TX
 *   DEMOD_CW
 *
 *   DigiModes_TxBufferRemove()    - uhsdr_digi_buffer.h
 *   DigiModes_TxBufferPutChar()
 *   DigiModes_TxBufferPutSign()
 *   DigiModes_TxBufferReset()
 *   CW
 *
 *   RadioManagement_Request_TxOn  -radio_management.h
 *   RadioManagement_Request_TxOff
 */

struct radio_global_state {
   uint32_t cw_keyer_speed;
   uint32_t cw_keyer_weight;
   uint32_t cw_rx_delay;
   uint32_t samp_rate;
   uint32_t cw_paddle_reverse;
   uint32_t cw_keyer_mode;
   uint32_t txrx_mode;
   uint32_t dmod_mode;
   uint32_t cw_text_entry;
} ts {};

#include "arm_math.h"
void softdds_runIQ( float32_t* i_buff, float32_t* q_buff, uint16_t size ) {};
void softdds_configRunIQ( float32_t freq[2], uint32_t samp_rate, uint8_t smooth ){};

#include "uhsdr_digi_buffer.h"
bool     DigiModes_TxBufferRemove( uint8_t* c_ptr, digi_buff_consumer_t consumer ) {return false;};
int32_t  DigiModes_TxBufferPutChar( uint8_t c, digi_buff_consumer_t source ) {return 0;};
void     DigiModes_TxBufferPutSign( const char* s, digi_buff_consumer_t source ) {};
void     DigiModes_TxBufferReset( ) {};

bool Board_PttDahLinePressed() {return false;};
bool Board_DitLinePressed() {return false;};

void RadioManagement_Request_TxOn() {};
void RadioManagement_Request_TxOff() {};

bool CatDriver_CWKeyPressed() {return false;};
bool CatDriver_CatPttActive() {return false;};

extern "C" {
// prevent from gcc find this header in original folder.
#include "uhsdr_digi_buffer.h"

#include "drivers/audio/cw/cw_gen.h"
#include "drivers/audio/cw/cw_gen.c"
}

SCENARIO( "Dummy." ) {
   REQUIRE( 1 == true );
}