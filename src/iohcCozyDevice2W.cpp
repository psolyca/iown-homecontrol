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

#include <iohcCozyDevice2W.h>
#include <LittleFS.h>
#include <iohcCryptoHelpers.h>
#include <numeric>

namespace IOHC {
    iohcCozyDevice2W* iohcCozyDevice2W::_iohcCozyDevice2W = nullptr;

    iohcCozyDevice2W::iohcCozyDevice2W() = default;

    iohcCozyDevice2W* iohcCozyDevice2W::getInstance() {
        if (!_iohcCozyDevice2W) {
            _iohcCozyDevice2W = new iohcCozyDevice2W();
            _iohcCozyDevice2W->load();
            _iohcCozyDevice2W->initializeValid();
        }
        return _iohcCozyDevice2W;
    }

    /**
    * @brief Forge IOHC Cozy Device 2 packet. This function is called by iohcCozyDevice2W :: forgePacket
    * @param packet * Pointer to the IOHC packet to forge
    * @param toSend Vector of data to put in the I / O
    */
    void iohcCozyDevice2W::forgePacket(iohcPacket* packet, const std::vector<unsigned char> &toSend) {
        digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
        IOHC::relStamp = esp_timer_get_time();

        // Common Flags
        // 8 if protocol version is 0 else 10
        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen = sizeof(_header) - 1;
        packet->payload.packet.header.CtrlByte1.asStruct.Protocol = 0;
        packet->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
        packet->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
        packet->payload.packet.header.CtrlByte2.asByte = 0;

        packet->payload.packet.header.CtrlByte1.asByte += toSend.size();
        memcpy(packet->payload.buffer + 9, toSend.data(), toSend.size());
        packet->buffer_length = toSend.size() + 9;

        packet->frequency = CHANNEL2;
        packet->repeatTime = 25;
        packet->repeat = 0; 
        packet->lock = false; 
    }

    /**
    * @brief Checks if this cozy is a fake. This is used to detect if we have an IOCHA device that is in charge of the IOCHA and should be woken up.
    * @param nodeSrc The source node address of the IOCHA.
    * @param nodeDst The destination node address of the IOCHA.
    * @return true if this device is a fake false otherwise. Note that this device is not a wake
    */
    bool iohcCozyDevice2W::isFake(address nodeSrc, address nodeDst) {
        this->Fake = false;
        // Fake to ensure that the node is in the same node src and dst.
        if (!memcmp(this->gateway, nodeSrc, 3) || !memcmp(this->gateway, nodeDst, 3)) { this->Fake = true; }
        return this->Fake;
    }

