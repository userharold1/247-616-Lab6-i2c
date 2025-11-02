#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>

int main() {
    int fdPortI2C[2];
    pipe(fdPortI2C);  // Création du pipe

    pid_t pid = fork();  // Création du processus enfant

    if (pid == 0) {  
        // --- Processus enfant ---
        close(fdPortI2C[1]);  // Ferme l’extrémité d’écriture
        char message[50];
        read(fdPortI2C[0], message, sizeof(message));
        printf("Enfant a reçu : %s\n", message);
        close(fdPortI2C[0]);
    } else {  
        // --- Processus parent ---
        close(fdPortI2C[0]);  // Ferme l’extrémité de lecture
        char *text = "Salut de ton parent !";
        write(fdPortI2C[1], text, strlen(text) + 1);
        close(fdPortI2C[1]);
    }

    return 0;
}
