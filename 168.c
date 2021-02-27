#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FPUTCCHECK(b,f)  if ((fputc(b, f)==EOF)) _exit(-1)

//utf16 ---> utf8

// 0xxxxxxx //110xxxxx 10yyyyyy //1110xxxx 10yyyyyy 10zzzzzz //1111xxxx 10ffffff 10eeeeee 10xxxxxx
int one_octet_needed(unsigned short word, FILE * out)
{
    char byte = word;
    FPUTCCHECK(byte, out);
    return 0;
}

int two_octets_needed(unsigned short word, FILE * out)
{
    char byte = 0xC0 | ( (word & 0x7C0) >> 6);         //0x00C0 == 11000000
    FPUTCCHECK(byte, out);
    byte = 0x80 | (word & 0x3F);                               //0x0080 == 10000000
    FPUTCCHECK(byte, out);
    return 0;
}

int three_octets_needed(unsigned short word, FILE * out)
{
    char byte = 0xE0 | ( (word & 0xF000) >> 12 );       //0x00E0 == 11100000
    FPUTCCHECK(byte, out);
    byte = 0x80 | ( (word & 0xFC0) >> 6);
    FPUTCCHECK(byte, out);
    byte = 0x80 |  (word & 0x3F);
    FPUTCCHECK(byte, out);
    return 0;
}

int four_octets_needed(unsigned short word1, unsigned short word2, FILE * out) //word1: D800..DBFF, word2: DC00..DFFF
{
    unsigned char byte = 0;
    unsigned int unicode = 0;
    unicode |= ( (word1 - 0xD800) << 10);
    unicode |= (word2 - 0xDC00);
    unicode += 0x10000;
    byte = 0xF0 | (unicode >> 18);
    FPUTCCHECK(byte, out);                                                    // putting 11110xxx
    for (int i = 12; i >=0; i = i-6)                                          // putting 10yyyyyy, 10zzzzzz, 10vvvvvv
        FPUTCCHECK(byte = (0x80 | ( (unicode >> i) & 0x3F)), out);
    return 0;
}

int main(int argc, char ** argv)
{
    int b1, b2;
    int cnt = 0;
    long eof = 0;
    int pair_flag = 0;            // surrogate pair or not
    int flag = 0;                 // LE or BE
    FILE * input = stdin;
    FILE * output = stdout;
    if (argc > 1)
    {
        if ( (input = fopen(argv[1], "r")) == NULL )
        {
            printf("Error! Can`t open the 'input' file\n");
            return 1;
        }
        if (argc > 2)
            if ( (output = fopen(argv[2], "w")) == NULL )
            {
                printf("Error! Can`t open the output file\n");
                return 2;
            }
        fseek(input, 0, SEEK_END);
        eof = ftell(input);
        if ( (eof = ftell(input)) % 2)                                 // odd number of bytes
        {
            fprintf(stderr, "Wrong number of bytes!\n");
            return 3;
        }
        fseek(input, 0, SEEK_SET);
    }
    unsigned short buf = 0;
    unsigned short tmp = 0;
    fread(&buf, sizeof(unsigned short), 1, input);
    if ( (buf != 0xFEFF) && (buf != 0xFFFE) )
        buf = 0xFEFF;
    else if (buf == 0xFFFE)
        flag = 1;
    fseek(input, 0, SEEK_SET); //

    while ( ftell(input) != eof)
    {
        b1 = fgetc(input);
        b2 = fgetc(input);
        if (b1 == EOF) break;
        else
        {
            cnt++;
            if (b2 == EOF) break;
            cnt++;
        }
        if (flag) //BE
            buf = ( ((b1 & 0xFF) << 8) | (b2 & 0xFF) );
        else
            buf = ( (b1 & 0xFF) | ((b2 & 0xFF) << 8) );

        if ((buf >= 0xD800) && (buf <= 0xDBFF))
         {
            if (pair_flag)
            {
                pair_flag = 0;
                fprintf(stderr, "Input file : %ld\n", ftell(input)-2);
                fseek(input, -1, SEEK_CUR);
            }
            else
            {
                pair_flag = 1;
                tmp = buf;
            }
            continue;
        }

        if ((buf >= 0xDC00) && (buf <= 0xDFFF))
        {
            if (!pair_flag)
                fprintf(stderr, "Input file : %ld\n", ftell(input)-1);
            else
            {
                pair_flag = 0;
                b1 = four_octets_needed(buf, tmp, output);
            }
            continue;
        }

        if (buf <= 127)
            b1 = one_octet_needed(buf, output);
        else if (buf <= 2047)
            b1 = two_octets_needed(buf, output);
        else if (buf <= 65535)
            b1 = three_octets_needed(buf, output);
    }
    if (cnt % 2)
        fprintf(stderr, "\nWrong number of bytes!\n");
    fclose(input), input = NULL;
    fclose(output), output = NULL;
    return 0;
}

