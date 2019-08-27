//
// Created by m-chichikalov on 8/21/2019.
//
#include "catch.hpp"
#include <algorithm>
#include <iostream>
#include <string>

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

namespace {
// prevent gcc find these headers in original folder.
#include "cw_gen.h"
#include "uhsdr_digi_buffer.h"
#include "uhsdr_board.h"
#include "radio_management.h"
#include "softdds.h"
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
} ts = { .cw_paddle_reverse = 0, .samp_rate = 10,
      .cw_keyer_speed = 20,
      .cw_keyer_weight = 100,
      .cw_rx_delay = 8,
      .cw_keyer_mode = CW_KEYER_MODE_IAM_B,
      .txrx_mode = TRX_MODE_RX,
      .dmod_mode = DEMOD_CW };

void set_ts_default( ) {
    ts.cw_paddle_reverse = 0;
    ts.samp_rate = 10;
    ts.cw_keyer_speed = 20;
    ts.cw_keyer_weight = 100;
    ts.cw_rx_delay = 8;
    ts.cw_keyer_mode = CW_KEYER_MODE_IAM_B;
    ts.txrx_mode = TRX_MODE_RX;
    ts.dmod_mode = DEMOD_CW;
}

#include "drivers/audio/cw/cw_gen.h"
#include "drivers/audio/cw/cw_gen.c"

constexpr uint32_t IQ_BLOCK_SIZE = 32;
float32_t i[IQ_BLOCK_SIZE] = { }, q[IQ_BLOCK_SIZE] = { };
void softdds_runIQ( float32_t*i_buff, float32_t*q_buff, uint16_t size ) {
    (void)i_buff;
    (void)q_buff;
    (void)size;
};
void softdds_configRunIQ( float32_t freq[2], uint32_t samp_rate, uint8_t smooth ) {
    (void)freq;
    (void)samp_rate;
    (void)smooth;
};

void logging_state( uint32_t cnt ) {
    static uint8_t old_state = -1;
    if ( old_state != ps.cw_state ) {
        old_state = ps.cw_state;
        std::cout << "changed to " << ps.cw_state << " on " << cnt << std::endl;
    }
}

uint32_t CwGen_GetBreakTime( );

struct digi_buffer_fixture {
    enum class SEQUENCE { ALL = 1, NEXT, BREAK_TIME };
    std::string in { "" };
    std::string out { "" };
    void reset_state( ) {
        in.clear();
        out.clear();
    };
    void push_in_buffer( const char c ) { in.append( &c ); /*std::cout << in << std::endl;*/ };
    void push_in_buffer( const std::string& str ) { in.append( str ); /*std::cout << in << std::endl;*/ };
    void push_out_buffer( const char c ) { out.append( &c, 1 ); /*std::cout << "out < " << c << std::endl;*/  };
    void push_out_buffer( const char*str ) { out.append( str ); };
    template<typename T>
    uint32_t count_len( T iter, uint32_t size ) {
        uint32_t retval { 0 };
        std::for_each( iter, ( iter + size ), [&retval]( char c ) {
          uint32_t pseudo = 0;
          c = CwGen_TranslateToUperCase( c );
          for ( int idx = 0; idx < CW_CHAR_CODES; idx++ ) {
              if ( cw_char_chars[idx] == c ) {
                  pseudo = CwGen_ReverseCode( cw_char_codes[idx] );
                  break;
              }
          }
          uint32_t len { 0 };
          while ( pseudo > 1 ) {
              assert(( pseudo % 4 ) == 3 || ( pseudo % 4 ) == 2 );
              if ( pseudo % 4 == 3 ) {
                  len += ps.dah_time;
              } else {
                  len += ps.dit_time;
              }
              len += ps.pause_time;
              pseudo /= 4;
          }
          // minuse one extra pause.
          // len -= ps.pause_time;
          retval += len + ps.dah_time + 1;
        } );
        return retval;
    };
    void run( SEQUENCE seq ) {
        uint32_t run_cnt { 0 };
        if ( seq == SEQUENCE::ALL ) {
            run_cnt = count_len( in.begin(), in.size()) + ps.space_time;
        } else if ( seq == SEQUENCE::NEXT ) {
            run_cnt = count_len( in.begin(), 1 );
        } else if ( seq == SEQUENCE::BREAK_TIME ) {
            run_cnt = CwGen_GetBreakTime();
        } else {
            assert( false );
        }
        run( run_cnt );
    };
    void run( uint32_t run_cnt ) {
//        std::cout << "run_cnt: " << run_cnt << std::endl;
//        std::cout << "in_buf: " << in << std::endl;
        for ( uint32_t idx = 1; idx < run_cnt+1; ++idx ) {
            CwGen_Process( i, q, IQ_BLOCK_SIZE );
//            logging_state( idx );
        }
    };
} digi_buffer;

