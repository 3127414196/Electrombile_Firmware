/*
 * data.c
 *
 *  Created on: 2015��7��9��
 *      Author: jk
 */


#include "data.h"


LOCAL_DATA data =
{
        EAT_FALSE,
        EAT_FALSE,
        EAT_FALSE,
        {0}
};


eat_bool socket_conneted(void)
{
    return data.connected;
}

void set_socket_state(eat_bool connected)
{
    data.connected = connected;
}

eat_bool client_logined(void)
{
    return data.logined;
}

void set_client_state(eat_bool logined)
{
    data.logined = logined;
}

eat_bool seek_fixed(void)
{
    return data.isSeekFixed;
}

void set_seek_state(eat_bool fixed)
{
    data.isSeekFixed = fixed;
}


