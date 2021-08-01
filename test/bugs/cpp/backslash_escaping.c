int puts(const char *);

#define PRINT_(X) puts(#X);
#define PRINT(X) PRINT_(X)

int main(void)
{
    PRINT(\\)
    return 0;
}
