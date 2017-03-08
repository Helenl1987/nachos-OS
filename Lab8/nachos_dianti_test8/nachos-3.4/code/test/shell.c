#include "syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '>';
    prompt[1] = '>';

    while( 1 )
    {
		Write(prompt, 2, output);

		i = 0;
	
		do {
	
	    	Read(&buffer[i], 1, input); 

		} while( buffer[i++] != '\n' );

		buffer[--i] = '\0';

		if(buffer[0] == '.' && buffer[1] == '/') {
			newProc = Exec(buffer+2);
			Join(newProc);
			continue;
		}

		if(buffer[0] == 'p' && buffer[1] == 'w' && buffer[2] == 'd' && buffer[3] == '\0') {
			Pwd();
			continue;
		}

		if(buffer[0] == 'l' && buffer[1] == 's' && buffer[2] == '\0') {
			Ls();
			continue;
		}

		if(buffer[0] == 'e' && buffer[1] == 'c' && buffer[2] == 'h' && buffer[3] == 'o' && buffer[4] == ' ') {
			Write(buffer+5, i-4, output);
			Write("\n", 1, output);
			continue;
		}

		if(buffer[0] == 'q' && buffer[1] == '\0') {
			Exit(0);
		}

		if(buffer[0] == 'c' && buffer[1] == 'd' && buffer[2] == ' ') {
			Cd(buffer+3);
			continue;
		}

		if(buffer[0] == 'c' && buffer[1] == 'f' && buffer[2] == ' ') {
			Create(buffer+3);
			continue;
		}

		if(buffer[0] == 'r' && buffer[1] == 'f' && buffer[2] == ' ') {
			Remove(buffer+3);
			continue;
		}

		if(buffer[0] == 'm' && buffer[1] == 'k' && buffer[2] == 'd' && buffer[3] == 'i' && buffer[4] == 'r' && buffer[5] == ' ') {
			CreateDir(buffer+6);
			continue;
		}

		if(buffer[0] == 'r' && buffer[1] == 'm' && buffer[2] == 'd' && buffer[3] == 'i' && buffer[4] == 'r' && buffer[5] == ' ') {
			RemoveDir(buffer+6);
			continue;
		}

		if(buffer[0] == 'h' && buffer[1] == '\0') {
			Write("################ help ################\n", 39, output);
			Write("h:\t\tshow help information\n", 26, output);
			Write("./xxx:\t\texecute program xxx\n", 28, output);
			Write("pwd:\t\tshow current path\n", 24, output);
			Write("ls:\t\tdisplay current menu\n", 26, output);
			Write("echo xxx:\techo xxx in console\n", 30, output);
			Write("q:\t\tquit console program\n", 25, output);
			Write("cd xxx:\t\tchange directory to xxx\n", 33, output);
			Write("cf xxx:\t\tcreate file xxx\n", 25, output);
			Write("rf xxx:\t\tremove file xxx\n", 25, output);
			Write("mkdir xxx:\tcreate directory xxx\n", 32, output);
			Write("rmdir xxx:\tremove directory xxx\n", 32, output);
			Write("############## end help ##############\n", 39, output);
			continue;
		}

		if( i > 0 ) {
			newProc = Exec(buffer);
			Join(newProc);
		}
    }
}

