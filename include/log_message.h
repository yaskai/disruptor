#ifndef LOG_MESSAGE_H_
#define LOG_MESSAGE_H_

#include "ansi_codes.h"

void SetLogState(short state);
short GetLogState();

void Message(char *text, char *color);
void MessageError(char *text, char *param);
void MessageDiag(char *text, char *param, char *color);
void MessageKeyValPair(char *key, char *val);

void MessageDiagInt(char *text, int param, char *color);

void MessageKeyValPairInt(char *key, int val);
void MessageKeyValPairFloat(char *key, float val);
void MessageKeyValPairVec3(char *key, float x, float y, float z);

#endif
