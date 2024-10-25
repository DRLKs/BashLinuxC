/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/
#include <string.h>		////// LO ESTOY PONIENDO YO
#include "job_control.h"   // remember to compile with module job_control.c 
#include <pthread.h>
#include <signal.h>
#include <dirent.h>



#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */




job *listaTrabajos;

void traverse_proc(void);		// FUNCIÓN PARA zjobs

int comandoInterno( char * comando );

void manejadorSIGHUP( int senal );

void manejadorSIGCHLD( int senal );

void parse_redirections(char **args, char **file_in, char **file_out);

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */

	listaTrabajos = new_list("Job List	");

	char hdirectory[1024];
	if (getcwd(hdirectory, sizeof(hdirectory)) == NULL) {
        perror("Error al obtener el directorio actual");
        exit(-1);
    }

	char directorioActual[MAX_LINE] = "~";
	
	ignore_terminal_signals();		/*	Esta función ignora las señales del sistema	 */
	signal( SIGCHLD , manejadorSIGCHLD );
	signal(SIGHUP, manejadorSIGHUP);
	
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   
		printf("\e[;31mDavid\e[0m:\e[;35m%s\e[0m$ " , directorioActual );	// Cambiamos el color
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command
		
		char *file_in, *file_out;
    	parse_redirections(args, &file_in, &file_out);

		if( args[0] != NULL ){	/// No Habría ocurrido ningún problema  de 

		////////////////////////////////////////////////////////

		int idComando = comandoInterno( args[0] );
		if( idComando == 1 ){		// Es un comando interno				// cd
			int codigoError;
			
			if( args[1] == NULL || strcmp("~",args[1]) == 0 ){
				codigoError = chdir( hdirectory );	
			}else{
				codigoError = chdir( args[1]);
			}

			if( codigoError < 0 ){
				printf( "No se puede cambiar al directorio %s\n" , args[1] );
			}
			if (getcwd(directorioActual, sizeof(directorioActual)) == NULL) {
				perror("Error al obtener el nuevo directorio");
				exit(-1);
			}

			if( strcmp( directorioActual , hdirectory ) == 0 ){
				directorioActual[0] = '~';
				directorioActual[1] = '\0';
			}

		}else if( idComando == 2 ){		// exit			
			printf("\n\e[;31mSalimos del Shell\e[0m\n");
			exit(0);
		}else if( idComando == 3){		// jobs

			block_SIGCHLD();
			if( listaTrabajos->next != NULL ){
				print_job_list( listaTrabajos );
			}
			unblock_SIGCHLD();
		}else if( idComando == 4 ){		// fg
			
			int pos = 1;
			if( args[1] != NULL ){
				pos = atoi( args[1] ); 
			}
			if( pos > 0 && pos <= list_size( listaTrabajos ) ){
				block_SIGCHLD();
				pid_t pidBashDavid = getpid();
				job *trabajo = get_item_bypos( listaTrabajos , pos );
				pid_t pidTrabajo = trabajo->pgid;
				set_terminal( pidTrabajo );
				killpg( pidTrabajo , SIGCONT );
				waitpid( pidTrabajo , &status , WUNTRACED );
				set_terminal( pidBashDavid );
				enum status estado = analyze_status( status , &info );
				if( estado == SIGNALED ){
					printf( "\nBackground pid: %d, command: %s, SIGNALED, info: %d\n" , trabajo->pgid , trabajo->command, info);
					delete_job( listaTrabajos , trabajo );
				}else if( estado == EXITED ){
					printf( "\nBackground pid: %d, command: %s, EXITED, info: %d\n" , trabajo->pgid , trabajo->command, info);			
					delete_job( listaTrabajos , trabajo );
				}else if( estado == SUSPENDED ){
					trabajo->state = STOPPED;
					printf( "\nBackground pid: %d, command: %s, STOPPED, info: %d\n" , trabajo->pgid , trabajo->command, info );
				}
				unblock_SIGCHLD();
				}else if( args[1] == NULL ){
					printf( "No existe ningún proceso pendiente \n");
				}else{
					printf( "No existe tal proceso en la [%s] posicion \n" , args[1] );
				}
		}else if( idComando == 5 ){		// bg
			int pos = 1;
			if( args[1] != NULL ){
				pos = atoi( args[1] );
			}
			if( pos > 0 && pos <= list_size( listaTrabajos ) ){
				block_SIGCHLD();
				job *trabajo = get_item_bypos( listaTrabajos , pos );
				trabajo->state = BACKGROUND;
				unblock_SIGCHLD();
				killpg( trabajo->pgid , SIGCONT );
			}else if( args[1] == NULL ){
				printf( "No existe ningún proceso pendiente \n");
			}else{
				printf( "No existe tal proceso en la [%s] posicion \n" , args[1] );
			}
				
		}else if(idComando == 6){
			int pos = 1;
			if( args[1] != NULL ){
				pos = atoi( args[1] );
			}
			if( pos > 0 && pos <= list_size( listaTrabajos ) ){
				block_SIGCHLD();
				job *trabajo = get_item_bypos( listaTrabajos , pos );
				printf("Trabajo actual: PID=%d command=%s\n" , trabajo->pgid , trabajo->command);
				unblock_SIGCHLD();
				killpg( trabajo->pgid , SIGCONT );
			}else if( args[1] == NULL ){
				printf( "No existe ningún proceso pendiente \n");
			}else{
				printf( "No existe tal proceso en la [%s] posicion \n" , args[1] );
			}

		}else if( idComando == 7){
			int pos = 1;
			if( args[1] != NULL ){
				pos = atoi( args[1] );
			}
			if( args[1] == NULL || args[2] == NULL ){
				printf("El comando bgteam requiere dos argumentos\n");
			}else if( pos > 0 ){
				char *argsAux[MAX_LINE - 2];
				int tamanyo = 0;
				while (args[tamanyo] != NULL) {
        			tamanyo++;
    			}
				int idx = 2;
				for(idx ; idx < tamanyo ; idx++ ){
					argsAux[idx - 2] = args[idx];
				}
				argsAux[idx - 2] = NULL;

				pid_t pidBgTeam;
				while( pos > 0 ){
					pidBgTeam = fork();

					if( pidBgTeam < 0 ){
						perror("Error, al crear proceso hijo");
						exit(-1);
					}else if( pidBgTeam == 0 ){
						new_process_group( getpid() );
						restore_terminal_signals();
						execvp( args[2] , argsAux);
						
						printf("Error, el comando '%s' no fue encontrado\n" , argsAux[0] );
						pos = 0;
						exit(-1);
						
					}else{
						block_SIGCHLD();
						job * trabajo = new_job( pidBgTeam , argsAux[0] , BACKGROUND );
						add_job( listaTrabajos , trabajo );
						unblock_SIGCHLD();
						}
					pos = pos - 1;
					}	
				}

			}else if( idComando == 8 ){		// deljob
				if( listaTrabajos->next == NULL ){
					printf("No hay trabajo actual\n");
				}else{
					int pos = 1;
					if( args[1] != NULL ){
						pos = atoi( args[1] );
					}
					if( pos > 0 && pos <= list_size( listaTrabajos ) ){
						block_SIGCHLD();
						job *trabajo = get_item_bypos( listaTrabajos , pos );
						if( STOPPED == trabajo->state ){
							printf("No se permiten borrar trabajos en segundo plano suspendidos\n");
						}else{
							printf("Borrando trabajo actual de la lista de jobs: PID=%d command=%s\n" , trabajo->pgid , trabajo->command );
							delete_job( listaTrabajos , trabajo );
						}
						unblock_SIGCHLD();
					}else{
						printf("No hay trabajo actual\n");
					}

				}
			}else if( idComando == 9 ){	// zjob
				traverse_proc();
			}else if( idComando == 10){	// mask
				int argsSignals[32];		// Hay 31 señales en total, aqui las guardaremos
				int idx = 1;
				while( args[idx] != NULL && strcmp( args[idx] , "-c") == 1 ){		// No es nulo y es diferente de "-c", entonces itera
					argsSignals[idx - 1] = atoi( args[idx] );

					idx++;
				}
				argsSignals[idx + 1] = -1;

				if( args[idx] == NULL ) printf("Debes indicar '-c'\n");		// Hay error
				else{														// No hay error
					int posArgsAux = 0;
					char *argsAux[MAX_LINE - idx];
					idx++;

					while( args[idx] != NULL ){
						argsAux[posArgsAux] = args[idx];
						idx++;
						posArgsAux++;
					}
					argsAux[posArgsAux + 1] = NULL;

					pid_t pidMaskFunction = fork();

					if( pidMaskFunction < 0 ){
						printf("Error, en la creación del proceso hijo");
						exit(-1);
					}else if( pidMaskFunction == 0 ){
						
						new_process_group( getpid() );

						set_terminal( getpid() );
						

						restore_terminal_signals();

					

						idx = 0;
						while( argsSignals[idx] != -1 ){
							signal( argsSignals[idx] , 0 );
							++idx;
						}

						execvp( argsAux[0] , argsAux );
					
					}else{
						waitpid( pidMaskFunction , &status , WUNTRACED );
					}
					
				}


			}else{

			pid_fork = fork();

			if( pid_fork < 0 ){
				perror("Error, al crear proceso hijo");
				exit(-1);
			}else if( pid_fork == 0 ){
				new_process_group( getpid() );

				if( background == 0 ){	// FG
					set_terminal( getpid() );
				}

				restore_terminal_signals();

				if( args[0] == NULL ){		// Error en la redirección
					exit(-1);
				}

				if( file_in != NULL ){
					freopen(file_in, "r", stdin);
					/*
					int fd_in = open( file_in );	
					dup2(fd_in, STDIN_FILENO);
					close(fd_in);
					*/
				}

				if( file_out != NULL ){
					freopen(file_out, "w", stdout);
					/*
					int fd_out = open( file_out );	
					dup2(fd_out, STDOUT_FILENO);
					close(fd_out);
					*/
				}
			
				execvp( args[0] , args);
				printf("Error, el comando '%s' no fue encontrado\n" , args[0] );
				exit(-1);
			}else{	// pid_fork > 0, proceso Padre
				if( background == 0  ){							// Proceso en FG

					waitpid( pid_fork ,&status, WUNTRACED );	// WUNTRACED es una opción de la función waitpid()
					set_terminal( getpid() );
					enum status estado = analyze_status( status , &info);
					if( SUSPENDED == estado ) {
						block_SIGCHLD();
						add_job( listaTrabajos , new_job( pid_fork , args[0] , STOPPED ) );	//BACKGROUND y me da más nota
						unblock_SIGCHLD();
						
					}else if( estado == SIGNALED ){
						printf( "Foreground pid: %d, command: %s, SIGNALED, info: %d\n" , pid_fork , args[0], info);
					}else if( estado == EXITED ){
						printf( "Foreground pid: %d, command: %s, EXITED, info: %d\n" , pid_fork , args[0] , info);			
					}

				}else{											// Proceso en BG
					printf("Background job running... pid: %d , command: %s\n" , pid_fork , args[0] );
					block_SIGCHLD();
					add_job( listaTrabajos , new_job( pid_fork , args[0] , BACKGROUND ) );
					unblock_SIGCHLD();
				}

				/*
				enum status estado = analyze_status( status , &info );

				
				*/
				}
			}
		}
	} // end while
}

