#  TP Final – Ransomware Pédagogique 

## 🎓 Objectif

Simuler un ransomware éducatif en C pour comprendre :

* La surveillance de répertoires
* Le chiffrement AES-256-CBC
* La communication client-serveur via sockets TCP
* Le déclenchement conditionnel et le déchiffrement sur demande

---

## 📁 Structure du projet

```
TP/
├── ransomware.c         # Agent de chiffrement
├── serveur_pardon.c     # Serveur de déblocage
├── client_decrypt.c     # Client pour récupération des fichiers
├── ransomware           # Binaire Linux
├── serveur_pardon       # Binaire Linux
├── client_decrypt       # Binaire Linux
└── Projet/              # Contient les fichiers à protéger (ex: main.c, notes.txt)
```

---

## ⚙️ Compilation (Linux)

Assurez-vous que `libssl-dev` est installé :

```bash
sudo apt update && sudo apt install libssl-dev
```

Puis compilez les fichiers :

```bash
gcc ransomware.c -o ransomware -lssl -lcrypto
gcc serveur.c -o serveur
gcc client.c -o client -lssl -lcrypto
```

---

## 🔄 Fonctionnement

### 1. Lancement

```bash
./serveur_pardon       # Serveur d’administration
./ransomware           # Surveillance du dossier TP/Projet/
```

### 2. Création du dossier

Créez manuellement `TP/Projet/` et placez-y des fichiers `.txt`, `.c`, etc.

### 3. Minuteur

Le ransomware attend 1h, puis :

* Chiffre les fichiers
* Supprime les originaux
* Crée `RANÇON.txt`
* Envoie la clé + IV au serveur

### 4. Déblocage

```bash
./client_decrypt
```

* Saisissez une excuse (minimum 20 caractères)
* Si acceptée, les fichiers sont déchiffrés automatiquement

---

## 🔐 Technologies utilisées

* AES-256-CBC 
* Sockets TCP 
* Parcours de fichiers (dirent.h)
* Manipulation binaire et extensions

---

## 🛡 Avertissement

Ce projet est à but 100% pédagogique. **Ne jamais l’exécuter en dehors d’un environnement contrôlé/test.**

---

## 👩‍💻 Réalisé par

Nancy, M2 Cybersécurité

