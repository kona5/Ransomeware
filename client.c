#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <sys/types.h>
#include <sys/stat.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 4242
#define PROJECT_PATH "TP/Projet"

// Convertit une chaîne hexadécimale en tableau binaire
void hex_to_bytes(const char *hex, unsigned char *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sscanf(hex + 2 * i, "%2hhx", &bytes[i]);
    }
}

// Déchiffre un fichier `.enc` et restaure l’original
int dechiffrer_fichier(const char *chemin, const unsigned char *cle, const unsigned char *iv) {
    FILE *fin = fopen(chemin, "rb");
    if (!fin) return -1;

    char chemin_sortie[512];
    strncpy(chemin_sortie, chemin, strlen(chemin) - 4);  // enlève ".enc"
    chemin_sortie[strlen(chemin) - 4] = '\0';

    FILE *fout = fopen(chemin_sortie, "wb");
    if (!fout) {
        fclose(fin);
        return -2;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, cle, iv);

    unsigned char buffer[1024], buffer_out[1040];
    int len, len_out;

    while ((len = fread(buffer, 1, sizeof(buffer), fin)) > 0) {
        EVP_DecryptUpdate(ctx, buffer_out, &len_out, buffer, len);
        fwrite(buffer_out, 1, len_out, fout);
    }

    EVP_DecryptFinal_ex(ctx, buffer_out, &len_out);
    fwrite(buffer_out, 1, len_out, fout);

    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    remove(chemin);  // supprime le .enc
    return 0;
}

// Parcourt TP/Projet/ pour trouver et déchiffrer les fichiers .enc
void dechiffrer_dossier(const unsigned char *cle, const unsigned char *iv) {
    DIR *dir = opendir(PROJECT_PATH);
    struct dirent *entry;
    char chemin[512];
    struct stat st;

    if (!dir) {
        perror("Erreur d'ouverture de TP/Projet/");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(chemin, sizeof(chemin), "%s/%s", PROJECT_PATH, entry->d_name);

        if (stat(chemin, &st) == 0 && S_ISREG(st.st_mode) && strstr(entry->d_name, ".enc")) {
            printf("🔓 Déchiffrement de : %s\n", chemin);
            dechiffrer_fichier(chemin, cle, iv);
        }
    }

    closedir(dir);
}


int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // 1. Demander le message d'excuse
    printf("🙏 Entrez un message d'excuse (>20 caractères) :\n> ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0; // supprime \n

    // 2. Connexion au serveur
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("❌ Connexion au serveur échouée");
        return 1;
    }

    // 3. Envoi du message
    write(sock, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer));
    close(sock);

    // 4. Vérifie réponse
    if (strncmp(buffer, "CLE=", 4) != 0) {
        printf("⛔ Accès refusé : %s\n", buffer);
        return 1;
    }

    // 5. Récupérer clé et IV
    char cle_hex[65], iv_hex[33];
    sscanf(buffer, "CLE=%64s\nIV=%32s", cle_hex, iv_hex);
    printf("✅ Clé/IV reçues. Déchiffrement en cours...\n");

    unsigned char cle[32], iv[16];
    hex_to_bytes(cle_hex, cle, 32);
    hex_to_bytes(iv_hex, iv, 16);

    // 6. Déchiffrement
    dechiffrer_dossier(cle, iv);
    printf("✅ Tous les fichiers ont été restaurés !\n");

    return 0;
}
