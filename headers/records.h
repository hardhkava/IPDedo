#ifndef RECORDS_H
#define RECORDS_H

#include "structs.h"

/*finds domain in records.csv*/
int findRecord(const char* domain, struct DNSRecord *res);

/*appending new record to records.csv, mutex used, 0 on success, -1 on file error*/
int saveRecord(const struct DNSRecord* record);

int deleteRecord(const char* domain);

#endif
