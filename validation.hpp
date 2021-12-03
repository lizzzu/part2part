#ifndef VALIDATION_H
#define VALIDATION_H

#include <cstring>
#include <cctype>

bool validateIPaddr(char* host) {
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

bool validatePort(char* port) {
    for(int i = 0; i < strlen(port); i++) {
        if(!isdigit(port[i]))
            return false;
    }

    return true;
}

#endif // VALIDATION_H