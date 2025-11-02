/*
 * VL6180X – Lecture de distance via I2C
 * Carte : Raspberry Pi 5
 * Auteur : Harold Kouadio
 * Description : Lecture du capteur de distance VL6180X via le bus I2C.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_ADDR 0x29              // Adresse I2C du capteur VL6180X
#define I2C_BUS "/dev/i2c-1"      // Nom du bus I2C

// Fonctions utilitaires pour lire et écrire des registres
int writeRegister(int fdPortI2C, uint16_t reg, uint8_t value)
{
    uint8_t Registre16bit[3];
    Registre16bit[0] = (reg >> 8) & 0xFF;
    Registre16bit[1] = reg & 0xFF;
    Registre16bit[2] = value;

    if (write(fdPortI2C, Registre16bit, 3) != 3)
    {
        perror("Erreur écriture registre");
        return -1;
    }
    return 0;
}

int readRegister(int fdPortI2C, uint16_t reg, uint8_t *value)
{
    uint8_t Registre16bit[2];
    Registre16bit[0] = (reg >> 8) & 0xFF;
    Registre16bit[1] = reg & 0xFF;

    if (write(fdPortI2C, Registre16bit, 2) != 2)
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

// Fonction pour lire la distance
uint8_t lireDistance(int fdPortI2C)
{
    uint8_t dist;
    writeRegister(fdPortI2C, 0x018, 0x01); // Lancer mesure
    usleep(50000);                         // Attendre mesure

    if (readRegister(fdPortI2C, 0x062, &dist) != 0)
    {
        printf("Erreur lecture distance\n");
        return 0;
    }
    return dist;
}

int main(void)
{
    uint8_t modelID;
    uint8_t range;

    int fdPortI2C; // File descriptor I2C

    fdPortI2C = open(I2C_BUS, O_RDWR); // Ouverture du port I2C
    if (fdPortI2C == -1)
    {
        perror("Erreur: ouverture du port I2C");
        return 1;
    }

    // Liaison de l'adresse I2C
    if (ioctl(fdPortI2C, I2C_SLAVE, I2C_ADDR) < 0)
    {
        perror("Erreur configuration esclave I2C");
        close(fdPortI2C);
        return 1;
    }

    // Lire l’identifiant du capteur
    if (readRegister(fdPortI2C, 0x000, &modelID) == 0)
    {
        printf("Capteur détecté, MODEL_ID = 0x%02X\n", modelID);
    }
    else
    {
        printf("Erreur de lecture du MODEL_ID\n");
        close(fdPortI2C);
        return 1;
    }

    // Initialisation minimale (voir datasheet)
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
writeRegister(fdPortI2C, 0x0011, 0x10); // Enables polling for ‘New Sample ready’
writeRegister(fdPortI2C, 0x010A, 0x30); // Set averaging sample period
writeRegister(fdPortI2C, 0x003F, 0x46); // Set light and dark gain
writeRegister(fdPortI2C, 0x0031, 0xFF); // Auto calibration period
writeRegister(fdPortI2C, 0x0040, 0x63); // Set ALS integration time to 100ms
writeRegister(fdPortI2C, 0x002E, 0x01); // Temperature calibration
writeRegister(fdPortI2C, 0x001B, 0x09); // Default ranging inter-measurement period
writeRegister(fdPortI2C, 0x003E, 0x31); // Default ALS inter-measurement period
writeRegister(fdPortI2C, 0x0014, 0x24); // Configures interrupt on ‘New Sample Ready’
writeRegister(fdPortI2C, 0x0016, 0x00); // Change fresh out of set status to 0


 printf("Initialisation terminée.\n");

    // Lecture en boucle de la distance
    while (1)
    {
        range = lireDistance(fdPortI2C);
        printf("Distance mesurée : %d mm\n", range);
        usleep(500000);
    }

    close(fdPortI2C);
    return 0;
}
