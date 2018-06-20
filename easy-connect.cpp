/*
 * FILE: easy-connect.cpp
 *
 * Copyright (c) 2015 - 2017 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "easy-connect.h"

#if MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR_WNC14A2A
#include "WNC14A2AInterface.h"
#if MBED_CONF_WNC14A2A_LIBRARY_WNC_DEBUG == true
#include "WNCDebug.h"
WNCDebug dbgout(stderr);
WNC14A2AInterface wnc(&dbgout);
#else
WNC14A2AInterface wnc;
#endif

#else
/*
 * Instantiate the configured network interface
 */
#include MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE_HEADER
MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE_CLASS iff;

#if (MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == WIFI_ESP8266 \
     || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == WIFI_ODIN \
     || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == WIFI_RTW \
     || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == WIFI_IDW0XX1 \
     || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == WIFI_ISM43362 \
     || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == WIFI_WIZFI310)

#include "WiFiInterface.h"

#define EASY_CONNECT_WIFI
#endif // MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE

#endif

/*
 * In case of Mesh, instantiate the configured RF PHY.
 */
#if MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_LOWPAN_ND || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_THREAD

#if MBED_CONF_EASY_CONNECT_MESH_RADIO_TYPE == ATMEL
#include "NanostackRfPhyAtmel.h"
NanostackRfPhyAtmel rf_phy(ATMEL_SPI_MOSI, ATMEL_SPI_MISO, ATMEL_SPI_SCLK, ATMEL_SPI_CS,
                           ATMEL_SPI_RST, ATMEL_SPI_SLP, ATMEL_SPI_IRQ, ATMEL_I2C_SDA, ATMEL_I2C_SCL);

#elif MBED_CONF_EASY_CONNECT_MESH_RADIO_TYPE == MCR20
#include "NanostackRfPhyMcr20a.h"
NanostackRfPhyMcr20a rf_phy(MCR20A_SPI_MOSI, MCR20A_SPI_MISO, MCR20A_SPI_SCLK, MCR20A_SPI_CS, MCR20A_SPI_RST, MCR20A_SPI_IRQ);

#elif MBED_CONF_EASY_CONNECT_MESH_RADIO_TYPE == SPIRIT1
#include "NanostackRfPhySpirit1.h"
NanostackRfPhySpirit1 rf_phy(SPIRIT1_SPI_MOSI, SPIRIT1_SPI_MISO, SPIRIT1_SPI_SCLK,
                             SPIRIT1_DEV_IRQ, SPIRIT1_DEV_CS, SPIRIT1_DEV_SDN, SPIRIT1_BRD_LED);

#elif MBED_CONF_EASY_CONNECT_MESH_RADIO_TYPE == EFR32
#include "NanostackRfPhyEfr32.h"
NanostackRfPhyEfr32 rf_phy;

#endif // MBED_CONF_EASY_CONNECT_MESH_RADIO_TYPE
#endif //MESH_LOWPAN_ND || MESH_THREAD

#if defined (EASY_CONNECT_WIFI)
#define WIFI_SSID_MAX_LEN      32    // As per IEEE 802.11 chapter 7.3.2.1 (SSID element)
#define WIFI_PASSWORD_MAX_LEN  64    //

char* _ssid = NULL;
char* _password = NULL;
#endif // EASY_CONNECT_WIFI

/* \brief print_MAC - print_MAC  - helper function to print out MAC address
 * in: network_interface - pointer to network i/f
 *     bool log-messages   print out logs or not
 * MAC address is printed, if it can be acquired & log_messages is true.
 *
 */
void print_MAC(NetworkInterface* network_interface, bool log_messages) {
#if MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE != CELLULAR_ONBOARD && MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE != CELLULAR
    const char *mac_addr = network_interface->get_mac_address();
    if (mac_addr == NULL) {
        if (log_messages) {
            printf("[EasyConnect] ERROR - No MAC address\n");
        }
        return;
    }
    if (log_messages) {
        printf("[EasyConnect] MAC address %s\n", mac_addr);
    }
#endif
}

static void setup_connectivity();
static void print_connectivity_messages();

/* \brief easy_connect     easy_connect() function to connect the pre-defined network bearer,
 *                         config done via mbed_app.json (see README.md for details).
 *
 * IN: bool  log_messages  print out diagnostics or not.
 */
