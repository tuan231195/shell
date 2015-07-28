#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstdlib>

using namespace std;

struct command
{
    char arg[100][500];
    int num_args;
    bool to;
    bool from;
    char srcFile[50];
    char destFile[50];
    bool pipeline;// if we use pipeline
};

void executeSingleCommand(command &);
void executeFullCommand(command &, istringstream &);// execute a full command
bool readCommand(command &, istringstream &);
bool empty(char *);

int main()
{
    char fullCommand[500];
    while (true)
	{
        cout << "$ ";
        cin.getline(fullCommand, 500);
        if (empty(fullCommand))
        {
            continue;
        }
        if (strcmp(fullCommand, "exit") == 0)
            exit(0);
        
        istringstream is(fullCommand);
        command cmd;
        readCommand(cmd, is);
        executeFullCommand(cmd, is);
    }
	return 0;
}


//read a command
bool readCommand (command & cmd, istringstream & is)
{
    cmd.to = false;
    cmd.from = false;
    cmd.pipeline = false;
    bool status;
    status = (is >> cmd.arg[0]);
    cmd.num_args = 1;
    while (is >> cmd.arg[cmd.num_args])
    {
        //stop reading when we get a pipe
        if (strcmp(cmd.arg[cmd.num_args], "|") == 0)
        {
            cmd.pipeline = true;
            break;
        }
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
    return status;
}

//execute a single commnad
void executeSingleCommand(command & cmd)
{
    FILE *dest, *src;
    char *ptr[100];
    //initialise the argument list
    for (int i = 0; i < cmd.num_args; i++)
    {
        ptr[i] = cmd.arg[i];
    }
    ptr[cmd.num_args] = NULL;
    //redirection
    if (cmd.to)
    {
        int fd = 0;
        dest = fopen(cmd.destFile, "w");
        fd = fileno(dest);
        dup2(fd, STDOUT_FILENO);
        fclose(dest);
    }
    if (cmd.from)
    {
        int fd = 0;
        src = fopen(cmd.srcFile, "r");
        fd = fileno(src);
        dup2(fd, STDIN_FILENO);
        fclose(src);
    }
    //execute command
    int result = execvp(ptr[0], ptr);
    if (result == -1)
    {
        cerr << cmd.arg[0] << " does not exist" << endl;
        exit(1);
    }
}


//execute a full command, which can handle multiple pipes
void executeFullCommand(command &cmd, istringstream &is)
{
    pid_t process;
    int data[2][2];
    int curr_pipe = 0;
    int last_pipe;
    int num_command = 0;
    do
    {
        //if pipeline is used, create a pipe
    	if (cmd.pipeline)
    	{
    		pipe(data[curr_pipe]);
        }
        //fork a child
        process = fork();
        if (process == 0)
        {
            if (num_command)
            {
                //get input from the last pipe
                dup2(data[last_pipe][0], STDIN_FILENO);
                close (data[last_pipe][0]);
            }
            //if the command is the last command, execute it
            if (!cmd.pipeline)
            {
                executeSingleCommand(cmd);
            }
            else
            {
               //close read end of the pipe because it is unused
                close(data[curr_pipe][0]);
                 //write to the current pipe
                dup2(data[curr_pipe][1], STDOUT_FILENO);
                close(data[curr_pipe][1]);
                //execute the command
                executeSingleCommand(cmd);
            }
        }
        else if (process < 0)
        {
            cerr << "Unable to fork a child" << endl;
            exit(1);
        }
        //parent process
        else
        {
        	int status;
        	wait(&status);
            //close the write end because it is unused.
            if (cmd.pipeline)
            {
                close(data[curr_pipe][1]);
            }
            //if we finish using the read end of the last pipe, close it for future use.
            if (num_command)
            {
                close(data[last_pipe][0]);
            }
        }
        num_command ++;
        //update curr_pipe and last_pipe
        last_pipe = curr_pipe;
        curr_pipe = (curr_pipe + 1) % 2;
        
    }
    while (readCommand(cmd, is));
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
