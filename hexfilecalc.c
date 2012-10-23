#include <stdio.h>

#define NUM_BANK                            8   

#define REC_TYPE_DATA                       0
#define REC_TYPE_EOF                        1
#define REC_TYPE_EXT_SEGM_ADDR              2
#define REC_TYPE_EXT_LIN_ADDR               4
#define REC_MDKARM_TYPE_START_LINEAR_ADDR   5 // (MDK-ARM only)

#define GET_8_BIT(in1,in2)                          ((in1 - 0x30) << 4 | (in2 - 0x30))

#define GET_16_BIT(chInp1,chInp2,chInp3,chInp4)     ((((chInp1 - 0x30) & 0x0F) << 12) | \
                                                     (((chInp2 - 0x30) & 0x0F) << 8)  | \
                                                     (((chInp3 - 0x30) & 0x0F) << 4)  | \
                                                     (((chInp4 - 0x30) & 0x0F)))

typedef unsigned char   BYTE;


int active_block = 0;

typedef union bankblock {
    BYTE block[64 *1024];
    BYTE banks[2][32*1024];
}bankblock_t;

static bankblock_t s_blocks[4];

static BYTE
asciito8bit(char msb,char lsb) {
    BYTE retval;
    retval = ((msb-0x30) < 0x10) ?(msb-0x30) << 4 : (msb-0x37) << 4;
    retval |= ((lsb-0x30) < 0x10) ?(lsb-0x30) : (lsb-0x37);
    return retval;
}

static int
asciito16bit(char nibble1,char nibble2,char nibble3,char nibble4){
    int retval;
    retval = ((nibble1-0x30) < 0x10) ?(nibble1-0x30) << 12 : (nibble1-0x37) << 12;
    retval |= ((nibble2-0x30) < 0x10) ?(nibble2-0x30) << 8 : (nibble2-0x37) << 8;
    retval |= ((nibble3-0x30) < 0x10) ?(nibble3-0x30) << 4 : (nibble3-0x37) << 4;
    retval |= ((nibble4-0x30) < 0x10) ?(nibble4-0x30) : (nibble4-0x37);
    return retval;
}

static void 
pushbankdata(char pascibuff[],int bnkaddrofs,int datalen){
    int i;
    for(i=0;i<datalen;++i)
    {
        s_blocks[active_block].block[bnkaddrofs+i] = asciito8bit(pascibuff[i*2],pascibuff[i*2 + 1]);
    }
};

/**
 * Input hexfile akan diparsing untuk mengisi alokasi bank
 * @retval  0   Success
 * @retval -1   Unable to open filename
 * @retval -2   Unsupported record type
 */
int 
parse_hexfile(const char* fname){
    char temp[255];
    int len,addr,rtype;
    FILE* ifs = fopen(fname,"r");
    if(!ifs)
        return -1;
    
    while(fgets(temp,255,ifs) != NULL)
    {
        /* if this is not colon, skip the whole line*/
        if(temp[0] != ':')
            continue;
        //int len = (temp[1] - 0x30) << 4| (temp[2] - 0x30);
        len = asciito8bit(temp[1],temp[2]);
        addr = asciito16bit(temp[3],temp[4],temp[5],temp[6]);
        rtype = asciito8bit(temp[7],temp[8]);
        
        printf("[len,addr,rtype] = [%2.2X, %4.4X,%2.2X]\n",len,addr,rtype);
        switch(rtype)
        {
            case REC_TYPE_DATA :
                pushbankdata(&temp[9],addr,len);
            break;
            
            case REC_TYPE_EOF :
                fclose(ifs);
                return 0;
            
            case REC_TYPE_EXT_LIN_ADDR:
                /* if (len != 2) error */
                active_block = GET_16_BIT(temp[9],temp[10],temp[11],temp[12]);
            break;
            
            case REC_TYPE_EXT_SEGM_ADDR: 
                return -2;
                
            case REC_MDKARM_TYPE_START_LINEAR_ADDR:
                return -3;
        }
    }
    
    fclose(ifs);
    return 0;
}


static void
help(void) {
    printf("this is help\n");
}


int 
main(int argc, char* argv[]){
    int retval;
    printf("argv = %s\n",argv[0]);
    if(argc < 2)
    {
        help();
        return -1;
    }
    retval = parse_hexfile(argv[1]);
    printf("retval = %d, currentblock = %d\n",retval,active_block);
    return 0;
}
