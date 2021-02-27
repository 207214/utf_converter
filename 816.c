#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//utf8 ---> utf16
//ef bb bf
// ff fe //fe ff
int main(int argc, char ** argv)
{
    unsigned char mask[5] = {0x7F, 0xBF, 0xDF, 0xEF, 0xFF}; // 0111 1111, 1011 1111, 1101 1111, 1110 1111, 1111 1111
    unsigned short res = 0;
    unsigned short res_2 = 0; // if there`s a surrogate pair
    unsigned char b1, b2,b3,b4;
    int flag = 0; //LE or BE
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
    }
    unsigned char tmp[3] = {0,0,0};
    if (fread(tmp, sizeof(char), 2, output))
        if (tmp[0] == 0xFF) flag = 1; // BE
    tmp[0] = tmp[1] =tmp[2] = 0;
    fseek(input, 0, SEEK_SET);


    while ( fread(&b1, sizeof(char), 1, input) )
    {
        res = b2 = b3 = b4 = 0;

        if ((b1 != (b1 & mask[0])) && (b1 != (b1 & mask[2])) && (b1 != (b1 & mask[3])) && (b1 != (b1 & mask[4]))) // ~ if (b1 != b1 & mask[1])
        {
            fprintf(stderr, "Input ffile : %ld\n", ftell(input)-1);
            continue;
        }

        if (b1 == (b1 & mask[0]) )                          //0xxxxxxx
        {
            res |= b1;
            if (!flag)
                res = ((res & 0xFF) << 8) | (res & 0xFF00);
            fwrite(&res, sizeof(unsigned short), 1, output);
            continue;
        }


        fread(&b2, sizeof(char), 1, input);
        if (b2 != (b2 & mask[1]))                               // not 10yyyyyy
        {
            fprintf(stderr, "Input file : %ld\n", ftell(input)-2);
            fseek(input, -1, SEEK_CUR);
            continue;
        }
        if (b1 == (b1 & mask[2]) )                              // 110xxxxx
        {
            res =  (res | (b1 & 0x1F)) << 6;
            res |= (b2 & 0x3F);
            if (!flag)
                res = ((res & 0xFF) << 8) | ((res & 0xFF00)>>8);
            fwrite(&res, sizeof(unsigned short), 1, output);
            continue;
        }


        fread(&b3, sizeof(char), 1, input);
        if (b3 != (b3 & mask[1]) )                              // not 10zzzzzz
        {
            fprintf(stderr, "Input file : %ld\n", ftell(input)-3);
            fseek(input, -1, SEEK_CUR);
            continue;
        }
        if (b1 == (b1 & mask[3]) )                              // 1110xxxx
        {
            res |= (b1 & 0x0F) << 6;
            res =  (res | (b2 & 0x3F)) << 6;
            res |= (b3 & 0x3F);
            if (!flag)
                res = ((res & 0xFF) << 8) | ((res & 0xFF00)>>8);
            fwrite(&res, sizeof(unsigned short), 1, output);
            continue;
        }


        fread(&b4, sizeof(char), 1, input);
        if (b4 != (b4 & mask[1]) )                              // not 10vvvvvv
        {
            fprintf(stderr, "Input file : %ld\n", ftell(input)-4);
            fseek(input, -1, SEEK_CUR);
            continue;
        }
        unsigned int unicode = 0;
        unicode |= (b1 & 0xF0) << 6;
        unicode |= (b2 & 0x3F) << 6;
        unicode |= (b3 & 0x3F) << 6;
        unicode |= (b4 & 0x3F);
        unicode -= 0x10000;
        res_2 = (unicode & 0x3FF) + 0xDC00;
        res = (unicode >> 10) + 0xD800;


        if (!flag)
        {
            res = ((res & 0xFF) << 8) | ((res & 0xFF00)>>8);
            res_2 = ((res_2 & 0xFF) << 8) | ((res_2 & 0xFF00)>>8);
        }


        fwrite(&res, sizeof(unsigned short), 1, output);
        fwrite(&res_2, sizeof(unsigned short), 1, output);
    }

    fclose(input), input = NULL;
    fclose(output), output = NULL;
    return 0;
}

