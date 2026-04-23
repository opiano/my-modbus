#include "simpleConfigParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define CONFIG_FILE "simple_modbus_config.ini"
#define MAX_LINE 256

// Simple INI parsing helper to extract value for a given key string
// e.g. IP=192.168.0.10
static void get_kv(char *line, char **key, char **val) {
    *key = line;
    *val = NULL;
    char *delim = strchr(line, '=');
    if (delim) {
        *delim = '\0';
        *val = delim + 1;
        
        // Remove trailing newlines
        char *ptr = *val;
        while(*ptr) {
            if (*ptr == '\r' || *ptr == '\n') {
                *ptr = '\0';
                break;
            }
            ptr++;
        }
    }
}

// Dummy/Simple approach: Reads simple_modbus_config.ini if it exists.
// For now, it populates a single dummy master configuration if file reading fails
// or if the user creates a basic one.
void get_master_device_config_list(struct TMBMasterConfHead *p_mbMasterConfHead_p)
{
    FILE *fp = fopen(CONFIG_FILE, "r");
    
    struct TMBMasterConfigEntry *currentMaster = NULL;
    struct TMBActionEntry *currentAction = NULL;

    if (fp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), fp)) {
            // Remove trailing newlines from line
            char *ptr = line;
            while(*ptr) { if (*ptr == '\r' || *ptr == '\n') { *ptr = '\0'; break; } ptr++; }
            if (strlen(line) == 0) continue;

            // Start of a new TCP or RTU Master device
            if (strcmp(line, "[TCPMaster]") == 0 || strcmp(line, "[RTUMaster]") == 0) {
                if (currentAction && currentMaster) {
                    SLIST_INSERT_HEAD(&currentMaster->mbMasterConfig.mbActionListHead, currentAction, entries);
                    currentAction = NULL;
                }
                if (currentMaster) {
                    SLIST_INSERT_HEAD(p_mbMasterConfHead_p, currentMaster, entries);
                }
                
                currentMaster = calloc(1, sizeof(struct TMBMasterConfigEntry));
                currentMaster->mbMasterConfig.i32ActionCount = 0;
                SLIST_INIT(&currentMaster->mbMasterConfig.mbActionListHead);

                if (strcmp(line, "[TCPMaster]") == 0) {
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.eProtocol = eProtTCP; 
                    strncpy(currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress, "127.0.0.1", INET_ADDRSTRLEN-1);
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort = 502;
                } else {
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.eProtocol = eProtRTU; 
                    strncpy(currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath, "/dev/ttyUSB0", PATH_MAX-1);
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.i32uBaud = 19200;
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.cParity = 'N';
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.i8uDatabits = 8;
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.i8uStopbits = 1;
                }
                continue;
            }

            // Start of a new Action block parsing
            if (strcmp(line, "[Action]") == 0) {
                if (!currentMaster) {
                    // Create default TCP master if user forgot [TCPMaster] at the top
                    currentMaster = calloc(1, sizeof(struct TMBMasterConfigEntry));
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.eProtocol = eProtTCP; 
                    strncpy(currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress, "127.0.0.1", INET_ADDRSTRLEN-1);
                    currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort = 502;
                    currentMaster->mbMasterConfig.i32ActionCount = 0;
                    SLIST_INIT(&currentMaster->mbMasterConfig.mbActionListHead);
                }

                if (currentAction) {
                    SLIST_INSERT_HEAD(&currentMaster->mbMasterConfig.mbActionListHead, currentAction, entries);
                }
                
                currentAction = calloc(1, sizeof(struct TMBActionEntry));
                currentAction->modbusAction.eFunctionCode = eREAD_HOLDING_REGISTERS;
                currentAction->modbusAction.i8uSlaveAddress = 1;         // default
                currentAction->modbusAction.i32uStartRegister = 1;       // default
                currentAction->modbusAction.i16uRegisterCount = 10;      // default
                currentAction->modbusAction.i32uInterval_us = 1000000;   // 1 sec default
                currentMaster->mbMasterConfig.i32ActionCount++;
                continue;
            }

            char *key, *val;
            get_kv(line, &key, &val);
            if (key && val) {
                // If we are configuring the device (before any Action block)
                if (currentMaster && !currentAction) {
                    if (currentMaster->mbMasterConfig.tModbusDeviceConfig.eProtocol == eProtTCP) {
                        if (strcmp(key, "IP") == 0) {
                            strncpy(currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress, val, INET_ADDRSTRLEN-1);
                        } else if (strcmp(key, "Port") == 0) {
                            currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort = atoi(val);
                        }
                    } else if (currentMaster->mbMasterConfig.tModbusDeviceConfig.eProtocol == eProtRTU) {
                        if (strcmp(key, "Device") == 0) {
                            strncpy(currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath, val, PATH_MAX-1);
                        } else if (strcmp(key, "Baudrate") == 0) {
                            currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.i32uBaud = atoi(val);
                        } else if (strcmp(key, "Parity") == 0) {
                            currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.cParity = val[0];
                        } else if (strcmp(key, "Databits") == 0) {
                            currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.i8uDatabits = atoi(val);
                        } else if (strcmp(key, "Stopbits") == 0) {
                            currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tRtuConfig.i8uStopbits = atoi(val);
                        }
                    }
                } 
                // If we are configuring an Action block
                else if (currentAction) {
                    if (strcmp(key, "ActionSlaveAddress") == 0) {
                        currentAction->modbusAction.i8uSlaveAddress = atoi(val);
                    } else if (strcmp(key, "ActionStartRegister") == 0) {
                        currentAction->modbusAction.i32uStartRegister = atoi(val);
                    } else if (strcmp(key, "ActionRegisterCount") == 0) {
                        currentAction->modbusAction.i16uRegisterCount = atoi(val);
                    } else if (strcmp(key, "ActionInterval_ms") == 0) {
                        currentAction->modbusAction.i32uInterval_us = atoi(val) * 1000;
                    } else if (strcmp(key, "ActionFunctionCode") == 0) {
                        currentAction->modbusAction.eFunctionCode = atoi(val);
                    } else if (strcmp(key, "ActionDeviceValue") == 0) {
                        currentAction->modbusAction.i32uStartByteProcessData = atoi(val);
                    }
                }
            }
        }
        fclose(fp);
    } 
    
    // Fallback if no file exists or if no master was created
    if (!currentMaster) {
        syslog(LOG_NOTICE, "simple_modbus_config.ini not found or empty. Using default dummy config.\n");
        currentMaster = calloc(1, sizeof(struct TMBMasterConfigEntry));
        currentMaster->mbMasterConfig.tModbusDeviceConfig.eProtocol = eProtTCP; 
        strncpy(currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress, "127.0.0.1", INET_ADDRSTRLEN);
        currentMaster->mbMasterConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort = 502;
        currentMaster->mbMasterConfig.i32ActionCount = 1;
        SLIST_INIT(&currentMaster->mbMasterConfig.mbActionListHead);
        
        currentAction = calloc(1, sizeof(struct TMBActionEntry));
        currentAction->modbusAction.eFunctionCode = eREAD_HOLDING_REGISTERS;
        currentAction->modbusAction.i8uSlaveAddress = 1;
        currentAction->modbusAction.i32uStartRegister = 1;
        currentAction->modbusAction.i16uRegisterCount = 10;
        currentAction->modbusAction.i32uInterval_us = 1000000;
    }

    // Insert the final remaining element into the list
    if (currentAction && currentMaster) {
        SLIST_INSERT_HEAD(&currentMaster->mbMasterConfig.mbActionListHead, currentAction, entries);
    }
    if (currentMaster) {
        SLIST_INSERT_HEAD(p_mbMasterConfHead_p, currentMaster, entries);
    }
}

