/*
 * 
 *
 */

/*
 * Main structure - armlet code gets on input only one structure pointer
 *  executes one function identified by functionID - functionData contains 
 *  data needed to run function.
 * functionID is used olso as a returned value - if function runs functionID
 *  is set to functionID + 100. It is used to find if function realy runs or not.
 */

typedef struct _armMainInput
{
    unsigned long functionID;
    void *functionData;
}armMainInput;

typedef struct _armFunction10Input
{
    char *data;
    unsigned long allocated;
    unsigned long used;
}armFunction10Input;