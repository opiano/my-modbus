/* SPDX-License-Identifier: GPL-2.0-or-later */

#define _POSIX_C_SOURCE 200809L

/*
 * Copyright 2023 KUNBUS GmbH
 */

#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>

//#include <common_define.h>
#include "project.h"
#include "modbusconfig.h"

#include "ModbusMasterThread.h"
#include "simpleConfigParser/simpleConfigParser.h"
// #include <piTest/piControlIf.h>
#define DEBUG_MODE

#include <unistd.h>
#include <stdlib.h>

int g_iDebugLevel = 0;

int main(int argc, char *argv[])
{
    int i, modbusDevicesCount;
    pthread_t *pThreads = NULL;
    struct TMBMasterConfigEntry *mbMasterConfigListEntry;

    int opt;
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        if (opt == 'd') {
            g_iDebugLevel = atoi(optarg);
        }
    }
    
    STDERR = stdout;    // show all messages on stdout

    //open syslog
    openlog("piModbusMaster", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "piModbusMaster started\n");
    
    while (1) {
        //create list for all modbus master configurations stored in pictory config file
        SLIST_INIT(&mbMasterConfHead);
    
        //parse config data
        get_master_device_config_list(&mbMasterConfHead);
    
        if (SLIST_EMPTY(&mbMasterConfHead))
        {
            syslog(LOG_ERR, "No modbus master configuration found in config file");
            modbusDevicesCount = 0;
        }
        else
        {
            modbusDevicesCount = 0;
            //count number of matching devices to allocate memory for pthreads
            SLIST_FOREACH(mbMasterConfigListEntry, &mbMasterConfHead, entries)
            {
                modbusDevicesCount++;
            }
            pThreads = calloc(modbusDevicesCount, sizeof(pthread_t));
        
            //start a thread for every matching configuration found in pictory config file
            i = 0;
            SLIST_FOREACH(mbMasterConfigListEntry, &mbMasterConfHead, entries)
            {
                pThreads[i] = 0;
                if (mbMasterConfigListEntry->mbMasterConfig.tModbusDeviceConfig.eProtocol == eProtRTU)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startRtuMasterThread, (void*)&(mbMasterConfigListEntry->mbMasterConfig)))
                    {
                        syslog(LOG_ERR,
                            "Cannot create modbus master thread for device %s\n",
                            mbMasterConfigListEntry->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);
                    }
                }
                else if (mbMasterConfigListEntry->mbMasterConfig.tModbusDeviceConfig.eProtocol == eProtTCP)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startTcpMasterThread, (void*)&(mbMasterConfigListEntry->mbMasterConfig)))
                    {
                        syslog(LOG_ERR,
                            "Cannot create modbus master thread for IP %s and Port %d\n",
                            mbMasterConfigListEntry->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress,
                            mbMasterConfigListEntry->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
                    }
                }
                else
                {
                    syslog(LOG_ERR, "Unknown modbus protocol");
                }
                i++;
            }
        }
        
        sleep(10);
        
        if (modbusDevicesCount > 0)
        {
            for (i = 0; i < modbusDevicesCount; i++) {
                if (pThreads[i])
                    pthread_cancel(pThreads[i]);
                else
                    syslog(LOG_ERR, "thread %d not started", i);
            }
            free(pThreads);
            while (SLIST_FIRST(&mbMasterConfHead))
            {
                struct TMBMasterConfigEntry *entry = SLIST_FIRST(&mbMasterConfHead);
                SLIST_REMOVE_HEAD(&mbMasterConfHead, entries);
                while (SLIST_FIRST(&entry->mbMasterConfig.mbActionListHead))
                {
                    struct TMBActionEntry *a_entry = SLIST_FIRST(&entry->mbMasterConfig.mbActionListHead);
                    SLIST_REMOVE_HEAD(&entry->mbMasterConfig.mbActionListHead, entries);
                    free(a_entry);
                }
                free(entry);
            }
        }
    }
    
    closelog();	//close syslog
    
    return 0;
}
