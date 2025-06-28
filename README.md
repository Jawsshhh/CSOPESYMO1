Developers: 
Ysabela Erika Alvarez
Julianna Victoria Brizuela
Josh Christian Nuñez

Instructions to run:
1. Clone the repository through Visual Studio 2022
2. Run the main.cpp file.
3. If a 'config.txt' error occurs, create a copy of the config file in the same directory as the projects executeable file (.exe)
    -> this is usually found inside the Debug folder.

A config.txt file must have the following:

num-cpu 
scheduler 
quantum-cycles
batch-process-freq 
min-ins 
max-ins 
delay-per-exec 

1. num-cpu = number of scheduler cpu cores
2. scheduler = scheduler type, can either be fcfs or rr.
3. quantum-cycles = only used in rr, the quantum time that needs to happen before a context switch
4. batch-process-freq = the frequency at which scheduler-start generates a process
5. min-ins = minimum amount of instructions a process can have
6. max-ins = maximum amount of instructions a process can have
7. delay-per-exec = the scheduler's delay per instruction execution.

Example config.txt:
num-cpu 4
scheduler rr
quantum-cycles 5
batch-process-freq 1
min-ins 50
max-ins 75
delay-per-exec 0

To use the app:
1. Initialize -> sets up the program using the config file settings.
2. scheduler-start -> automatically creates processes with a delay equal to batch-process-freq
3. scheduler-end -> stops the process populstion
4. screen -s <name> -> manually creates a screen and its respective process.
5. screen -r <name> -> accesses a process's screen given that it exists/isn't finished.
6. process-smi -> can only be accessed through a process screen and displays that process's instruction logs.
7. screen -ls -> displays running and finished processes as well as CPU utilization.
8. report-util -> same as screen -ls, but outputs it to a 'csopesy.txt' file.