using SEQUENCE = digi_buffer_fixture::SEQUENCE;

bool DigiModes_TxBufferRemove( uint8_t*c_ptr, digi_buff_consumer_t consumer ) {
    CHECK( consumer == CW );
    if ( digi_buffer.in.empty())
        return false;
    *c_ptr = digi_buffer.in[0];
    digi_buffer.in.erase( digi_buffer.in.begin());
    return true;
}
int32_t DigiModes_TxBufferPutChar( uint8_t c, digi_buff_consumer_t source ) {
    CHECK( source == CW );
    digi_buffer.push_out_buffer( c );
    return 1;
}
void DigiModes_TxBufferPutSign( const char*s, digi_buff_consumer_t source ) {
    CHECK( source == CW );
    digi_buffer.push_out_buffer( s );
}
void DigiModes_TxBufferReset( ) { digi_buffer.out.clear(); }

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
void set_cat_PTT( bool ptt ) {
    cat.ptt_active = ptt;
    ts.txrx_mode = ptt;
}
bool CatDriver_CWKeyPressed( ) { return cat.cw_key_down; }
bool CatDriver_CatPttActive( ) { return cat.ptt_active; }

} // namespace

SCENARIO( "Straight key: ", "[]" ) {
    GIVEN( "ts: default except txrx_mode: TRX_MODE_TX, cw_keyer_mode: STRAIGHT." ) {
        set_ts_default();
        digi_buffer.reset_state();
        set_dah( false );
        set_dit( false );
        CwGen_Init();
        RadioManagement_Request_TxOn();
        ts.cw_keyer_mode = CW_KEYER_MODE_STRAIGHT;

        WHEN( "No paddles are pressed " ) {
            set_dah( false );
            set_dit( false );
            THEN( "Process() return false." ) {
                CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr );
                CHECK( CwGen_Process( i, q, 2 ) == false );
            }
        }

        WHEN( "Dah pressed" ) {
            set_dah( true );
            THEN( "Process() return true." ) {
                CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr );
                CHECK( CwGen_Process( i, q, 2 ) == true );
            }
        }

        WHEN( "Afer Dah pressed and released" ) {
            set_dah( true );
            CwGen_Process( i, q, 32 );
            set_dah( false );
            THEN( "TRX stays in TX for ps.break_timer and come to RX after." ) {
                // Number of calls required to pass hardly depends on size of blocks, SMOOTH table size and SMOOTH_len.
                uint32_t timer = ps.break_timer + 2 * ( CW_SMOOTH_TBL_SIZE * CW_SMOOTH_LEN ) / IQ_BLOCK_SIZE
                      - 1/*as we already call once*/;
                for ( uint32_t idx = 0; idx < timer; ++idx ) {
                    CwGen_Process( i, q, IQ_BLOCK_SIZE );
                }
                CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
            THEN( "Process() return true for 2*(CW_SMOOTH_TBL_SIZE*CW_SMOOTH_LEN)/IQ_BLOCK_SIZE calls, and false for break_timer." ) {
                uint32_t timer =
                      2 * ( CW_SMOOTH_TBL_SIZE * CW_SMOOTH_LEN ) / IQ_BLOCK_SIZE - 1 /*as we already call once*/;
                for ( uint32_t idx = 0; idx < timer; ++idx ) {
                    CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr, idx );
                    CHECK( CwGen_Process( i, q, IQ_BLOCK_SIZE ) == true );
                }

                timer = ps.break_timer;
                for ( uint32_t idx = 0; idx < timer; ++idx ) {
                    CAPTURE( ps.key_timer, ps.break_timer, ps.sm_tbl_ptr, idx );
                    CHECK( CwGen_Process( i, q, IQ_BLOCK_SIZE ) == false );
                }
            }
        }

        WHEN( "The pressing Dit" ) {
            set_dit( true );
            CwGen_Process( i, q, 32 );
            set_dit( false );
            THEN( "shouldn't affect anything in STRAIGTH mode." ) {
                CHECK( CwGen_Process( i, q, 32 ) == false );
            }
        }
    }
}