void get_slave_device_config_list(struct TMBSlaveConfHead *p_mbSlaveConfHead_p)
{
    FILE *fp = fopen(CONFIG_FILE, "r");
    
    // Allocate one slave entry
    struct TMBSlaveConfigEntry *newSlave = calloc(1, sizeof(struct TMBSlaveConfigEntry));
    newSlave->mbSlaveConfig.tModbusDeviceConfig.eProtocol = eProtTCP;
    strncpy(newSlave->mbSlaveConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress, "0.0.0.0", INET_ADDRSTRLEN);
    newSlave->mbSlaveConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort = 502;
    
    // Allocate dummy data boundaries
    newSlave->mbSlaveConfig.tModbusDataConfig.u16DiscreteInputs = 100;
    newSlave->mbSlaveConfig.tModbusDataConfig.u16Coils = 100;
    newSlave->mbSlaveConfig.tModbusDataConfig.u16InputRegisters = 100;
    newSlave->mbSlaveConfig.tModbusDataConfig.u16HoldingRegisters = 100;
    
    if (fp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), fp)) {
            char *key, *val;
            get_kv(line, &key, &val);
            if (key && val) {
                if (strcmp(key, "SlavePort") == 0) {
                    newSlave->mbSlaveConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort = atoi(val);
                }
            }
        }
        fclose(fp);
    }

    SLIST_INSERT_HEAD(p_mbSlaveConfHead_p, newSlave, entries);
}