NetworkInterface* easy_connect(bool log_messages) {
    NetworkInterface* network_interface = NULL;
    int connect_success = -1;

    setup_connectivity();

    if (log_messages) {
        print_connectivity_messages();
    }

#if MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR_WNC14A2A
    network_interface = &wnc;
#else
    network_interface = &iff;
#endif

    connect_success = network_interface->connect();

    if(connect_success == 0) {
        if (log_messages) {
            printf("[EasyConnect] Connected to Network successfully\n");
            print_MAC(network_interface, log_messages);
        }
    } else {
        if (log_messages) {
            print_MAC(network_interface, log_messages);
            printf("[EasyConnect] Connection to Network Failed %d!\n", connect_success);
        }
        return NULL;
    }
    const char *ip_addr  = network_interface->get_ip_address();
    if (ip_addr == NULL) {
        if (log_messages) {
            printf("[EasyConnect] ERROR - No IP address\n");
        }
        return NULL;
    }

    if (log_messages) {
        printf("[EasyConnect] IP address %s\n", ip_addr);
    }
    return network_interface;
}

/* \brief easy_connect - easy_connect function to connect the pre-defined network bearer,
 *                       config done via mbed_app.json (see README.md for details).
 *                       This version is just a helper version and uses the easy_connect() with
 *                       one parameters to do it's job.
 * IN: bool  log_messages  print out diagnostics or not.
 *     char* WiFiSSID      WiFi SSID - pointer to WiFi SSID, but if it is NULL
 *                         then MBED_CONF_EASY_CONNECT_WIFI_SSID will be used
 *     char* WiFiPassword  WiFi Password - pointer to WiFI password, but if it's NULL
 *                         then MBED_CONF_EASY_CONNECT_WIFI_PASSWORD will be used
 */

NetworkInterface* easy_connect(bool log_messages,
                               char* WiFiSSID,
                               char* WiFiPassword ) {

// This functionality only makes sense when using WiFi
#if defined (EASY_CONNECT_WIFI)
    // We essentially want to populate the _ssid and _password and then call easy_connect() again.
    if (WiFiSSID != NULL) {
        if(strlen(WiFiSSID) > WIFI_SSID_MAX_LEN) {
            printf("[EasyConnect] ERROR - WiFi SSID is too long - %d vs %d.\n", strlen(WiFiSSID), WIFI_SSID_MAX_LEN);
            return NULL;
        }
        _ssid = WiFiSSID;
    }

    if (WiFiPassword != NULL) {
        if(strlen(WiFiPassword) > WIFI_PASSWORD_MAX_LEN) {
            printf("[EasyConnect] ERROR - WiFi Password is too long - %d vs %d\n", strlen(WiFiPassword), WIFI_PASSWORD_MAX_LEN);
            return NULL;
        }
        _password = WiFiPassword;
    }
    return easy_connect(log_messages);
#else // EASY_CONNECT_WIFI
    (void)WiFiSSID;
    (void)WiFiPassword;
    if (log_messages) {
        printf("[EasyConnect] ERROR - Wifi not in use, can not return WifiInterface.\n");
    }
    return NULL;
#endif
}

/* \brief easy_get_netif - easy_connect function to get pointer to network interface
 *                        without connecting to it.
 *
 * IN: bool  log_messages  print out diagnostics or not.
 */
NetworkInterface* easy_get_netif(bool log_messages) {
#if MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR_WNC14A2A
    if (log_messages) {
        printf("[EasyConnect] WNC14A2A\n");
    }
    return  &wnc;
#else
    if (log_messages) {
        printf("[EasyConnect] %s\n", MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE_NAME);
    }
    return &iff;

#endif
}

/* \brief easy_get_wifi - easy_connect function to get pointer to Wifi interface
 *                        without connecting to it. You would want this 1st so that
 *                        you can scan the APNs, choose the right one and then connect.
 *
 * IN: bool  log_messages  print out diagnostics or not.
 */
WiFiInterface* easy_get_wifi(bool log_messages) {
#if defined (EASY_CONNECT_WIFI)
    if (log_messages) {
        printf("[EasyConnect] WiFi: %s\n", MBED_CONF_EASY_CONNECT_WIFI_TYPE);
    }
    return (WiFiInterface*)(&iff);
#else
    if (log_messages) {
        printf("[EasyConnect] ERROR - Wifi not in use, can not return WifiInterface.\n");
    }
    return NULL;
#endif
}

static void print_connectivity_messages() {

    /// This should be removed once mbedOS supports proper dual-stack
#if (MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_LOWPAN_ND \
    || MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_THREAD \
    || (MBED_CONF_LWIP_IPV6_ENABLED==true))
    printf("[EasyConnect] IPv6 mode\n");
#else
    printf("[EasyConnect] IPv4 mode\n");
#endif

#if defined (EASY_CONNECT_WIFI)

    printf("[EasyConnect] Using WiFi (%s) \n", MBED_CONF_EASY_CONNECT_WIFI_TYPE);
    printf("[EasyConnect] Connecting to WiFi %s\n",
           ((_ssid == NULL) ? MBED_CONF_EASY_CONNECT_WIFI_SSID : _ssid) );