    /// Emulates device button press
    void iohcCozyDevice2W::cmd(DeviceButton cmd, Tokens* data) {

        if (!_radioInstance) {
            Serial.println("NO RADIO INSTANCE");
            _radioInstance = IOHC::iohcRadio::getInstance();
        } 

        switch (cmd) {
            case DeviceButton::associate: {
                std::vector<uint8_t> toSend = {};

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_ASK_CHALLENGE_0x31;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = iohcDevice::SEND_ASK_CHALLENGE_0x31;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_to, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); 
                break;
            }
            case DeviceButton::powerOn: {
                std::vector<uint8_t> toSend = {0x0C, 0x60, 0x01, 0x2C};

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_to, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); 

                break;
            }
            case DeviceButton::setTemp: {
                std::vector<uint8_t> toSend = {0x0C, 0x61, 0x01, 0x03, 0xFF, 0x00};

                int temp = 10 * std::stof(data->at(1));
                toSend[4] = temp;

                int addr = 0;
                if (data->size() == 2) addr = 0;
                else addr = std::stoi(data->at(2));

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, addresses.at(addr).data()/* 0 Master_to*/, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                packets2send.back()->delayed = 50;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                //                mqttClient.publish("iown/Frame", 0, false, message.c_str(), messageSize);

                break;
            }
            case DeviceButton::setMode: {
                std::vector<uint8_t> toSend = {0x0C, 0x61, 0x01, 0x00, 0xFF};

                const char* dat = data->at(1).c_str();
                if (strcasecmp(dat, "auto") == 0) toSend[4] = 0x00;
                if (strcasecmp(dat, "manual") == 0) toSend[4] = 0x01;
                if (strcasecmp(dat, "prog") == 0) toSend[4] = 0x02;
                // if (strcasecmp(data, "special") == 0) toSend[4] = 0x03;
                if (strcasecmp(dat, "off") == 0) toSend[4] = 0x04; // TODO if mode off, disable setPresence

                // int addr = 0;
                // if (data->size() == 2) addr = 0;
                // else addr = std::stoi(data->at(2));

                size_t dest = 0;

                packets2send.clear();
                for (const auto &addr: addresses) {
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                    memorizeSend.memorizedData = toSend;
                    memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                    packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();
                    memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                    memcpy(packets2send.back()->payload.packet.header.target, addresses.at(dest/*addr*/).data()/* 0 Master_to*/, 3);

//                    memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                    packets2send.back()->buffer_length = toSend.size() + 9;

                    dest++;
                }
                packets2send[1]->delayed = 250;
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                _radioInstance->send(packets2send);

                break;
            }
            case DeviceButton::setPresence: {
                std::vector<uint8_t> toSend = {0x0C, 0x61, 0x01, 0x10, 0xFF};
 
                const char* dat = data->at(1).c_str();
                if (strcasecmp(dat, "on") == 0) toSend[4] = 0x01;
                if (strcasecmp(dat, "off") == 0) toSend[4] = 0x00;

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_to, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); 
                break;
            }
            case DeviceButton::setWindow: {
                std::vector<uint8_t> toSend = {0x0C, 0x61, 0x01, 0x0E, 0xFF};

                const char* dat = data->at(1).c_str();
                if (strcasecmp(dat, "open") == 0) toSend[4] = 0x01;
                if (strcasecmp(dat, "close") == 0) toSend[4] = 0x00;

                int addr = 0;
                if (data->size() == 2) addr = 0;
                else addr = std::stoi(data->at(2));

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
//                memcpy(packets2send.back()->payload.packet.header.target, master_to, 3);
                memcpy(packets2send.back()->payload.packet.header.target, addresses.at(addr).data()/* 0 Master_to*/, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;
                packets2send.back()->delayed = 50;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); 
                break;
            }
            case DeviceButton::midnight: {
                // std::vector<uint8_t> toSend = {0x00, 0x0c, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, 0x53};
                std::vector<uint8_t> toSend = {0x0c, 0x60, 0x01, 0x30}; //, 0x2b, 0x05, 0x00, 0x0f, 0x04, 0x0c, 0xe7, 0x07};

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_to, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); 
                
                break;
            }
            case DeviceButton::custom: {
                std::vector<uint8_t> toSend =  {0x01, 0x47, 0xc8, 0x00, 0x00, 0x00}; //{0x03, 0x65, 0xd4, 0x00, 0x00, 0x00}; //{0x0C, 0x60, 0x01, 0xFF, 0xFF};
                //const char* dat = data->at(1).c_str();
                for (int acei = 0; acei < 256; acei++) {
 //               int custom = std::stoi(data->at(1));
    AceiUnion ACEI{};
    ACEI.asByte = acei;
    // Only other ACEI are valids, other give answer: 0xFE 0x58
    if (!ACEI.asStruct.isvalid || ACEI.asStruct.service != 0) continue; 

                toSend[1] = acei; //custom;
// toSend[2] = acei;

                uint8_t from[3] = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //
                uint8_t to[3] = {0xda, 0x2e, 0xe6}; //
                uint8_t to_1[3] = {0x05, 0x4e, 0x17}; //{0x31, 0x58, 0x24}; //

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = 0x00; //SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = 0x00; //SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                memcpy(packets2send.back()->payload.packet.header.source, from/*gateway*/, 3);
                memcpy(packets2send.back()->payload.packet.header.target, to_1, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                packets2send.back()->delayed = 250; // Give enough time for the answer

                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); // Verify !
                break;
            }
            case DeviceButton::custom60: {
                std::vector<uint8_t> toSend =  {0x0C, 0x60, 0x01, 0xFF};
                // Accepted command {0x0C, 0x61, 0x01, 0xFF, FF};
                const char* dat = data->at(1).c_str();
 
//                for (int custom = 0; custom < 256; custom++) {
                int custom = std::stoi(data->at(1));
 
                toSend[3] = custom; //custom;

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeSend.memorizedData = toSend;
                memorizeSend.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                memcpy(packets2send.back()->payload.packet.header.source, gateway/*master_from*/, 3);
                memcpy(packets2send.back()->payload.packet.header.target, slave_to, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;
                
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                packets2send.back()->delayed = 250;
                // packets2send.back()->repeat = 1;
//                }
               _radioInstance->send(packets2send); // Verify !
                break;
            }
            case DeviceButton::discover28: {
                std::vector<uint8_t> toSend = {};

                //                uint8_t broadcast[3];
                uint8_t broadcast[3] = {0x00, 0xFF, 0xFB}; //{0x02, 0x02, 0xFB}; //data->at(1).c_str();
                //            hexStringToBytes(dat, broadcast);
                packets2send.clear();
                size_t i = 0;
                for (i = 0; i < 10; i++) {
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send[i], toSend);

                    packets2send[i]->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_0x28; 
                    memorizeSend.memorizedData = toSend;
                    memorizeSend.memorizedCmd = iohcDevice::SEND_DISCOVER_0x28;

                    packets2send[i]->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                    packets2send[i]->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;
//                    packets2send[i]->payload.packet.header.CtrlByte1.asByte += toSend.size();

                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                    memcpy(packets2send[i]->payload.packet.header.source, gateway, 3);
                    memcpy(packets2send[i]->payload.packet.header.target, broadcast, 3);

//                    memcpy(packets2send[i]->payload.buffer + 9, toSend.data(), toSend.size());
//                    packets2send[i]->buffer_length = toSend.size() + 9;

                    packets2send[i]->delayed = 250; // Give enough time for the answer
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                break;
            }
            case DeviceButton::discover2A: {
                /*std::vector<uint8_t>*/
                std::vector<uint8_t>  toSend = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12};

                // uint8_t broadcast[3];
                // const char* dat = data->at(1).c_str();
                // hexStringToBytes(dat, broadcast);
                //                uint8_t broadcast[3];
                uint8_t broadcast_1[3] = {0x00, 0xFF, 0xFB}; //data->at(1).c_str();
                uint8_t broadcast_2[3] = {0x00, 0x0d, 0x3b};
                //                hexStringToBytes(dat, broadcast);
                packets2send.clear();
                size_t i = 0;
                for (i = 0; i < 20; i++) {
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_REMOTE_0x2A;

                    memorizeSend.memorizedData = toSend; //.assign(toSend, toSend + 12);
                    memorizeSend.memorizedCmd = iohcDevice::SEND_DISCOVER_REMOTE_0x2A;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;
//                    packets2send.back()->payload.packet.header.CtrlByte1.asByte += sizeof(toSend); ///*.size()*/ + 8;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
if( i < 10)
    memcpy(packets2send.back()->payload.packet.header.target, broadcast_2/*associated_to*/, 3);
else
    memcpy(packets2send.back()->payload.packet.header.target, broadcast_1, 3);

//                    memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size()); // 12);
//                    packets2send.back()->buffer_length = toSend.size() /*sizeof(toSend)*/ + 9;
                    packets2send.back()->delayed = 250; // Give enough time for the answer
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                
                _radioInstance->send(packets2send); 
                
                break;
            }
            case DeviceButton::fake0: {
                // 09:54:36.226 > (14) 2W S 1 E 0  FROM 0842E3 TO 14E00E CMD 00, F868.950 s+0.000   >  DATA(06)  03 e7 00 00 00 00
//                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                /*std::vector<uint8_t>*/
                std::vector<uint8_t>  toSend = {0x03, 0xe7, 0x32, 0x00, 0x00, 0x00}; //{0x03, 0x00, 0x00}; //  Not good for Cmd 0x01 Answer FE 0x10

                uint8_t gateway[3] = {0xba, 0x11, 0xad}; //{0x08, 0x42, 0xe3};
                uint8_t from[3] = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //

                // Those are the local reals discovered device, can be huge, and must but put in 2W.json
                address guessed[15] = {
                    {0x2D, 0xBE, 0x8D}, {0xDA, 0x2E, 0xE6}, {0x31, 0x58, 0x24}, {0x20, 0xE5, 0x2E}, {0x14, 0xe0, 0x0e},
                    {0x05, 0x4E, 0x17}, {0x1C, 0x68, 0x58}, {0x90, 0x4c, 0x09}, {0xfe, 0x90, 0xee}, {0x41, 0x56, 0x84}, {0x08, 0x42, 0xe3},
                    {0x47, 0x77, 0x06}, {0x48, 0x79, 0x02}, {0x8C, 0xCB, 0x30}, {0x8C, 0xCB, 0x31}
                };

                packets2send.clear();
                size_t i = 0;
                for (i = 0; i < 15; i++) {
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = 0x00;
                    memorizeSend.memorizedData = toSend; // .assign(toSend, toSend + sizeof(toSend));
                    memorizeSend.memorizedCmd = 0x00;
                    IOHC::lastSendCmd = 0x00;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                    packets2send.back()->payload.packet.header.CtrlByte1.asByte += sizeof(toSend);

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Unk1 = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, from/*gateway*/, 3);
                    memcpy(packets2send.back()->payload.packet.header.target, guessed[i], 3);

//                    memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size()); //  sizeof(toSend));
//                    packets2send.back()->buffer_length = sizeof(toSend) + 9;

                    packets2send.back()->delayed = 250; // Give enough time for the answer
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                
                break;
            }
            case DeviceButton::ack: {
                std::vector<uint8_t> toSend = {};

                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_KEY_TRANSFERT_ACK_0x33;
                memorizeSend.memorizedCmd = iohcDevice::SEND_KEY_TRANSFERT_ACK_0x33;
                memorizeSend.memorizedData = toSend;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_from, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;
 
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send); 
                break;
            }
            case DeviceButton::checkCmd: {
                std::vector<uint8_t> toSend;
                // = {}; //{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; //, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12};
                uint8_t special12[] = {
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21
                };
                uint8_t specacei[] = {0x01, 0xe7, 0x00, 0x00, 0x00, 0x00};
                // uint8_t broadcast[3];
                // const char* dat = data->at(1).c_str();
                // hexStringToBytes(dat, broadcast);
                //                uint8_t broadcast[3];
                address from/*[3]*/ = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //
                address to_0/*[3]*/ = {0xda, 0x2e, 0xe6}; //
                address to_1/*[3]*/ = {0x05, 0x4e, 0x17}; //{0x31, 0x58, 0x24}; //
                address to_2/*[3]*/ = {0x9a, 0x50, 0x65};
                address to_3/*[3]*/ = {0x31, 0x58, 0x24};
                //{0x47, 0x77, 0x06}, {0x48, 0x79, 0x02}, {0x8C, 0xCB, 0x30}, {0x8C, 0xCB, 0x31}
                address broad/*[3]*/ = {0x00, 0x0d, 0x3b}; //{0x00, 0xFF, 0xFB}; //

                uint8_t counter = 0;
                packets2send.clear();
                for (const auto&command: mapValid) {
                    if (command.second == 0 || (command.second == 5 && command.first != 0x19)) {
                        counter++;

// 0x00,  0x01, 0x03, 0x0a, 0x0c, 0x19, 0x1e, 0x20, 0x23, 0x28, 0x2a(12), 0x2c, 0x2e, 0x31, 0x32(16), 0x36, 0x38(6), 0x39, 0x3c(6), 0x46(9), 0x48(9), 0x4a(18), 0x4b 
// ,0x50, 0x52(16), 0x54, 0x56, 0x60(21), 0x64(2), 0x6e(9), 0x6f(9), 0x71, 0x73(3), 0x80, 0x82(21), 0x84, 0x86, 0x88, 0x8a(18), 0x8b(1), 0x8e, 0x90, 0x92(16), 0x94, 0x96(12), 0x98

                        if (command.first == 0x00 || command.first == 0x01 || command.first == 0x0B || command.first == 0x0E || command.first == 0x23 || command.first == 0x2A || command.first == 0x1E) toSend.assign(specacei, specacei + 6);
                        if (command.first == 0x8B || command.first == 0x19) toSend.assign(special12, special12 + 1);
                        if (command.first == 0x04) toSend.assign(special12, special12 + 14);
                        uint8_t special03[] = {0x03, 0x00, 0x00};
                        if (command.first == 0x03 || command.first == 0x73) toSend.assign(special03, special03 + 3);
                        uint8_t special0C[] = {0xD8, 0x00, 0x00, 0x00};
                        if (command.first == 0x0C) toSend.assign(special0C, special0C + 4);
                        uint8_t special0D[] = {0x05, 0xaa, 0x0d, 0x00, 0x00};
                        if (command.first == 0x0D) toSend.assign(special0D, special0D + 5);
                        if (command.first == 0x64 || command.first == 0x14) toSend.assign(special12, special12 + 2);
                        if (command.first == 0x2A || command.first == 0x96) toSend.assign(special12, special12 + 12);
                        if (command.first == 0x38 || command.first == 0x3C || command.first == 0x3D) toSend.assign(special12, special12 + 6);
                        if (command.first == 0x32 || command.first == 0x52 || command.first == 0x92) toSend.assign(special12, special12 + 16);
                        if (command.first == 0x46 || command.first == 0x48 || command.first == 0x6E || command.first == 0x6F) toSend.assign(special12, special12 + 9);
                        if (command.first == 0x4A || command.first == 0x8A) toSend.assign(special12, special12 + 18);
                        if (command.first == 0x60 || command.first == 0x82) toSend.assign(special12, special12 + 21);

                        packets2send.push_back(new iohcPacket);
                        forgePacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = command.first;
                        memorizeSend.memorizedCmd = packets2send.back()->payload.packet.header.cmd;

                        packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                        packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;
                        
//                        packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                        memcpy(packets2send.back()->payload.packet.header.source, gateway/*from*//*gateway*/, 3);
                        // if (command.first == 0x14 || command.first == 0x19 || command.first == 0x1e || command.first == 0x2a || command.first == 0x34 || command.first == 0x4a) {
                            // memcpy(packets2send.back()->payload.packet.header.target, broad, 3);
                        // } else {    
                        memcpy(packets2send.back()->payload.packet.header.target, from/*master_to*/, 3);
                        // }
                        
//                        memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                        packets2send.back()->buffer_length = toSend.size() + 9;

                        packets2send.back()->delayed = 245;
                    }
                    toSend.clear();
                }
                Serial.printf("valid %u\n", counter);
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                
                _radioInstance->send(packets2send);
                
                break;
            }
            default: break;
        }
        IOHC::packetStamp = esp_timer_get_time();
        //        save(); // Save Cozy associated devices
    }

    /* Initialise all valids commands for scanMode(checkCmd), other arent implemented in 2W devices 
     00 04 - 01 04 - 03 04 - 0a 0D - 0c 0D - 19 1a - 1e fe - 20 21 - 23 24 - 28 29 - 2a(12) 2b - 2c 2d - 2e 2f - 31 3c - 32(16) 33 - 36 37 - 38(6) 32 - 39 fe - 3c(6) 3d - 46(9) 47 - 48(9) 49 - 4a(18) 4b 
     50 51 - 52(16) 53 - 54 55 - 56 57 - 60(21) .. - 64(2) 65 - 6e(9) fe - 6f(9) .. - 71 72 - 73(3) .. - 80 81 - 82(21) .. - 84  85 - 86 87 - 88 89 - 8a(18) 8c - 8b(1) 8c - 8e .. - 90 91 - 92(16) 93 - 94 95 - 96(12) 97 - 98 99
    */
    void iohcCozyDevice2W::initializeValid() {

        size_t validKey = 0;
        auto valid = std::vector<uint8_t>(256);
        std::iota(valid.begin(), valid.end(), 0);

valid = {
0x00,  0x01, 0x03, 0x0a, 0x0c, 0x19, 0x1e, 0x20, 0x23, 0x28, 0x2a, 0x2c, 0x2e, 0x31, 0x32, 0x36, 0x38, 0x39, 0x3c, 0x46, 0x48, 0x4a, 0x4b, 
0x50, 0x52, 0x54, 0x56, 0x60, 0x64, 0x6e, 0x6f, 0x71, 0x73, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8b, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98,
//Not in firmware 02 0e 25 30 34 3a 3d 58 
 0x02, 0x0b, 0x0e, 0x14, 0x16, 0x25, 0x30, 0x34, 0x3a, 0x3d, 0x58};

        for (int key: valid) {
            // validKey++;
            mapValid[key] = 0;
        }
        // printf("ValidKey: %d\n", validKey);
        // printf("MapValid size: %zu\n", mapValid.size());
    }

    /**
    * @brief Dump the scan result to the console for debugging purposes. \ ingroup iohcCozy
    */
    void iohcCozyDevice2W::scanDump() {
        printf("*********************** Scan result ***********************\n");

        uint8_t count = 0;

        for (auto &it: mapValid) {
            // Prints the first two bytes of the second.
            // Prints the token and argument.
            if (it.second != 0x08) {
                // Prints the first and second of the token.
                // Prints the argument string representation of the argument.
                if(it.second == 0x3C)
                    printf("%2.2x=AUTH ", it.first, it.second);
                // Prints the string representation of the argument.
                // Prints the string representation of the argument.
                else if(it.second == 0x80)
                    printf("%2.2x=NRDY ", it.first, it.second);
                else
                    printf("%2.2x=%2.2x\t", it.first, it.second);
                count++;
                // Prints the number of bytes to the console.
                // Prints the number of bytes to the console.
                if (count % 16 == 0) printf("\n");
            }
        }

        // Prints the number of bytes to the console.
        if (count % 16 != 0) printf("\n");
        
        printf("%u toCheck \n", count);
    }

    /**
    * @brief Load Cozy 2W settings from file and store in _radioInstance.
    * @return True if successful false otherwise. This is a blocking call
    */
    bool iohcCozyDevice2W::load() {
        _radioInstance = iohcRadio::getInstance();
        // Load Cozy 2W device settings from file
        if (LittleFS.exists(COZY_2W_FILE))
            Serial.printf("Loading Cozy 2W devices settings from %s\n", COZY_2W_FILE);
        else {
            Serial.printf("*2W Cozy devices not available\n");
            return false;
        }

        fs::File f = LittleFS.open(COZY_2W_FILE, "r", true);
        JsonDocument doc; 
        deserializeJson(doc, f);
        f.close();

        // Iterate through the JSON object
        for (JsonPair kv: doc.as<JsonObject>()) {
            device d;
            hexStringToBytes(kv.key().c_str(), d._node);
            auto jobj = kv.value().as<JsonObject>();
            d._type = jobj["type"].as<std::string>();
            d._description = jobj["description"].as<std::string>();
            //     hexStringToBytes(jobj["key"].as<const char*>(), _key);
            hexStringToBytes(jobj["dst"].as<const char *>(), d._dst);
            //     uint8_t btmp[2];
            //     hexStringToBytes(jobj["sequence"].as<const char*>(), btmp);
            //            /*hexStringToBytes*/(jobj["type"].as<const char*>(), _type);
            //     _sequence = (btmp[0]<<8)+btmp[1];
            //     JsonArray jarr = jobj["type"];
            //     // Iterate through the JSON array
            //     for (uint8_t i=0; i<jarr.size(); i++)
            //         _type.insert(_type.begin()+i, jarr[i].as<uint16_t>());
            //     _manufacturer = jobj["manufacturer_id"].as<uint8_t>();
            devices.push_back(d);
        }
        Serial.printf("Loaded %d x 2W devices\n", devices.size()); // _type.size());

        return true;
    }
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
    bool iohcCozyDevice2W::save() {
        fs::File f = LittleFS.open(COZY_2W_FILE, "a+");
        JsonDocument doc; 
        for (const auto&d: devices) {
            // JsonObject jobj = doc.createNestedObject(bytesToHexString(_node, sizeof(_node)));
            auto jobj = doc[bytesToHexString(d._node, sizeof(d._node))].to<JsonObject>();
            //        jobj["key"] = bytesToHexString(_key, sizeof(_key));
            jobj["dst"] = bytesToHexString(d._dst, sizeof d._dst);

            jobj["type"] = d._type;
            jobj["description"] = d._description;

            //        uint8_t btmp[2];
            //        btmp[1] = _sequence & 0x00ff;
            //        btmp[0] = _sequence >> 8;
            //        jobj["sequence"] = bytesToHexString(btmp, sizeof(btmp));

            //        jobj["_type"] = _type;

            //        JsonArray jarr = jobj.createNestedArray("type");
            //        for (uint8_t i=0; i<_type.size(); i++)
            //            if (_type[i])
            //                jarr.add(_type.at(i));
            //            else
            //                break;
            //        jobj["manufacturer_id"] = _manufacturer;
        }
        serializeJsonPretty/*serializeJson*/(doc, f);
        f.close();

        return true;
    }
}
