#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>

using namespace std;


// a single command
struct command
{
    char arg[100][500];
    int num_args;
    bool to;
    bool from;
    char srcFile[50];
    char destFile[50];
    bool pipeline;
};


//a shell variable
struct variable
{
    char key[100];
    char fullCommand[500];
};


//list of variables
variable varlist[100];
int num_var = 0;

void executeSingleCommand(command &, int);
void executeFullCommand(command &, istringstream &);
bool readCommand(command &, istringstream &);
void printVar(command &);
void getVariable(istringstream &);
bool empty(char *);

int main()
{
    char *p;
    
    char fullCommand[500];
    while (true)
	{
        cout << "$ ";
        cin.getline(fullCommand, 500);
        istringstream is(fullCommand);
        //read a variable
        if (strstr(fullCommand, "`") != NULL)
        {
            getVariable(is);
        }
        else
        {
            if (empty(fullCommand))
                continue;
            if (strcmp(fullCommand, "exit") == 0)
                exit(0);
            command cmd;
            readCommand(cmd, is);
            executeFullCommand(cmd, is);
        }
    }
	return 0;
}


void getVariable(istringstream& is)
{
    int pos;
    char ch;
    is.get(varlist[num_var].key, 100, '=');
    //get the name of the variable
    istringstream temp(varlist[num_var].key);
    //name is empty
    if (!(temp >> varlist[num_var].key))
    {
        cerr << "ERROR!!" << endl;
        return;
    }
    //check if the key already exists
    for (pos = 0; pos < num_var; pos++)
    {
        if (strcmp(varlist[num_var].key, varlist[pos].key) == 0)
        {
            break;
        }
    }
    //get =
    is.get(ch);
    //extract whitespaces
    is >> ws;
    //get `
    is.get(ch);
    // if we is not `, report an error
    if (ch != '`')
    {
        cerr << "ERROR!!" << endl;
        return;
    }
    // get full command
    is.get(varlist[pos].fullCommand, 500, '`');
    //if the variable name is not yet in the variable list, then increase the number of variables
    if (pos == num_var)
        num_var ++;
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

//execute a command
void executeSingleCommand(command & cmd, int saveFd)
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
    if (saveFd != -1)
    {
        dup2(saveFd, STDOUT_FILENO);
    }
    //execute command
    int result = execvp(ptr[0], ptr);
    if (result == -1)
    {
        cerr << cmd.arg[0] << " does not exist" << endl;
        exit(1);
    }
}

void executeFullCommand(command &cmd, istringstream &is)
{
    //print the values of shell variables
    if (strcmp(cmd.arg[0], "printvar") == 0)
    {
        printVar(cmd);
        return;
    }
    // execute a full command
    int data[2][2];
    int curr_pipe = 0;
    int last_pipe;
    int num_command = 0;
    pid_t process;
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
            //if the command is the last command, execute it directly
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


//print variables
void printVar(command & cmd)
{
    const char *p;
    for (int i = 1; i < cmd.num_args; i++)
    {
        //get an error, exit the function
        if (cmd.arg[i][0] != '$')
        {
            cerr << "The variable name must be preceded by $";
            return;
        }
        else
        {
            //get the variable name
            p = (cmd.arg[i] + 1);
            for (int i = 0; i < num_var; i++)
            {
                //look for the command corresponding the variable name
                if (strcmp(p, varlist[i].key) == 0)
                {
                    cout << varlist[i].fullCommand << ":" << endl;
                    istringstream is(varlist[i].fullCommand);
                    command var_cmd;
                    readCommand(var_cmd, is);
                    //execute
                    executeFullCommand(var_cmd, is);
                    cout << endl;
                }
            }
        }
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