SCENARIO( "CAT interface keying CW: ", "[]" ) {
    GIVEN( "" ) {
        digi_buffer.reset_state();
        WHEN( "" ) {
            THEN( "" ) {

            }
        }
    }
}

SCENARIO( "Iambic A CW mode: ", "[]" ) {
    GIVEN( "Default settings: keyMode IAMBIC_A." ) {
        digi_buffer.reset_state();
        set_ts_default();
        set_dah( false );
        set_dit( false );
        CwGen_Init();
        ts.cw_keyer_mode = CW_KEYER_MODE_IAM_A;
        ts.txrx_mode = TRX_MODE_TX;
        WHEN( "First Dah, next Dit" ) {
            /** @todo > Implement method to count run numbers for particular string. */
            set_dah( true );
            CwGen_Process( i, q, 32 );
            set_dit( true );
            THEN( " after it runs enough to generate (_._.) digibuffer should contain C." ) {
                uint32_t c_time = (( ps.dit_time + ps.dah_time) + 2*ps.pause_time)*2;
                digi_buffer.run( c_time - ps.pause_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "C" );
            }
            AND_THEN( " after it runs enough to generate (_.) digibuffer should contain N." ) {
                uint32_t c_time = (( ps.dit_time + ps.dah_time) + 2*ps.pause_time)*1;
                digi_buffer.run( c_time - ps.pause_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "N" );
            }
            AND_THEN( " after it runs enough to generate (_._) digibuffer should contain K." ) {
                uint32_t c_time = (( ps.dit_time + ps.dah_time) + 2*ps.pause_time) + ( ps.dah_time + ps.pause_time );
                digi_buffer.run( c_time - ps.pause_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "K" );
            }
        }
        WHEN( "First Dit, next Dah" ) {
            /** @todo > Implement method to count run numbers for particular string. */
            set_dit( true );
            CwGen_Process( i, q, 32 );
            set_dah( true );
            THEN( " after it runs enough to generate A digibuffer should contain A." ) {
                uint32_t c_time = (( ps.dit_time + ps.dah_time ) + 2*ps.pause_time );
                digi_buffer.run( c_time - ps.pause_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "A" );
            }
            AND_THEN( " after it runs enough to generate R digibuffer should contain R." ) {
                uint32_t c_time = (( ps.dit_time + ps.dah_time) + 2*ps.pause_time) + ( ps.dit_time + ps.pause_time );
                digi_buffer.run( c_time - ps.pause_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "R" );
            }
        }
        WHEN("first pressed Dit") {
            set_dit( true );
            THEN( " after it runs enough to generate (..), Dah pressed and,"
                  "runs for dah (_) and after released, runs for one more (.): digibuffer should contain 'F'." ) {
                digi_buffer.run( ( ps.pause_time + ps.dit_time )*2 );
                set_dah(true );
                digi_buffer.run( ps.dah_time );
                set_dah( false );
                digi_buffer.run( ps.pause_time + ps.dit_time );
                set_dit(false );
                digi_buffer.run( ps.pause_time + 1 );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "F" );
            }
        }
        WHEN("first pressed Dah") {
            set_dah( true );
            THEN( " after it runs enough to generate (__), Dit pressed and,"
                  "runs for (.) and after released, runs for one more (_): digibuffer should contain 'Q'." ) {
                digi_buffer.run( ( ps.pause_time + ps.dah_time )*2 );
                set_dit(true );
                digi_buffer.run( ps.dit_time );
                set_dit( false );
                digi_buffer.run( ps.pause_time + ps.dah_time );
                set_dah(false );
                digi_buffer.run( ps.pause_time + 1 );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "Q" );
            }
        }
    }
}

