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
    if (argc >= 2 && strcmp(argv[1], "gen") == 0) {
        FILE *fp = fopen("modbus_config.ini.txt", "w");
        if (fp) {
            fprintf(fp, "# ===================================================\n");
            fprintf(fp, "# 1. 첫 번째 포트 : 지정된 시리얼 장비 (RTU)\n");
            fprintf(fp, "# ===================================================\n");
            fprintf(fp, "[RTUMaster]\n");
            fprintf(fp, "Device=/dev/ttyUSB0\n");
            fprintf(fp, "Baudrate=19200\n");
            fprintf(fp, "Parity=N\n");
            fprintf(fp, "Databits=8\n");
            fprintf(fp, "Stopbits=1\n\n");
            fprintf(fp, "# /dev/ttyUSB0 에 물려있는 1번 슬레이브 장비 데이터\n");
            fprintf(fp, "#address,fc,reg,count,interval,value\n");
            fprintf(fp, "#fc: 1(Read Coils), 2(Read Discrete Inputs), 3(Read Holding Registers), 4(Read Input Registers), 5(Write Single Coil), 6(Write Single Register), 15(Write Multiple Coils), 16(Write Multiple Registers)\n");
            fprintf(fp, "Action=1,3,1,5,1000,0\n\n");
            fprintf(fp, "# /dev/ttyUSB0 에 물려있는 2번 슬레이브 장비 데이터\n");
            fprintf(fp, "Action=2,3,10,2,1000,10\n\n\n");
            fprintf(fp, "# ===================================================\n");
            fprintf(fp, "# 2. 두 번째 포트 : 또 다른 시리얼 포트 (RTU)\n");
            fprintf(fp, "# ===================================================\n");
            fprintf(fp, "[RTUMaster]\n");
            fprintf(fp, "Device=/dev/ttyAMA0\n");
            fprintf(fp, "Baudrate=9600\n");
            fprintf(fp, "Parity=E\n");
            fprintf(fp, "Databits=8\n");
            fprintf(fp, "Stopbits=1\n\n");
            fprintf(fp, "#address,fc,reg,count,interval,value\n");
            fprintf(fp, "#fc: 1(Read Coils), 2(Read Discrete Inputs), 3(Read Holding Registers), 4(Read Input Registers), 5(Write Single Coil), 6(Write Single Register), 15(Write Multiple Coils), 16(Write Multiple Registers)\n");
            fprintf(fp, "Action=5,3,100,10,1000,20\n\n\n");
            fprintf(fp, "# ===================================================\n");
            fprintf(fp, "# 3. 세 번째 포트 : 기존 테스트하시던 TCP 통신\n");
            fprintf(fp, "# ===================================================\n");
            fprintf(fp, "[TCPMaster]\n");
            fprintf(fp, "IP=192.168.219.251\n");
            fprintf(fp, "Port=502\n\n");
            fprintf(fp, "#address,fc,reg,count,interval,value\n");
            fprintf(fp, "#fc: 1(Read Coils), 2(Read Discrete Inputs), 3(Read Holding Registers), 4(Read Input Registers), 5(Write Single Coil), 6(Write Single Register), 15(Write Multiple Coils), 16(Write Multiple Registers)\n");
            fprintf(fp, "Action=1,3,1,5,1000,40\n");
            fprintf(fp, "Action=1,16,10,2,1000,50\n");
            fclose(fp);
            printf("Sample config file generated: modbus_config.ini.txt\n");
        } else {
            printf("Failed to generate modbus_config.ini.txt\n");
        }
        return 0;
    }

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
        
        char line[256];
        printf("Modbus Master started. Type 'help' for commands.\n");
        while (1) {
            printf("cli> ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) {
                break;
            }
            
            // parse command
            char cmd[32] = {0};
            int id = -1;
            char type[32] = {0};
            int index = -1;
            int value = -1;
            
            sscanf(line, "%s", cmd);
            
            if (strcmp(cmd, "help") == 0) {
                printf("Commands:\n");
                printf("  list\n");
                printf("  show <id> <all|in_word|out_word|in_bit|out_bit>\n");
                printf("  set <id> <out_word|out_bit> <index(1-32)> <value>\n");
                printf("  debug <level>\n");
                printf("  quit\n");
            } else if (strcmp(cmd, "list") == 0) {
                int list_id = 0;
                struct TMBMasterConfigEntry *entry;
                SLIST_FOREACH(entry, &mbMasterConfHead, entries) {
                    if (entry->mbMasterConfig.tModbusDeviceConfig.eProtocol == eProtTCP) {
                        printf("[%d] TCP %s:%d\n", list_id, 
                            entry->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress,
                            entry->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
                    } else {
                        printf("[%d] RTU %s\n", list_id, 
                            entry->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);
                    }
                    list_id++;
                }
            } else if (strcmp(cmd, "show") == 0) {
                if (sscanf(line, "%*s %d %s", &id, type) == 2) {
                    int list_id = 0;
                    struct TMBMasterConfigEntry *entry;
                    SLIST_FOREACH(entry, &mbMasterConfHead, entries) {
                        if (list_id == id) {
                            pthread_mutex_lock(&entry->mbMasterConfig.dataBuffer.lock);
                            if (strcmp(type, "all") == 0) {
                                printf("in_word: ");
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.input_words[k]);
                                printf("\nin_bit: ");
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.input_bits[k]);
                                printf("\nout_word: ");
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.output_words[k]);
                                printf("\nout_bit: ");
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.output_bits[k]);
                            } else if (strcmp(type, "in_word") == 0) {
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.input_words[k]);
                            } else if (strcmp(type, "out_word") == 0) {
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.output_words[k]);
                            } else if (strcmp(type, "in_bit") == 0) {
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.input_bits[k]);
                            } else if (strcmp(type, "out_bit") == 0) {
                                for(int k=0; k<32; k++) printf("%d ", entry->mbMasterConfig.dataBuffer.output_bits[k]);
                            } else {
                                printf("Invalid type. Use all, in_word, out_word, in_bit, out_bit.\n");
                            }
                            pthread_mutex_unlock(&entry->mbMasterConfig.dataBuffer.lock);
                            printf("\n");
                            break;
                        }
                        list_id++;
                    }
                } else {
                    printf("Usage: show <id> <type>\n");
                }
            } else if (strcmp(cmd, "set") == 0) {
                if (sscanf(line, "%*s %d %s %d %d", &id, type, &index, &value) == 4) {
                    if (index >= 1 && index <= 32) {
                        int arr_idx = index - 1;
                        int list_id = 0;
                        struct TMBMasterConfigEntry *entry;
                        SLIST_FOREACH(entry, &mbMasterConfHead, entries) {
                            if (list_id == id) {
                                pthread_mutex_lock(&entry->mbMasterConfig.dataBuffer.lock);
                                if (strcmp(type, "out_word") == 0) {
                                    entry->mbMasterConfig.dataBuffer.output_words[arr_idx] = value;
                                    printf("Set out_word[%d] = %d\n", index, value);
                                } else if (strcmp(type, "out_bit") == 0) {
                                    entry->mbMasterConfig.dataBuffer.output_bits[arr_idx] = value;
                                    printf("Set out_bit[%d] = %d\n", index, value);
                                } else {
                                    printf("Invalid type for set. Use out_word, out_bit.\n");
                                }
                                pthread_mutex_unlock(&entry->mbMasterConfig.dataBuffer.lock);
                                break;
                            }
                            list_id++;
                        }
                    } else {
                        printf("Invalid index. Must be 1-32.\n");
                    }
                } else {
                    printf("Usage: set <id> <type> <index> <value>\n");
                }
            } else if (strcmp(cmd, "debug") == 0) {
                if (sscanf(line, "%*s %d", &value) == 1) {
                    g_iDebugLevel = value;
                    printf("Debug level set to %d\n", g_iDebugLevel);
                } else {
                    printf("Usage: debug <level>\n");
                }
            } else if (strcmp(cmd, "quit") == 0) {
                break;
            } else if (strlen(cmd) > 0) {
                printf("Unknown command: %s\n", cmd);
            }
        }
        
        break; // break the outer while(1) loop to exit completely on quit
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
