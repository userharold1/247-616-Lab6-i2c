#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>
#include <errno.h>

// ------------------- I2C & CAPTEUR --------------------
int writeRegister(int fdPortI2C, uint16_t reg, uint8_t value)
{
    uint8_t buf[3] = { (reg >> 8) & 0xFF, reg & 0xFF, value };
    if (write(fdPortI2C, buf, 3) != 3)
    {
        perror("Erreur écriture registre");
        return -1;
    }
    return 0;
}

int readRegister(int fdPortI2C, uint16_t reg, uint8_t *value)
{
    uint8_t buf[2] = { (reg >> 8) & 0xFF, reg & 0xFF };
    if (write(fdPortI2C, buf, 2) != 2)
    {
        perror("Erreur sélection registre");
        return -1;
    }
    if (read(fdPortI2C, value, 1) != 1)
    {
        perror("Erreur lecture registre");
        return -1;
    }
    return 0;
}

uint8_t lireDistance(int fdPortI2C)
{
    uint8_t dist;
    writeRegister(fdPortI2C, 0x018, 0x01); // Lancer mesure
    usleep(50000); // 50ms pour mesure
    if (readRegister(fdPortI2C, 0x062, &dist) != 0)
    {
        printf("Erreur lecture distance\n");
        return 0;
    }
    return dist;
}

void apply_tuning(int fdPortI2C)
{
    writeRegister(fdPortI2C, 0x0207, 0x01);
    writeRegister(fdPortI2C, 0x0208, 0x01);
    writeRegister(fdPortI2C, 0x0096, 0x00);
    writeRegister(fdPortI2C, 0x0097, 0xFD);
    writeRegister(fdPortI2C, 0x00E3, 0x00);
    writeRegister(fdPortI2C, 0x00E4, 0x04);
    writeRegister(fdPortI2C, 0x00E5, 0x02);
    writeRegister(fdPortI2C, 0x00E6, 0x01);
    writeRegister(fdPortI2C, 0x00E7, 0x03);
    writeRegister(fdPortI2C, 0x00F5, 0x02);
    writeRegister(fdPortI2C, 0x00D9, 0x05);
    writeRegister(fdPortI2C, 0x00DB, 0xCE);
    writeRegister(fdPortI2C, 0x00DC, 0x03);
    writeRegister(fdPortI2C, 0x00DD, 0xF8);
    writeRegister(fdPortI2C, 0x009F, 0x00);
    writeRegister(fdPortI2C, 0x00A3, 0x3C);
    writeRegister(fdPortI2C, 0x00B7, 0x00);
    writeRegister(fdPortI2C, 0x00BB, 0x3C);
    writeRegister(fdPortI2C, 0x00B2, 0x09);
    writeRegister(fdPortI2C, 0x00CA, 0x09);
    writeRegister(fdPortI2C, 0x0198, 0x01);
    writeRegister(fdPortI2C, 0x01B0, 0x17);
    writeRegister(fdPortI2C, 0x01AD, 0x00);
    writeRegister(fdPortI2C, 0x00FF, 0x05);
    writeRegister(fdPortI2C, 0x0100, 0x05);
    writeRegister(fdPortI2C, 0x0199, 0x05);
    writeRegister(fdPortI2C, 0x01A6, 0x1B);
    writeRegister(fdPortI2C, 0x01AC, 0x3E);
    writeRegister(fdPortI2C, 0x01A7, 0x1F);
    writeRegister(fdPortI2C, 0x0030, 0x00);
    writeRegister(fdPortI2C, 0x0011, 0x10);
    writeRegister(fdPortI2C, 0x010A, 0x30);
    writeRegister(fdPortI2C, 0x003F, 0x46);
    writeRegister(fdPortI2C, 0x0031, 0xFF);
    writeRegister(fdPortI2C, 0x0040, 0x63);
    writeRegister(fdPortI2C, 0x002E, 0x01);
    writeRegister(fdPortI2C, 0x001B, 0x09);
    writeRegister(fdPortI2C, 0x003E, 0x31);
    writeRegister(fdPortI2C, 0x0014, 0x24);
    writeRegister(fdPortI2C, 0x0016, 0x00);
}

// ------------------- PIPE & PROCESSUS --------------------
int main()
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 30;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    int pere_fils[2];
    int fils_pf[2];
    int pf_fils[2];

    pipe(pere_fils);
    pipe(fils_pf);
    pipe(pf_fils);

    fcntl(pere_fils[0], F_SETFL, O_NONBLOCK);
    fcntl(fils_pf[0], F_SETFL, O_NONBLOCK);
    fcntl(pf_fils[0], F_SETFL, O_NONBLOCK);

    pid_t pidFils = fork();

    if (pidFils == 0)
    {
        pid_t pidPetitFils = fork();

        if (pidPetitFils == 0)
        {
            // --- PETIT-FILS ---
            close(fils_pf[1]);
            close(pf_fils[0]);

            int fd = open("/dev/i2c-1", O_RDWR);
            if (fd < 0)
            {
                perror("open i2c");
                exit(1);
            }
            ioctl(fd, I2C_SLAVE, 0x29);

            // Appliquer le tuning 
            apply_tuning(fd);

            int lecture_active = 0;
            char commande;

            while (1)
            {
                int n = read(fils_pf[0], &commande, 1);
                if (n > 0)
                {
                    if (commande == 's' || commande == 'S')
                    {
                        lecture_active = 1;
                    }
                    else if (commande == 'q' || commande == 'Q')
                    {
                        close(fd);
                        exit(0);
                    }
                }

                if (lecture_active)
                {
                    uint8_t d = lireDistance(fd);
                    write(pf_fils[1], &d, 1);
                    usleep(300000);
                }
                else
                {
                    usleep(100000);
                }
            }
        }
        else
        {
            // --- FILS ---
            close(pere_fils[1]);
            close(fils_pf[0]);
            close(pf_fils[1]);

            char commande;
            char msg;
            while (1)
            {
                int n = read(pere_fils[0], &commande, 1);
                if (n > 0)
                {
                    write(fils_pf[1], &commande, 1);
                    if (commande == 'q' || commande == 'Q')
                    {
                        exit(0);
                        
                    }
                }

                int r = read(pf_fils[0], &msg, 1);
                if (r > 0)
                {
                    printf("Distance = %d mm\n", msg);
                    fflush(stdout);
                }
                usleep(100000);
            }
        }
    }
    else
    {
        // --- PÈRE ---
        close(pere_fils[0]);
        printf("Appuyez sur 's' + Entrée pour démarrer, 'q' pour quitter\n");

        char c;
        while (1)
        {
            int n = read(STDIN_FILENO, &c, 1);
            if (n > 0)
            {
                write(pere_fils[1], &c, 1);
                if (c == 'q' || c == 'Q')
                {
                    break;
                }
            }
        }

        close(pere_fils[1]);
        wait(NULL);
    }

    return 0;
}
