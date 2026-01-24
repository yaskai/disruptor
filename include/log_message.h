#ifndef LOG_MESSAGE_H_
#define LOG_MESSAGE_H_

#include "ansi_codes.h"

void Message(char *text, char *color);
void MessageError(char *text, char *param);
void MessageKeyValPair(char *key, char *val);

#endif