int comandoInterno( char * comando ){

	
	int res = 0;
	if( strcmp( "cd" , comando ) == 0  ){
		res = 1;
	}else if( (strcmp( "exit" , comando ) == 0) || (strcmp( "EXIT" , comando ) == 0) ){
		res = 2;
	}else if( strcmp( "jobs" , comando ) == 0 ){
		res = 3;
	}else if( strcmp( "fg" , comando ) == 0 ){
		res = 4;
	}else if( strcmp( "bg" , comando ) == 0 ){
		res = 5;
	}else if( strcmp( "currjob" , comando ) == 0 ){
		res = 6;
	}else if( strcmp( "bgteam" , comando ) == 0 ){
		res = 7;
	}else if( strcmp( "deljob" , comando ) == 0 ){
		res = 8;
	}else if( strcmp( "zjobs" , comando ) == 0 ){
		res = 9;
	}else if( strcmp( "mask" , comando ) == 0 ){
		res = 10;
	}
	return res;

}

void manejadorSIGCHLD( int senal ){

	pid_t pid;
	job *trabajo;
	int status;

	for( int idx = list_size( listaTrabajos ); idx > 0 ; idx-- ){

		trabajo = get_item_bypos( listaTrabajos , idx );
		if( trabajo->state != FOREGROUND ){
			pid = waitpid( trabajo->pgid , &status , WUNTRACED | WNOHANG | WCONTINUED );

			if( pid == trabajo->pgid ) {
				block_SIGCHLD();
				int info;
				enum status estado = analyze_status( status , &info );

				if( estado == SIGNALED ){
					printf( "Background pid: %d, command: %s, SIGNALED, info: %d\n" , pid , trabajo->command, info);
					delete_job( listaTrabajos , trabajo );
				}else if( estado == EXITED ){
					printf( "Background pid: %d, command: %s, EXITED, info: %d\n" , pid , trabajo->command, info);			
					delete_job( listaTrabajos , trabajo );
				}else if( estado == SUSPENDED ){
					trabajo->state = STOPPED;
					printf( "Background pid: %d, command: %s, STOPPED, info: %d\n" , pid , trabajo->command, info );
				}
				unblock_SIGCHLD();
			}
		}
	}

}

