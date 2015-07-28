#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>

using namespace std;

//a single command
struct command
{
    char arg[100][500];
    int num_args;
    bool to;// if we have redirection to a file
    bool from;// if we have redirection from a file
    char srcFile[50];//source file
    char destFile[50];//destination file
};

void executeSingleCommand(command &);
bool readCommand(command &);
bool empty(char *);

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
    cmd.to = false;
    cmd.from = false;
    
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
    is >> cmd.arg[0];
    cmd.num_args = 1;
    while (is >> cmd.arg[cmd.num_args])
    {
        //redirection
        if (strcmp(cmd.arg[cmd.num_args], "<") == 0)
        {
            cmd.from = true;
            is >> cmd.srcFile;
        }
        if (strcmp(cmd.arg[cmd.num_args], ">") == 0)
        {
            cmd.to = true;
            is >> cmd.destFile;
        }
        if (strcmp(cmd.arg[cmd.num_args], "<") && strcmp(cmd.arg[cmd.num_args], ">"))
        {
            cmd.num_args ++;
        }
    }
    return true;
}

//execute a single command
void executeSingleCommand(command & cmd)
{
    pid_t process;
    FILE *dest, *src;
    //get the argument list
    char *ptr[100];
    for (int i = 0; i < cmd.num_args; i++)
    {
        ptr[i] = cmd.arg[i];
    }
    ptr[cmd.num_args] = NULL;
    process = fork();
    if (process == 0)
    {
        if (cmd.to)
        {
            // redirect output to a file, open the destination file, get the file descriptor and duplicate it
            int fd = 0;
            dest = fopen(cmd.destFile, "w");
            fd = fileno(dest);
            dup2(fd, STDOUT_FILENO);
            fclose(dest);
        }
        if (cmd.from)
        {
            // get input from a file, open the source file, get the file descriptor and duplicate it
            int fd = 0;
            src = fopen(cmd.srcFile, "r");
            fd = fileno(src);
            dup2(fd, STDIN_FILENO);
            fclose(src);
        }
        // execute the command
        int result = execvp(cmd.arg[0], ptr);
        if (result == -1)
        {
            cerr << cmd.arg[0] << " does not exist" << endl;
            exit(1);
        }
    }
    else if (process < 0)
    {
        cerr << "Unable to fork a child" << endl;
        exit(1);
    }
    else
    {
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
