
8051 Hex File CRC Calculator


Usage: hexfilecalc  -i inputfile
                    -b no-bank
                    -l length
                    -a start-addr
                    -n initial-crc-val

Example:
                    
hexfilecalc -i ../example/ROM.hex -b 1 -l 10 -a 0x00A0 -n 0xDEAD


Warning:
    Input argument for start address (- a start-addr) and initial CRC value (-n initial-crc-val)
    shall use hexadecimal format as in example
    
    
Error Code list:
0   : There's no error (operation complete)
-1  : File input cannot be found
-2  : Format payload for extended linear address record has wrong in length field
-3  : Not supported 