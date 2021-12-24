

#include <stdio.h>

int main(int argc, char** argv) {
    FILE *logPtr;
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Error: could not open file %s", argv[1]);
        return 1;
    }
    char line[256];
	while(fgets(line, 256, fp))	{
		printf("%s\n", line);        
    }
    printf("done reading\n");
    fclose(fp);
    return 0;
}