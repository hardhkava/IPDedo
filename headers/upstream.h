#ifndef UPSTREAM_H
#define UPSTREAM_H

/*forwards dns query to google ka 8.8.8.8 via udp, returns 1 on success, 0 on fail*/
int fetchFromUpstream(const char* domain, char* ipOut);

#endif

