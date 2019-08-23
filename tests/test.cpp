//
// Created by m-chichikalov on 8/21/2019.
//
#include "catch.hpp"
#include <iostream>

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

extern "C" {
// prevent gcc find these headers in original folder.
#include "uhsdr_digi_buffer.h"
#include "uhsdr_board.h"
#include "radio_management.h"
#include "softdds.h"
#include "cw_gen.h"
#include "cat_driver.h"
#include "ui_driver.h"

struct radio_global_state {
    uint8_t cw_paddle_reverse;
    uint32_t samp_rate;

    uint8_t cw_keyer_speed; // default 20  ( 5..48 )
    uint8_t cw_keyer_weight; // default 100  ( 50..150 )
    uint8_t cw_rx_delay; // default 8  (0..50) ??
    uint8_t cw_keyer_mode;

    uint8_t txrx_mode;
    uint8_t dmod_mode;
    bool cw_text_entry;
} ts = {  .cw_paddle_reverse = 0, .samp_rate = 10,
          .cw_keyer_speed = 20,
          .cw_keyer_weight = 100,
          .cw_rx_delay = 8,
          .cw_keyer_mode = CW_KEYER_MODE_IAM_B,
          .txrx_mode = TRX_MODE_RX,
          .dmod_mode = DEMOD_CW };

void set_ts_default() {
    ts.cw_paddle_reverse = 0; ts.samp_rate = 10; ts.cw_keyer_speed = 20; ts.cw_keyer_weight = 100;
    ts.cw_rx_delay = 8; ts.cw_keyer_mode = CW_KEYER_MODE_IAM_B; ts.txrx_mode = TRX_MODE_RX;
    ts.dmod_mode = DEMOD_CW;
}

void softdds_runIQ( float32_t* i_buff, float32_t* q_buff, uint16_t size ) { };
void softdds_configRunIQ( float32_t freq[2], uint32_t samp_rate, uint8_t smooth ) { };

bool DigiModes_TxBufferRemove( uint8_t* c_ptr, digi_buff_consumer_t consumer ) { *c_ptr = 'K'; return true; };
int32_t DigiModes_TxBufferPutChar( uint8_t c, digi_buff_consumer_t source ) { return 0; };
void DigiModes_TxBufferPutSign( const char* s, digi_buff_consumer_t source ) { };
void DigiModes_TxBufferReset( ) { };

struct paddle_state {
    bool dit_pressed;
    bool dah_pressed;
} paddles { false, false };

void set_dah( bool state ) { paddles.dah_pressed = state; }
void set_dit( bool state ) { paddles.dit_pressed = state; }
bool Board_PttDahLinePressed( ) { return paddles.dah_pressed; };
bool Board_DitLinePressed( ) { return paddles.dit_pressed; };

void RadioManagement_Request_TxOn( ) { ts.txrx_mode = TRX_MODE_TX; };
void RadioManagement_Request_TxOff( ) { ts.txrx_mode = TRX_MODE_RX; };

struct cat_state {
    bool cw_key_down;
    bool ptt_active;
} cat { false, false };

void set_cat_CW( bool cw ) { cat.cw_key_down = cw; }
void set_cat_PTT( bool ptt ) { cat.ptt_active = ptt; ts.txrx_mode = ptt; }
bool CatDriver_CWKeyPressed( ) { return cat.cw_key_down; }
bool CatDriver_CatPttActive( ) { return cat.ptt_active; }

#include "drivers/audio/cw/cw_gen.h"
#include "drivers/audio/cw/cw_gen.c"
}

uint32_t get_length( char c )
{
    uint32_t pseudo = 0;
    c = CwGen_TranslateToUperCase( c );
    for ( int i = 0; i < CW_CHAR_CODES; i++ )
    {
        if (cw_char_chars[i] == c)
        {
            pseudo = CwGen_ReverseCode(cw_char_codes[i]);
            break;
        }
    }

    uint32_t length {0};

    while ( pseudo > 1)
    {
        assert(( pseudo % 4 ) == 3 || ( pseudo % 4 ) == 2 );

        if ( pseudo % 4 == 3 ) {
            length += ps.dah_time;
        } else {
            length += ps.dit_time;
        }
        pseudo /= 4;
    }

    return length;
}

template<typename TFunc>
void call_multipal_times( uint32_t cnt, TFunc&& f )
{
    for ( uint32_t i = 0; i < cnt; ++i )
    {
        bool result = f();
        std::cout << result << std::endl;
    }
}

void logging_state( uint32_t cnt ) {
    static uint8_t old_state = -1;
    if ( old_state != ps.cw_state )
    {
        old_state = ps.cw_state;
        std::cout << "changed to " << ps.cw_state << " on " << cnt << std::endl;
    }
}

