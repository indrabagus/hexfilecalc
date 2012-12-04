/*
 *
 * Purpose: 
 * parsing and calculating CRC value on 8051 Hex file
 *
 * Writer : Indra Bagus W <indra@xirkachipset.com>
 *
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#define NUM_BANK                            8   

#define REC_TYPE_DATA                       0
#define REC_TYPE_EOF                        1
#define REC_TYPE_EXT_SEGM_ADDR              2
#define REC_TYPE_EXT_LIN_ADDR               4
#define REC_MDKARM_TYPE_START_LINEAR_ADDR   5 // (MDK-ARM only)


#define PARSE_ERR_NOERROR        0
#define PARSE_ERR_FILE          -1
#define PARSE_ERR_EXT_LIN_ERR   -2
#define PARSE_ERR_MDKARM        -3
#define PARSE_ERR_EOF_NOTFOUND  -4


typedef unsigned char   BYTE;

typedef union bankblock {
    BYTE block[64 *1024];
    BYTE banks[2][32*1024];
}bankblock_t;

typedef struct inp_param {
    char filein[512];
    int no_bank;
    int start_addr;
    int length;
    unsigned int crc_init;
}inp_param_t;

static bankblock_t s_blocks[4];
int active_block = 0;
static const unsigned int crchighbit    = 0x8000;
static const unsigned int crcinit       = 0xFFFF;
static const unsigned int crcpolynom    = 0x1021;
static const unsigned int crcmask       = 0xFFFF;

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
    int retval;
    int len,addr,rtype;
    FILE* ifs = fopen(fname,"r");
    if(!ifs){
        return PARSE_ERR_FILE;
    }    
    while(fgets(temp,255,ifs) != NULL)
    {
        /* if this is not colon, skip the whole line*/
        if(temp[0] != ':')
            continue;
        len     = asciito8bit(temp[1],temp[2]);
        addr    = asciito16bit(temp[3],temp[4],temp[5],temp[6]);
        rtype   = asciito8bit(temp[7],temp[8]);
        
        switch(rtype)
        {
            case REC_TYPE_DATA :
                pushbankdata(&temp[9],addr,len);
            break;
            
            case REC_TYPE_EOF :
                break;
            
            case REC_TYPE_EXT_LIN_ADDR:
                /* if (len != 2) error */
                if(len != 2){
                    retval = PARSE_ERR_EXT_LIN_ERR;
                    goto closure;
                }
                active_block = asciito16bit(temp[9],temp[10],temp[11],temp[12]);
            break;
            
            case REC_TYPE_EXT_SEGM_ADDR:
                break;
                
            case REC_MDKARM_TYPE_START_LINEAR_ADDR:
                retval = PARSE_ERR_MDKARM;
                goto closure;
        }
    }
    
    if(rtype == REC_TYPE_EOF)
        retval = PARSE_ERR_NOERROR;
    else
        retval = PARSE_ERR_EOF_NOTFOUND;

closure:
    fclose(ifs);
    return retval;

}


static unsigned int
calculate_crc(inp_param_t *pinpparam){
    BYTE* pbuffer;
	unsigned int i, j, c, bit;
    unsigned int crc = pinpparam->crc_init;
    pbuffer = s_blocks[pinpparam->no_bank/2].banks[pinpparam->no_bank % 2];

    for (i=0; i<pinpparam->length; i++) {

		c = (unsigned long)*(pbuffer+i);

		for (j=0x80; j; j>>=1) {

			bit = crc & crchighbit;
			crc<<= 1;
			if (c & j) bit^= crchighbit;
            if (bit) crc^= crcpolynom;
		}
	}	

	crc&= crcmask;

	return(crc);
}

static int
help(void) {
    printf("\
Usage: hexfilecalc  -i inputfile \n\
                    -b no-bank \n\
                    -l length \n\
                    -a start-addr \n\
                    -n initial-crc-val\n"
                    );
    return -1;
}


int 
main(int argc, char* argv[]){
    int retval;
    unsigned int crcout;
    int i;
    char in;
    inp_param_t input;
    if(argc < 11)
    {
        return help();
    }
    /* parse input argument */
    for(i=0;i<5;++i){
        if(argv[(i*2)+1][0] != '-')
            return help();
        if(strlen(argv[(i*2)+1]) != 2)
            return help();
        in = tolower(argv[(i*2) + 1][1]);
        switch(in)
        {
            case 'i':
                strcpy(input.filein,argv[(i*2) + 2]);
            break;

            case 'b':
                input.no_bank = atoi(argv[(i*2) + 2]);
            break;

            case 'l':
                input.length = atoi(argv[(i*2) + 2]);
            break;

            case 'a':
                input.start_addr = (int)strtol(argv[(i*2) + 2],NULL,16);
            break;
            
            case 'n':
                input.crc_init = (int)strtol(argv[(i*2) + 2],NULL,16);
            break;
            default:
                return help();
        }
    }


    retval = parse_hexfile(input.filein);
    if(retval)
        printf("PARSE ERROR CODE: %d\n",retval);
    crcout = calculate_crc(&input);
    printf("CRC OUT = 0x%X\n",crcout);

    return 0;
}
