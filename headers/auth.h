#ifndef AUTH_H
#define AUTH_H

#include "structs.h"

enum Role authenticate(const char* username, const char* password); //enum function which will return role based on username

int allowed(enum Role role, const char* operation) //return 1 if operation allowed for that role, 0 if not

//#endif
