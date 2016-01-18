#ifndef MILEAGE_H
#define MILEAGE_H

#include <eat_type.h>

#define MAX_MILEAGE_LEN 60

typedef enum
{
    SETTINGFILE,
    MILEAGEFILE

}FILE_NAME;


typedef struct
{
    float voltage[MAX_MILEAGE_LEN];
    float dump_mileage[MAX_MILEAGE_LEN];

}DumpVoltage;

typedef enum
{
    MILEAGE_START,
    MILEAGE_STOP,
    MILEAGE_RESTORE

}MILEAGE_HANDLE;




void mileage_initial(void);
void mileagehandle(short MILEAGE_STATE);
void adc_mileageinit_proc(EatAdc_st* adc);
void adc_mileagestart_proc(EatAdc_st* adc);
void adc_mileageend_proc(EatAdc_st* adc);



eat_bool mileage_restore(void);



#define MILEAGEFILE_NAME   L"C:\\mileage.txt"


#endif//MILEAGE_H

