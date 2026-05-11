#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "system_state.h"

void handleSerialCommands();
void processCommand(const char* command);
void showHelp();
void showCurrentState();
String getTimeString();
void printStateName(SystemState state);
char toLowerCase(char c);

#endif // COMMAND_HANDLER_H
