#ifndef AUTH_H
#define AUTH_H

#include "structs.h"

int authenticate(const char* username, const char* password); //return 1 if admin exist and creds match, 0 if not

int registerAdmin(const char* username, const char* password);// adds new admin to admins.csv

int allowed(enum Role role, const char* operation); //return 1 if operation allowed for that role, 0 if not

void authInit(void);

#endif
