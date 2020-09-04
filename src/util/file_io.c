#include "input.h"

int lines_in_file( const char *file_name) {
    FILE *fileptr;
    int count_lines = 0;
    fileptr = fopen(file_name, "r");

   //extract character from file and store in chr
   int c;
    c = getc(fileptr);
    while (c != EOF)
    {
        //Count whenever new line is encountered
        if (c == '\n')
        {
            count_lines++;
        }

        //take next character from file.
        c = getc(fileptr);
    }

    fclose(fileptr); //close file.
    return count_lines;
}

#define LINE_WIDTH 512

void get_line( int line_num, TerminalInfo* terminal ) {
    const char *f_name = "terminal_history.txt";

    int num_lines = lines_in_file(f_name);
    char line[num_lines][LINE_WIDTH];
    int i = 0;
    int tot = 0;

    FILE *t_hist = fopen(f_name, "r");
    while(fgets(line[i], LINE_WIDTH, t_hist)) 
	{
        line[i][strlen(line[i]) - 1] = '\0';
        i++;
    }
    tot = i;

#if DEBUG_TERMINAL
    printf("%d\n", num_lines);
    for(i = 0; i < tot; ++i)
    {
        printf(" %s\n", line[i]);
    }
#endif

    if ( line_num >= num_lines ) line_num = num_lines;
    strcpy( terminal->input, line[num_lines-line_num] );

    fclose (t_hist);
}