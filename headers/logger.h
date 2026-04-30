#ifndef LOGGER_H
#define LOGGER_H

/*runs as separate child proc, reads from pipe and writes to server.log*/
void loggerProcess(const char* pipePath);

/*sends msg to logger proc via pipe*/
void logEvent(const char* pipePath, const char* domain, const char* clientIP, const char* result);

#endif

