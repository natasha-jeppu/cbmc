typedef unsigned char BYTE;
 
int main()
{
    unsigned int value;
    BYTE* bp = (BYTE*)(&value); 

    bp[0] = bp[2];
    return 0;
}
