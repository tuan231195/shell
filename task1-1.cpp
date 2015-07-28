#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
using namespace std;

//represent a single command
struct command
{
    char arg[100][500];// list of arguments
    int num_args;//number of arguments
};


bool empty(char *);// check if a string is empty
void executeSingleCommand(command &);//execute a command
bool readCommand(command &);//read a command

int main()
{
	command cmd;
	while (true)
	{
		bool result = readCommand(cmd);
        // if the command is not empty, execute the command
        if (result)
            executeSingleCommand(cmd);
    }
	return 0;
}


bool readCommand (command & cmd)
{
    char fullCommand[500];
    cout << "$ ";
    cin.getline(fullCommand, 500);
    //exit the program
    if (strcmp(fullCommand, "exit") == 0)
        exit(0);
    //get an empty string, return false;
    if (empty(fullCommand))
    {
        return false;
    }
    istringstream is(fullCommand);
    is >> cmd.arg[0];// get the command's name
    cmd.num_args = 1;
    //get the command 's arguments
    while (is >> cmd.arg[cmd.num_args])
    {
        cmd.num_args ++;
    }
    return true;
}

//execute a single command
void executeSingleCommand(command & cmd)
{
    pid_t process;
    char *ptr[100];
    for (int i = 0; i < cmd.num_args; i++)
    {
        ptr[i] = cmd.arg[i];
    }
    ptr[cmd.num_args] = NULL;
    //spawn a child process
    process = fork();
    if (process == 0)
    {
        //execute the child process
        int result = execvp(cmd.arg[0], ptr);
        if (result == -1)
        {
            cerr << cmd.arg[0] << " does not exist" << endl;
            exit(1);
        }
    }
    //unable to fork a child process
    else if (process < 0)
    {
        cerr << "Unable to fork a child" << endl;
        exit(1);
    }
    else
    {
        // wait for child's termination
        int status;
        wait(&status);
    }
}


// check if a string is empty
bool empty(char * str)
{
    while (*str != '\0' && *str == ' ')
    {
        str ++;
    }
    return (*str == '\0');
}
