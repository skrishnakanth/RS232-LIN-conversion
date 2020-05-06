#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "argparse.h"
#include <windows.h>

int lin_prepare_array(uint8_t *final_array,uint8_t id, uint8_t *data, uint8_t size);
uint8_t final_id(uint8_t identifier);
int calcIdentParity(uint8_t ident);

HANDLE com_init(char *port);
int com_break(HANDLE hSerial,DWORD delay);
int com_write(HANDLE hSerial,uint8_t *data,uint8_t len);
int com_close(HANDLE hSerial);

int k=0,l=0;

static const char *const usages[] = {
    "lin-send [options] [[--] args]",
    "lin-send [options]",
    NULL,
};

int main(int argc, const char **argv) {

	char *com_port_str=NULL,*lin_id_str=NULL,*lin_data_str=NULL;

	uint8_t lin_id;
	uint8_t lin_data[8];
	uint8_t lin_uart_data[32];

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Basic options"),
        OPT_STRING('c', "com-port", &com_port_str, "Specify COM port", NULL, 0, 0),
        OPT_STRING('i', "LIN ID", &lin_id_str, "6-bit LIN ID", NULL, 0, 0),
        OPT_STRING('d', "DATA", &lin_data_str, "Specify data in hex separated by commas (Eg: 0x12,0x96,0x34)", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nA brief description of what the program does and how it works.", "\nAdditional description of the program after the description of the arguments.");
    argc = argparse_parse(&argparse, argc, argv);

    lin_id = strtoul(lin_id_str,NULL,0);
    char *temp = NULL;
    uint8_t count =0;
	temp = strtok(lin_data_str,",");
	while(temp != NULL)
	{
		lin_data[count] = strtoul(temp,NULL,0);
		temp =strtok(NULL,",");
			count++;
	}


	lin_prepare_array(lin_uart_data,lin_id,lin_data,count);

	printf("Data on LIN BUS : \n");

	for(int i=0;i<count+3;i++)
		printf("%02x ",lin_uart_data[i]);

	HANDLE hSerial = com_init(com_port_str);
	com_break(hSerial,1);
	com_write(hSerial,lin_uart_data,count+3);
	com_close(hSerial);


	return 0;
}

int lin_prepare_array(uint8_t *final_array,uint8_t id, uint8_t *data, uint8_t size) {

	final_array[0]= 0x55;
	final_array[1]= final_id(id);
	memcpy(&final_array[2],&data[0],size);
	uint16_t suma = 0;
	for (int i = 0; i < size; i++)
	{
		suma = suma + data[i];
		if(suma > 0xFF)
			suma -= 0xFF;
	}
	uint8_t checksum = ~(uint8_t)suma;
	final_array[size+2] = checksum;

	return 0;
}
uint8_t final_id(uint8_t identifier) {
	uint8_t identbyte = (identifier & 0x3f) | calcIdentParity(identifier);
	return identbyte;
}

#define BIT(data,shift) ((ident&(1<<shift))>>shift)
int calcIdentParity(uint8_t ident) {
	int p0 = BIT(ident,0) ^ BIT(ident, 1) ^ BIT(ident, 2) ^ BIT(ident, 4);
	int p1 = ~(BIT(ident,1) ^ BIT(ident, 3) ^ BIT(ident, 4) ^ BIT(ident, 5));
	return (p0 | (p1 << 1)) << 6;
}

HANDLE com_init(char *port)
{
	HANDLE hSerial;
	char port_path[16];
	sprintf(port_path,"\\\\.\\%s",port);
    hSerial = CreateFile(port_path, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );



    DCB dcbSerialParams = {0};

    if (hSerial == INVALID_HANDLE_VALUE)
    {
            fprintf(stderr, "Error opening serial port\n");
            exit(-1);
    }


    // Set device parameters (38400 baud, 1 start bit,
    // 1 stop bit, no parity)
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stderr, "Error getting device state\n");
        CloseHandle(hSerial);
        exit(-2);
    }

    dcbSerialParams.BaudRate = CBR_19200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(SetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stderr, "Error setting device parameters\n");
        CloseHandle(hSerial);
        exit(-3);
    }

    return hSerial;
}

int com_break(HANDLE hSerial, DWORD delay)
{
     SetCommBreak(hSerial);
	 Sleep(delay);
	 ClearCommBreak(hSerial);
	 return 0;

}

int com_write(HANDLE hSerial,uint8_t *data,uint8_t len)
{
    if(!WriteFile(hSerial, data, len,NULL, NULL))
    {
        fprintf(stderr, "Error sending data\n");
        CloseHandle(hSerial);
        exit(-4);
    }
    return 0;
}

int com_close(HANDLE hSerial)
{
	CloseHandle(hSerial);
	return 0;
}