void parse_redirections(char **args, char **file_in, char **file_out)
{
	*file_in = NULL;
	*file_out = NULL;
	char **args_start = args;
	while (*args)
	{
		int is_in = !strcmp(*args, "<");
		int is_out = !strcmp(*args, ">");
		if (is_in || is_out)
		{
			args++;
			if (*args)
			{
				if (is_in)
					*file_in = *args;
				if (is_out)
					*file_out = *args;
				char **aux = args + 1;
				while (*aux)
				{
					*(aux - 2) = *aux;
					aux++;
				}
				*(aux - 2) = NULL;
				args--;
			}
			else
			{
				/* Syntax error */
				fprintf(stderr, "syntax error in redirection\n");
				args_start[0] = NULL; // Do nothing
			}
		}
		else
		{
			args++;
		}
	}
}

void manejadorSIGHUP( int senal){
	
    FILE *fp =fopen("hup.txt","a"); // abre un fichero en modo 'append'
	if (fp == NULL) {
        perror("Error al abrir o crear el archivo");
		exit(-1);
    }
	fprintf(fp, "SIGHUP recibido.\n"); //escribe en el fichero
	fclose( fp );
}

void traverse_proc(void) {
    DIR *d; 
    struct dirent *dir;
    char buff[2048];
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            sprintf(buff, "/proc/%s/stat", dir->d_name); 
            FILE *fd = fopen(buff, "r");
            if (fd){
                long pid;     // pid
                long ppid;    // ppid
                char state;   // estado: R (runnable), S (sleeping), T(stopped), Z (zombie)

                // La siguiente línea lee pid, state y ppid de /proc/<pid>/stat
                fscanf(fd, "%ld %s %c %ld", &pid, buff, &state, &ppid);

				if( state == 'Z' && ppid == getpid() ) printf("%ld \n" , pid);

                fclose(fd);
            }
        }
        closedir(d);
    }
}