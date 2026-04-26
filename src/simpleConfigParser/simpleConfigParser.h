#pragma once

#include "../modbusconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parses a simplified .ini configuration file to populate the master configuration list
void get_master_device_config_list(struct TMBMasterConfHead *p_mbMasterConfHead_p);

// Parses a simplified .ini configuration file to populate the slave configuration list
void get_slave_device_config_list(struct TMBSlaveConfHead *p_mbSlaveConfHead_p);

// The parse functions simply read predefined parameters or parse an INI file
// For this standalone setup, we will look for a default file "modbus_config.ini" in the current directory.

#ifdef __cplusplus
}
#endif
