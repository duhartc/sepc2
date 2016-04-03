/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "variante.h"
#include "readcmd.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */
struct list_bg {
    char * cmd;
    pid_t pid;
    struct list_bg *nxt;
};

#if USE_GUILE == 1
#include <libguile.h>

void list_bg_jobs(struct list_bg **list_bg) {
    printf("Liste des jobs\n");
    bool vide = true;
    bool debut = false;
    struct list_bg *cour = *list_bg;
    struct list_bg *prec = *list_bg;
    while (cour != NULL) {
        vide = false;
        printf("%d %s", cour->pid, cour->cmd);
        if (waitpid(cour->pid, NULL, WNOHANG) != 0) {
            printf(" fini");
            // on supprime le job
            if (cour != *list_bg) {
                //si on n'est pas au début
                prec->nxt = cour->nxt;
            }
            else {
                *list_bg = (*list_bg)->nxt;
                prec = *list_bg;
                debut = true;
            }
            free(cour->cmd);
            free(cour);
            cour = prec;
        }
        printf("\n");
        prec = cour;
        if (!debut) cour = cour->nxt;
        debut = false;
    }
    if (vide) printf("Pas de job en cours\n");
}

 struct list_bg *add_jobs(struct list_bg *list_bg, char * cmd, pid_t pid) {
    // on crée le nouveau job
    struct list_bg *new_job = malloc(sizeof(struct list_bg));
    new_job->pid = pid;
    char * commande = malloc(sizeof(*cmd) * (strlen(cmd) + 1)); 
    new_job->cmd = strcpy(commande, cmd);
    new_job->nxt = NULL;
    printf("Ajout d'un job\n");
    printf("%d %s\n", new_job->pid, new_job->cmd);
    if (list_bg == NULL) {
        // si la liste est vide
        return new_job;
    }
    else {
        struct list_bg * temp = list_bg;
        while(temp->nxt != NULL) {
            temp = temp->nxt;
        }
        temp->nxt = new_job;
        return list_bg;
    }
}

int executer(char *line, struct list_bg **list_bg)
{
	/* Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
        int status;
        int tuyau[2];
        pipe(tuyau);
        struct cmdline *sline = parsecmd(&line);
	if (sline == NULL) {
            //si on a pas de commande
        }
        if (sline->err != 0) {
            // on a eu une erreur dans la commande
            // on doit afficher un message d'erreur
        }
        if (sline->in != 0) {
            //name of file for input redirection
            // question 6
        }
        if (sline->out != 0) {
            //name of file for output redirection
            // question 6
        }

        if (sline->seq != NULL) {
            // pipe multiple ici, à gérer plus tard
            // séparation par pipe
            //for (int i = 0; sline->seq[i]!=0 ;i++) {
                //char **cour_cmd = sline[i];
                // la sortie d'une commande est l'entrée de la suivante
                // TODO
            //}
            int nbCmd = 0;
            while (sline->seq[nbCmd] != 0) {
                nbCmd++;
            }
            // jobs : question 4
            for (int i = 0; i < nbCmd; i++) {
                char **cmd = sline->seq[i];
                if (strcmp(cmd[0],"jobs") == 0) {
                    list_bg_jobs(list_bg);
                } 
                else {
                    pid_t pid;
                    switch(pid = fork()) {
                        case -1:
                            perror("fork: erreur de création de processus fils" ); 
                            break;
                        case 0:
                            // si on est le fils
                            // TODO fonction pour facto
                            if (i == 1) {
                                if (nbCmd > 1) {
                                    dup2(tuyau[0],STDIN_FILENO);
                                    close(tuyau[0]);close(tuyau[1]);
                                }
                                execvp(cmd[0], cmd); 
                            }
                            else if (i == 0) {
                                if (nbCmd > 1) {
                                    dup2(tuyau[1], STDOUT_FILENO);
                                    close(tuyau[1]);close(tuyau[0]);
                                }
                                execvp(cmd[0], cmd);
                            }
                            break;
                        default:
                            // si on est le père
                            if (sline->bg == 0) {  //question 3
                                if (i == nbCmd - 1) {
                                    close(tuyau[1]); close(tuyau[0]);
                                    waitpid(pid, &status ,0); //question 2
                                }
                            }
                            else {
                                //the command must run in background (par defaut)
                                // on ajoute le processus à la liste
                                *list_bg = add_jobs(*list_bg, cmd[0], pid);
                            }
                            break;
                    }
                }
            }
        }
	return 0;
}

SCM executer_wrapper(SCM x, struct list_bg *list_bg)
{
        return scm_from_int(executer(scm_to_locale_stringn(x, 0), &list_bg));
}
#endif


void terminate(char *line) {
#ifdef USE_GNU_READLINE
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#ifdef USE_GUILE
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif
        struct list_bg *list_bg = NULL;
	while (1) {
		//struct cmdline *l;
		char *line=0;
		//int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#ifdef USE_GNU_READLINE
		add_history(line);
#endif


#ifdef USE_GUILE
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif
                executer(line, &list_bg);
	}

}
