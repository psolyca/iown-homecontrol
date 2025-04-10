/*
   Copyright (c) 2024. CRIDP https://github.com/cridp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef IOHC_BOARD_H
#define IOHC_BOARD_H

#define Regulatory_Domain_EU_868
/*
 * Board pins definitions
 * See _env.ini files
 */

#define RADIO_MOSI                      RADIO_MOSI_PIN
#define RADIO_MISO                      RADIO_MISO_PIN
#define RADIO_SCLK                      RADIO_SCLK_PIN
#define RADIO_NSS                       RADIO_CS_PIN
#define RADIO_RESET                     RADIO_RST_PIN
// Radio Pin
#define RADIO_DIO_0                     RADIO_DIO0_PIN
#define RADIO_DIO_1                     RADIO_DIO1_PIN
#define RADIO_DIO_2                     RADIO_DIO2_PIN
#define RADIO_DIO_4                     RADIO_DIO4_PIN

#define RADIO_PAYLOAD_READY             RADIO_DIO_0     // Payload Ready
#define RADIO_FIFO_EMPTY                RADIO_DIO_1     // FIFO (not) empty from Radio
#define RADIO_RXTIMEOUT                 RADIO_DIO_2     // Radio Rx Sequencer timeout (used to switch the receiver frequency)
#define RADIO_ADDRESS_SYNC              RADIO_DIO_2     // Address sync
#define RADIO_PREAMBLE_DETECTED         RADIO_DIO_4     // Preamble detected from Radio (used instead of FIFO empty)

#define SPI_CLK_FRQ                     10000000

/*
 * Defines the time required for the TCXO to wakeup [ms].
 */

#define BOARD_TCXO_WAKEUP_TIME          0
#define BOARD_READY_AFTER_POR           10000

#define PREAMBLE_MSB                    0x00
#define PREAMBLE_LSB                    64      // 52  // 0x34: 12ms to have receiver up and running (52 0x55 bytes - 13,54mS)

#define SYNC_BYTE_1                     0xff
#define SYNC_BYTE_2                     0x33    // Sync word - Size must be set to 2; first byte 0xff then 0x33 size-1 times

//#define SYNC_BYTE_2_ENC               0xB3    // Sync word Inverted + Encoded with start & stop bits

#define CHANNEL1                        868250000 //2W
#define CHANNEL2                        868950000 //1W 2W
#define CHANNEL3                        869850000 //2W

#define FREQS2SCAN                      {CHANNEL2, CHANNEL1, CHANNEL3}
#define MAX_FREQS                       1       // Number of Frequencies to scan through Fast Hopping set to 1 to disable FHSS

#define SCAN_LED                        BOARD_LED_PIN // 22
#define RX_LED                          SCAN_LED

#endif