#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR_ONBOARD || \
      MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR

    #ifdef MBED_CONF_EASY_CONNECT_CELLULAR_APN
        printf("[EasyConnect] Connecting using Cellular interface and APN %s\n", MBED_CONF_EASY_CONNECT_CELLULAR_APN);
    #else
        printf("[EasyConnect] Connecting using Cellular interface and default APN\n");
    #endif

#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_THREAD || \
      MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_LOWPAN_ND
    printf("[EasyConnect] Using Mesh (%s)\n", MBED_CONF_EASY_CONNECT_MESH_TYPE);
    printf("[EasyConnect] Connecting to Mesh...\n");

#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == ETHERNET
    printf("[EasyConnect] Using Ethernet\n");

#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR_WNC14A2A
    printf("[EasyConnect] Using WNC14A2A\n");

    #if MBED_CONF_WNC14A2A_LIBRARY_WNC_DEBUG == true
        printf("[EasyConnect] With WNC14A2A debug output set to 0x%02X\n",MBED_CONF_WNC14A2A_LIBRARY_WNC_DEBUG_SETTING);
        wnc.doDebug(MBED_CONF_WNC14A2A_LIBRARY_WNC_DEBUG_SETTING);
    #endif

#endif

}

static void setup_connectivity() {

#if defined (EASY_CONNECT_WIFI)
    // We check if the _ssid and _password have already been set (via the easy_connect()
    // that takes thoses parameters or not.
    // If they have not been set, use the ones we can gain from mbed_app.json.
    if (_ssid == NULL) {
        if(strlen(MBED_CONF_EASY_CONNECT_WIFI_SSID) > WIFI_SSID_MAX_LEN) {
            printf("ERROR - MBED_CONF_EASY_CONNECT_WIFI_SSID is too long %d vs. %d\n",
                    strlen(MBED_CONF_EASY_CONNECT_WIFI_SSID),
                    WIFI_SSID_MAX_LEN);
            return;
        }
    }

    if (_password == NULL) {
        if(strlen(MBED_CONF_EASY_CONNECT_WIFI_PASSWORD) > WIFI_PASSWORD_MAX_LEN) {
            printf("ERROR - MBED_CONF_EASY_CONNECT_WIFI_PASSWORD is too long %d vs. %d\n",
                    strlen(MBED_CONF_EASY_CONNECT_WIFI_PASSWORD),
                    WIFI_PASSWORD_MAX_LEN);
            return;
        }
    }
    if (_ssid == NULL) {
        ((WiFiInterface*)(&iff))->set_credentials(MBED_CONF_EASY_CONNECT_WIFI_SSID, MBED_CONF_EASY_CONNECT_WIFI_PASSWORD,
                                                        (strlen(MBED_CONF_EASY_CONNECT_WIFI_PASSWORD) > 1) ? NSAPI_SECURITY_WPA_WPA2 : NSAPI_SECURITY_NONE);
    } else {
        ((WiFiInterface*)(&iff))->set_credentials(_ssid, _password,
                                                        (strlen(_password) > 1) ? NSAPI_SECURITY_WPA_WPA2 : NSAPI_SECURITY_NONE);
    }

#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR_ONBOARD || \
    MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == CELLULAR

    #ifdef MBED_CONF_EASY_CONNECT_CELLULAR_SIM_PIN
        ((CellularBase*)(&iff))->set_sim_pin(MBED_CONF_EASY_CONNECT_CELLULAR_SIM_PIN);
    #endif

    #ifdef MBED_CONF_EASY_CONNECT_CELLULAR_APN

        #ifndef MBED_CONF_EASY_CONNECT_CELLULAR_USERNAME
        #define MBED_CONF_EASY_CONNECT_CELLULAR_USERNAME 0
        #endif

        #ifndef MBED_CONF_EASY_CONNECT_CELLULAR_PASSWORD
        #define MBED_CONF_EASY_CONNECT_CELLULAR_PASSWORD 0
        #endif

        ((CellularBase*)(&iff))->set_credentials(MBED_CONF_EASY_CONNECT_CELLULAR_APN,
                                                       MBED_CONF_EASY_CONNECT_CELLULAR_USERNAME,
                                                       MBED_CONF_EASY_CONNECT_CELLULAR_PASSWORD);
    #endif

#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_LOWPAN_ND
    static_cast<LoWPANNDInterface>(iff).initialize(&rf_phy);
#elif MBED_CONF_EASY_CONNECT_NETWORK_INTERFACE == MESH_THREAD
    static_cast<ThreadInterface>(iff).initialize(&rf_phy);
#endif
}
