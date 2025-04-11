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

#ifndef IOHC_USER_H
#define IOHC_USER_H

#include <board-config.h>

//#define WIFI
#define WIFI_SSID ""
#define WIFI_PASSWD ""

//#define MQTT
#define MQTT_SERVER ""
#define MQTT_USER "user"
#define MQTT_PASSWD "passwd"

#define HTTP_LISTEN_PORT    80
#define HTTP_USERNAME       "admin"
#define HTTP_PASSWORD       "admin"
#define SERIALSPEED         115200 //921600

#endif
