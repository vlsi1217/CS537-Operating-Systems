#ifndef __REQUEST_H__

void requestHandle(int fd);
int requestParseURI(char *uri, char *filename, char *cgiargs);
void requestServeStatic(int fd, char *filename, int filesize);
void requestServeDynamic(int fd, char *filename, char *cgiargs);
void requestError(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void requestReadhdrs(rio_t *rp);

#endif