SCENARIO( "Iambic B CW mode: ", "[]" ) {
    GIVEN( "" ) {
        digi_buffer.reset_state();
        set_ts_default();
        set_dah( false );
        set_dit( false );
        CwGen_Init();
        ts.cw_keyer_mode = CW_KEYER_MODE_IAM_B;
        ts.txrx_mode = TRX_MODE_TX;
        WHEN( "first Dah, next Dit" ) {
            set_dah( true );
            CwGen_Process( i, q, 32 );
            set_dit( true );
            THEN( " after it runs enough to generate (_._), levers released,"
                  "run for one more (.), digibuffer should contain C." ) {
                digi_buffer.run( ps.dah_time + ps.pause_time + ps.dit_time + ps.pause_time + ps.dah_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.pause_time + ps.dit_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "C" );
            }
            THEN( " after it runs enough to generate (_.), levers released,"
                  "run for one more (_), digibuffer should contain 'K'." ) {
                digi_buffer.run( ps.dah_time + ps.pause_time + ps.dit_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "K" );
            }
        }
        WHEN("first Dit, next Dah") {
            set_dit( true );
            CwGen_Process( i, q, 32 );
            set_dah( true );
            THEN( " after it runs enough to generate (._), levers released,"
                  "run for one more (.), digibuffer should contain R." ) {
                digi_buffer.run( ps.dah_time + ps.pause_time + ps.dit_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.pause_time + ps.dit_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "R" );
            }
            THEN( " after it runs enough to generate (._._.), levers released,"
                  "run for one more (_), digibuffer should contain '.'" ) {
                digi_buffer.run( (ps.dit_time + ps.pause_time)*2 + (ps.dah_time + ps.pause_time)*2 + ps.dit_time );
                set_dah(false);
                set_dit(false);
                digi_buffer.run( ps.pause_time + ps.pause_time + ps.dah_time );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "." );
            }
        }
        WHEN("first pressed Dit") {
            set_dit( true );
            THEN( " after it runs enough to generate (..), Dah pressed and,"
                  "runs for dah (_) and after released, runs for one more (.): digibuffer should contain 'F'." ) {
                digi_buffer.run( ( ps.pause_time + ps.dit_time )*2 );
                set_dah(true );
                digi_buffer.run( ps.dah_time );
                set_dah( false );
                digi_buffer.run( ps.pause_time + ps.dit_time );
                set_dit(false );
                digi_buffer.run( ps.pause_time + 1 );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "F" );
            }
        }
        WHEN("first pressed Dah") {
            set_dah( true );
            THEN( " after it runs enough to generate (__), Dit pressed and,"
                  "runs for (.) and after released, runs for one more (_): digibuffer should contain 'Q'." ) {
                digi_buffer.run( ( ps.pause_time + ps.dah_time )*2 );
                set_dit(true );
                digi_buffer.run( ps.dit_time );
                set_dit( false );
                digi_buffer.run( ps.pause_time + ps.dah_time );
                set_dah(false );
                digi_buffer.run( ps.pause_time + 1 );

                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "Q" );
            }
        }
    }
}

SCENARIO( "Ultimatic CW mode: ", "[]" ) {
    GIVEN( "Tx mode, default state, all levers are released" ) {
        set_ts_default();
        set_dah( false );
        set_dit( false );
        CwGen_Init();
        ts.cw_keyer_mode = CW_KEYER_MODE_ULTIMATE;
        ts.txrx_mode = TRX_MODE_TX;
        digi_buffer.reset_state();
        WHEN( "paddles depressed to imitate 'P'" ) {
            set_dit( true );
            digi_buffer.run( ps.dit_time );
            set_dah( true );
            digi_buffer.run( (ps.dah_time + ps.pause_time)*2 );
            set_dah( false );
            digi_buffer.run( ps.dit_time + ps.pause_time );
            set_dit( false );
            digi_buffer.run( ps.pause_time*2 );
            THEN( "the digibuffer should contains 'P'" ) {
                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char,
                      ps.ultim, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "P" );
            }
        }
        WHEN( "paddles depressed to imitate 'X'" ) {
            set_dah( true );
            digi_buffer.run( ps.dah_time );
            set_dit( true );
            digi_buffer.run( (ps.dit_time + ps.pause_time)*2 );
            set_dit( false );
            digi_buffer.run( ps.dah_time + ps.pause_time );
            set_dah( false );
            digi_buffer.run( ps.pause_time*2 );
            THEN( "the digibuffer should contains 'X'" ) {
                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char,
                         ps.ultim, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.out == "X" );
            }
        }
    }
}