constexpr uint32_t IQ_BLOCK_SIZE = 32;
float32_t i[IQ_BLOCK_SIZE] = {}, q[IQ_BLOCK_SIZE] = {};

SCENARIO( "Dummy.", "[.]" ) {
    CwGen_Init();
    ts.cw_keyer_speed = 21;
    CwGen_SetSpeed();
    uint32_t run_cnt = get_length( 'k' )*10;
    for ( uint32_t idx = 0; idx < run_cnt; idx++ )
    {
        logging_state( idx );
        CwGen_Process( i, q, 2 );
    }
    CAPTURE( ts.cw_keyer_mode, ts.cw_keyer_speed, ts.cw_keyer_weight, ts.cw_text_entry, ps.cw_state, ps.dit_time, ps.dah_time, ps.pause_time, ps.space_time );

    CHECK( get_length( 'k' ) == 2 );
}

SCENARIO("Straight key: ", "[]") {
    GIVEN("ts: default except txrx_mode: TRX_MODE_TX, cw_keyer_mode: STRAIGHT.") {
        set_ts_default();
        set_dah( false );
        set_dit( false );
        CwGen_Init();
        RadioManagement_Request_TxOn();
        ts.cw_keyer_mode = CW_KEYER_MODE_STRAIGHT;

        WHEN("No paddles are pressed ") {
            set_dah( false );
            set_dit( false );
            THEN("Process() return false.") {
                CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr );
                CHECK( CwGen_Process( i, q, 2 ) == false );
            }
        }

        WHEN("Dah pressed") {
            set_dah( true );
            THEN("Process() return true.") {
                CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr );
                CHECK( CwGen_Process( i, q, 2 ) == true );
            }
        }

        WHEN("Afer Dah pressed and released") {
            set_dah( true );
            CwGen_Process( i, q, 32 );
            set_dah( false );
            THEN("TRX stays in TX for ps.break_timer and come to RX after.") {
                // Number of calls required to pass hardly depends on size of blocks, SMOOTH table size and SMOOTH_len.
                uint32_t timer = ps.break_timer + 2*(CW_SMOOTH_TBL_SIZE*CW_SMOOTH_LEN)/IQ_BLOCK_SIZE - 1/*as we already call once*/;
                for ( uint32_t idx = 0; idx < timer; ++idx )
                {
                    CwGen_Process( i, q, IQ_BLOCK_SIZE );
                }
                CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
        }

        WHEN("After Dah pressed and released") {
            set_dah( true );
            CwGen_Process( i, q, 32 );
            set_dah( false );
            THEN("Process() return true for 2*(CW_SMOOTH_TBL_SIZE*CW_SMOOTH_LEN)/IQ_BLOCK_SIZE calls, and false for break_timer.") {
                uint32_t timer = 2*(CW_SMOOTH_TBL_SIZE*CW_SMOOTH_LEN)/IQ_BLOCK_SIZE - 1 /*as we already call once*/;
                for ( uint32_t idx = 0; idx < timer; ++idx )
                {
                    CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr, idx );
                    CHECK( CwGen_Process( i, q, IQ_BLOCK_SIZE ) == true );
                }

                timer = ps.break_timer;
                for ( uint32_t idx = 0; idx < timer; ++idx )
                {
                    CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr, idx );
                    CHECK( CwGen_Process( i, q, IQ_BLOCK_SIZE ) == false );
                }
            }
        }

        WHEN("The pressing Dit") {
            set_dit( true );
            CwGen_Process( i, q, 32 );
            set_dit( false );
            THEN("doesn't affect anything in STRAIGTH mode.") {
                CHECK( CwGen_Process( i, q, 32 ) == false );
            }
        }
    }
}

SCENARIO("CAT interface keying CW: ", "[]") {
    GIVEN("") {
        WHEN("") {
            THEN("") {

            }
        }
    }
}

SCENARIO("Iambic A CW mode: ", "[]") {
    GIVEN("") {
        WHEN("") {
            THEN("") {

            }
        }
    }
}

SCENARIO("Iambic B CW mode: ", "[]") {
    GIVEN("") {
        WHEN("") {
            THEN("") {

            }
        }
    }
}

SCENARIO("Ultimatic CW mode: ", "[]") {
    GIVEN("") {
        WHEN("") {
            THEN("") {

            }
        }
    }
}


SCENARIO("Feeding CW sm from DigiBuffer: ", "[]") {
    GIVEN("") {
        WHEN("") {
            THEN("") {

            }
        }
    }
}