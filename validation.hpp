#ifndef VALIDATION_H
#define VALIDATION_H

#include <cstring>
#include <cctype>

#define LOCALHOST "127.0.0.1"

bool validateIPaddr(const char* host) {
    if(strlen(host) == 1 && host[0] == '0')
        return true;

    int dot_pos = -1;
    int dot_count = 0;

    for(int i = 0; i < strlen(host); i++) {
        if(host[i] == '.') {
            if(dot_pos == i - 1 || dot_count == 3)
                return false;
            else {
                dot_count++;
                dot_pos = i;
            }
        }
        else if(!isdigit(host[i]))
            return false;
    }

    if(dot_count != 3)
        return false;
    
    return true;
}

bool validatePort(const char* port) {
    for(int i = 0; i < strlen(port); i++) {
        if(!isdigit(port[i]))
            return false;
    }

    return true;
}

void getIPandPort(char* host, int &port) {
	char p[20];

	// IP
	printf("Enter server IP (for localhost, enter 0):\n> ");
	scanf("%s", host);

	while(!validateIPaddr(host)) {
		printf("Invalid IP address. Try again:\n> ");
		scanf("%s", host);
	}
	if(strlen(host) == 1 && host[0] == '0')
		strcpy(host, LOCALHOST);

	// PORT
	printf("Enter port number:\n> ");
	scanf("%s", p);

	while(!validatePort(p)) {
		printf("Invalid port number. Try again:\n> ");
		scanf("%s", p);
	}
    
    port = atoi(p);
}

#endif // VALIDATION_H