/**
 * As the GenCw module generate output pseudo code which should be the same as input one,
 * it could be used to test almost every internal functionality.
 */
SCENARIO( "Feeding CW from DigiBuffer: ", "[]" ) {
    GIVEN( "" ) {
        digi_buffer.reset_state();
        set_ts_default();
        set_dah( false );
        set_dit( false );
        CwGen_Init();
        ts.cw_keyer_mode = CW_KEYER_MODE_IAM_A;
        WHEN( "" ) {

            digi_buffer.push_in_buffer( "HELLO" );
            digi_buffer.run( SEQUENCE::ALL );

            THEN( "" ) {
                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char );
                CHECK( digi_buffer.out == "HELLO " );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
        }
        WHEN( "" ) {
            ts.cw_keyer_mode = CW_KEYER_MODE_ULTIMATE;

            digi_buffer.push_in_buffer( "HELLO" );
            digi_buffer.run( SEQUENCE::ALL );

            THEN( "" ) {
                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char );
                CHECK( digi_buffer.out == "HELLO " );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
        }
        WHEN( "space at the end of macro it shouldn't stuck in TX and " ) {

            digi_buffer.push_in_buffer( "E   E  " );
            /** @todo > add method to count required runs for buffer with spaces within. */
            digi_buffer.run( 10000 ); // should be enough to "eat all buffer"

            THEN( "IN buffer should be empty, and mode RX." ) {
                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.in == "" );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
        }
        WHEN( "Ultimatic mode; space at the end of macro it shouldn't stuck in TX and " ) {
            ts.cw_keyer_mode = CW_KEYER_MODE_ULTIMATE;

            digi_buffer.push_in_buffer( "E   E  " );
            /** @todo > add method to count required runs for buffer with spaces within. */
            digi_buffer.run( 10000 ); // should be enough to "eat all buffer"

            THEN( "IN buffer should be empty, and mode RX." ) {
                CAPTURE( ps.space_timer, ps.cw_state, ps.break_timer, ps.port_state, ps.sending_char, digi_buffer.in, digi_buffer.out );
                CHECK( digi_buffer.in == "" );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
        }
        WHEN( "" ) {
            digi_buffer.push_in_buffer( "OLLEH" );

            digi_buffer.run( SEQUENCE::NEXT );
            THEN( "" ) {
                CHECK( digi_buffer.out == "O" );
                CHECK( ts.txrx_mode == TRX_MODE_TX );

                digi_buffer.run( SEQUENCE::NEXT );
                CHECK( digi_buffer.out == "OL" );
                CHECK( ts.txrx_mode == TRX_MODE_TX );

                digi_buffer.run( SEQUENCE::NEXT );
                CHECK( digi_buffer.out == "OLL" );
                CHECK( ts.txrx_mode == TRX_MODE_TX );

                digi_buffer.run( SEQUENCE::NEXT );
                CHECK( digi_buffer.out == "OLLE" );
                CHECK( ts.txrx_mode == TRX_MODE_TX );

                digi_buffer.run( SEQUENCE::NEXT );
                CAPTURE( ps.cw_state, ps.space_timer, ps.break_timer );
                CHECK( digi_buffer.out == "OLLEH" );
                CHECK( ts.txrx_mode == TRX_MODE_TX );

                digi_buffer.run( SEQUENCE::BREAK_TIME );
                CHECK( ts.txrx_mode == TRX_MODE_RX );
            }
        }
    }
}
