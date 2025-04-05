#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#define GREEN 32
#define RED 31
#define YELLOW 33
#define BLUE 34

typedef struct etat {
    int num_etat;
    bool isFinal;
    bool isInitial;
    int count;
} etat;

typedef struct transition {
    etat source;
    etat dest;
    char *etiquette; 
    struct transition* suivant;
} transition;

typedef struct Automate {
    etat *Etats;             // Tableau dynamique d'etats
    transition *transitions; // Liste chainee de transitions
    int nbEtats;
    int nbTransitions;
    char **alphabet;
    int nbAlphabets;
} Automate;

// Fonction pour afficher un message avec une couleur
void afficherMessage(const char* message, int color) {
    printf("\033[1;%dm%s\033[0m\n", color, message);
}

// Creer une transition
transition* creerTransition(etat source, etat dest, const char* etiquette) {
    transition* nouvelleTransition = malloc(sizeof(transition));
    if (!nouvelleTransition) {
        printf("Erreur d'allocation memoire pour la transition.\n");
        return NULL;
    }
    nouvelleTransition->source = source;
    nouvelleTransition->dest = dest;
    nouvelleTransition->etiquette = strdup(etiquette);  // Alloue et copie l'etiquette
    if (!nouvelleTransition->etiquette) {
        printf("Erreur d'allocation memoire pour l'etiquette.\n");
        free(nouvelleTransition);
        return NULL;
    }
    nouvelleTransition->suivant = NULL;
    return nouvelleTransition;
}

void ajouterEtat(Automate* automate, int num_etat, bool isInitial, bool isFinal) {
    // Verifier si l'etat existe deja
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].num_etat == num_etat) {
            if (isInitial) automate->Etats[i].isInitial = true;
            if (isFinal)   automate->Etats[i].isFinal = true;
            return;
        }
    }
    // Ajouter un nouvel etat
    automate->Etats = realloc(automate->Etats, (automate->nbEtats + 1) * sizeof(etat));
    if (!automate->Etats) {
        printf("Erreur de reallocation memoire pour les etats.\n");
        return;
    }
    automate->Etats[automate->nbEtats].num_etat  = num_etat;
    automate->Etats[automate->nbEtats].isInitial = isInitial;
    automate->Etats[automate->nbEtats].isFinal   = isFinal;
    automate->Etats[automate->nbEtats].count = 0;
    automate->nbEtats++;
}

// Fonction pour ajouter un alphabet sans doublon
void ajouterAlphabet(Automate* automate, const char* etiquette) {
    // Separer l'etiquette en plusieurs symboles si elle contient une virgule
    char copie[128];
    strcpy(copie, etiquette);

    char *token = strtok(copie, ","); // Separer par ","
    while (token != NULL) {
        // Verifier si l'alphabet existe deja
        bool existe = false;
        for (int i = 0; i < automate->nbAlphabets; i++) {
            if (strcmp(automate->alphabet[i], token) == 0) {
                existe = true;
                break;
            }
        }
        // Ajouter si non existant
        if (!existe) {
            automate->alphabet = realloc(automate->alphabet, (automate->nbAlphabets + 1) * sizeof(char*));
            automate->alphabet[automate->nbAlphabets] = strdup(token);
            automate->nbAlphabets++;
        }
        token = strtok(NULL, ","); // Passer au prochain element
    }
}

// Ajouter une transition a l'automate
void ajouterTransition(Automate* automate, etat source, etat dest, const char* etiquette) {
    // Vérifier si la transition existe déjà
    transition* courant = automate->transitions;
    while (courant != NULL) {
        if (courant->source.num_etat == source.num_etat &&
            courant->dest.num_etat == dest.num_etat &&
            strcmp(courant->etiquette, etiquette) == 0) {
            // La transition existe déjà, ne pas l'ajouter
            return;
        }
        courant = courant->suivant;
    }

    // Créer une nouvelle transition
    transition* nouvelleTransition = creerTransition(source, dest, etiquette);
    if (!nouvelleTransition) return;

    nouvelleTransition->suivant = automate->transitions;
    automate->transitions = nouvelleTransition;
    automate->nbTransitions++;

    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].num_etat == source.num_etat) {
            automate->Etats[i].count++;
            break;
        }
    }

    // Ajouter les états source et destination, s'ils n'existent pas déjà
    ajouterEtat(automate, source.num_etat, source.isInitial, source.isFinal);
    ajouterEtat(automate, dest.num_etat, dest.isInitial, dest.isFinal);
    ajouterAlphabet(automate, etiquette);
}

// Afficher les transitions de l'automate
void afficherTransitions(Automate* automate) {
    if (automate->nbTransitions == 0) {
        printf("Aucune transition dans l'automate.\n");
        return;
    }
    printf("Transitions de l'automate :\n");
    transition* courant = automate->transitions;
    while (courant != NULL) {
        printf("  %d -> %d [label=\"%s\"]\n",
               courant->source.num_etat, courant->dest.num_etat, courant->etiquette);
        courant = courant->suivant;
    }
}

void lireAutomateDepuisDot(Automate* automate, const char* fichier) {
    FILE* f = fopen(fichier, "r");
    if (!f) {
        printf("Erreur : Impossible d'ouvrir le fichier %s\n", fichier);
        return;
    }

    char ligne[256];
    int source_num, dest_num;
    char etiquette[128];

    while (fgets(ligne, sizeof(ligne), f)) {
        // Detection des etats initiaux avec plusieurs variables "startX"
        if (strstr(ligne, "start") && strstr(ligne, "->")) {
            char* ptr = ligne;
            while ((ptr = strstr(ptr, "start")) != NULL) { // Trouver chaque occurrence de "start"
                if (sscanf(ptr, "start%*[^-] -> %d", &source_num) == 1) {
                    ajouterEtat(automate, source_num, true, false);
                }
                ptr += 5; // Avancer pour eviter de retomber sur le meme "start"
            }
            continue;
        }

        // Detection des etats finaux
        if (strstr(ligne, "final") && strstr(ligne, "->")) {
            if (sscanf(ligne, " %d -> final", &source_num) == 1 ||
                sscanf(ligne, "%d->final", &source_num) == 1) {
                ajouterEtat(automate, source_num, false, true);
            }
            continue;
        }

        // Detection des transitions
        if (strstr(ligne, "->") && strstr(ligne, "[label")) {
            if (sscanf(ligne, " %d -> %d [label = \"%127[^\"]\"]", &source_num, &dest_num, etiquette) == 3 ||
                sscanf(ligne, " %d->%d[label=\"%127[^\"]\"]", &source_num, &dest_num, etiquette) == 3) {
                etat source = {source_num, false, false, 0};
                etat dest = {dest_num, false, false, 0};
                ajouterTransition(automate, source, dest, etiquette);
            }
            continue;
        }
    }

    fclose(f);
}

void afficher_Etat_avec_etique(Automate *automate, char etique) {
    printf("Les etats avec l'etiquette %c sont :\n", etique);
    bool trouve = false;

    transition* courant = automate->transitions;
    while (courant) {
        if (strchr(courant->etiquette, etique)) {
            printf("%d\n", courant->source.num_etat);
            trouve = true;
        }
        courant = courant->suivant;
    }

    if (!trouve) {
        printf("Aucun etat trouve avec l'etiquette '%c'.\n", etique);
    }
}

// Afficher les etats de l'automate
void afficherEtats(Automate* automate) {
    printf("Etats de l'automate :\n");
    if (automate->nbEtats == 0) {
        printf("  Aucun etat.\n");
        return;
    }
    for (int i = 0; i < automate->nbEtats; i++) {
        etat e = automate->Etats[i];
        printf("  Etat %d : ", e.num_etat);
        if (e.isInitial) { printf("Initial "); 
        }else if (e.isFinal)   { printf("Final "); 
        }else{printf("Normal");}
        printf("\n");
    }
}

// Fonction pour afficher les alphabets
void afficherAlphabet(Automate* automate) {
    printf("\nAlphabets de l'automate : ");
    if (automate->nbAlphabets == 0) {
        printf("Aucun alphabet.\n");
        return;
    }
    for (int i = 0; i < automate->nbAlphabets; i++) {
        printf("\"%s\" ", automate->alphabet[i]);
    }
    printf("\n");
}

// Afficher la liste des etats
void afficherListeEtats(Automate* automate) {
    printf("\nListe des Etats : ");
    if (automate->nbEtats == 0) {
        printf("Aucun etat.\n");
        return;
    }
    for (int i = 0; i < automate->nbEtats; i++) {
        printf("%d ", automate->Etats[i].num_etat);
    }
    printf("\n");
}

// Afficher la liste des etiquettes des transitions
void afficherListeEtiquettes(Automate* automate) {
    printf("\nListe des Etiquettes : ");
    if (automate->nbTransitions == 0) {
        printf("Aucune transition.\n");
        return;
    }
    transition* courant = automate->transitions;
    while (courant != NULL) {
        printf("\"%s\" ", courant->etiquette);
        courant = courant->suivant;
    }
    printf("\n");
}

void genererDotDepuisAutomate(Automate* automate, const char* nomFichier) {
    FILE* file = fopen(nomFichier, "w");
    if (!file) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    fprintf(file, "digraph Automate {\n    rankdir=LR;  // Orientation de gauche à droite\n\n");

    // Ajouter les états
    fprintf(file, "      node [shape = circle];");
    for (int i = 0; i < automate->nbEtats; i++) {
        fprintf(file, " %d", automate->Etats[i].num_etat);
    }
    fprintf(file, ";\n\n");

    // Ajouter les états initiaux
    fprintf(file, "       node [shape = point width =0]");
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].isInitial) {
            fprintf(file, " start%d", automate->Etats[i].num_etat);
        }
    }
    fprintf(file, ";\n");
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].isInitial) {
            fprintf(file, "    start%d -> %d;\n", automate->Etats[i].num_etat, automate->Etats[i].num_etat);
        }
    }
    fprintf(file, "\n");

    // Ajouter les états finaux
    fprintf(file, "       node [shape = point];");
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].isFinal) {
            fprintf(file, " final%d", automate->Etats[i].num_etat);
        }
    }
    fprintf(file, ";\n");
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].isFinal) {
            fprintf(file, "    %d -> final%d;\n", automate->Etats[i].num_etat, automate->Etats[i].num_etat);
        }
    }
    fprintf(file, "\n");

    // Ajouter les transitions
    fprintf(file, "    // Transitions\n");
    transition* courant = automate->transitions;
    while (courant != NULL) {
        fprintf(file, "    %d -> %d [label = \"%s\"];\n",
                courant->source.num_etat, courant->dest.num_etat, courant->etiquette);
        courant = courant->suivant;
    }

    fprintf(file, "}\n");
    fclose(file);
}

void Etat_avec_max_Trans_sortants(Automate* automate) {
    if (automate->nbEtats == 0) {
        printf("Aucun etat dans l'automate.\n");
        return;
    }
    
    int max = 0;
    // Parcourir le tableau des etats pour trouver le maximum de transitions sortantes.
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].count > max)
            max = automate->Etats[i].count;
    }
    
    if (max == 0) {
        printf("Aucune transition dans l'automate.\n");
        return;
    }
    
    printf("Les etats avec le plus de transitions (%d transitions) sont :\n", max);
    // Afficher tous les etats dont le nombre de transitions correspond au maximum.
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].count == max) {
            printf("Etat %d\n", automate->Etats[i].num_etat);
        }
    }
}

// Fonction recursive pour explorer tous les chemins possibles
bool verifierMotRecursif(Automate* automate, etat* etatActuel, const char* mot, int index) {
    // Si on a atteint la fin du mot, verifier si l'etat actuel est final
    if (mot[index] == '\0') {
        return etatActuel->isFinal;
    }

    // Parcourir toutes les transitions pour trouver celles qui correspondent au caractere actuel
    transition* courant = automate->transitions;
    while (courant != NULL) {
        // Verifier si la transition part de l'etat actuel et si l'etiquette contient le caractere actuel
        if (courant->source.num_etat == etatActuel->num_etat && strchr(courant->etiquette, mot[index])) {
            // Trouver l'etat de destination dans le tableau des etats
            etat* dest = NULL;
            for (int i = 0; i < automate->nbEtats; i++) {
                if (automate->Etats[i].num_etat == courant->dest.num_etat) {
                    dest = &automate->Etats[i];
                    break;
                }
            }

            // Explorer recursivement ce chemin
            if (dest != NULL && verifierMotRecursif(automate, dest, mot, index + 1)) {
                return true; // Si un chemin valide est trouve, retourner vrai
            }
        }
        courant = courant->suivant;
    }

    // Aucun chemin valide trouve
    return false;
}

// Fonction principale pour verifier un mot
bool verifierMot(Automate* automate, char mot[100]) {
    bool trouve = false;

    // Parcourir tous les etats initiaux
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].isInitial) {
            // Verifier si un chemin valide existe depuis cet etat initial
            if (verifierMotRecursif(automate, &automate->Etats[i], mot, 0)) {
                trouve = true;
                break; // Un chemin valide a ete trouve, pas besoin de continuer
            }
        }
    }
    // Afficher le resultat
    if (trouve) {
        printf("Le mot \"%s\" est reconnu !\n", mot);
    } else {
        printf("Le mot \"%s\" n'est pas reconnu.\n", mot);
    }
    return trouve;
}

//--------------------------------fichier.txt---------------------------------
void mots_depuis_fichier_txt(Automate* automate, const char* fichier) {
    FILE* f = fopen(fichier, "r");
    if (!f) {
        printf("Erreur : Impossible d'ouvrir le fichier %s\n", fichier);
        return;
    }
   
    char mot[100];
    while (fscanf(f, "%s", mot) == 1) {
        verifierMot(automate, mot);
        }
    fclose(f);
}


Automate unionAutomates(Automate* automateA, Automate* automateB, const char* nomFichierUnion) {
    Automate automateC;
    memset(&automateC, 0, sizeof(Automate));  // Initialiser le nouvel automate

    // Trouver le numéro d'état maximal dans automateA pour éviter les conflits
    int maxEtatA = 0;
    for (int i = 0; i < automateA->nbEtats; i++) {
        if (automateA->Etats[i].num_etat > maxEtatA) {
            maxEtatA = automateA->Etats[i].num_etat;
        }
    }

    // Ajouter les états de A à C
    for (int i = 0; i < automateA->nbEtats; i++) {
        ajouterEtat(&automateC, automateA->Etats[i].num_etat, false, false);  // Ne pas marquer comme initial ou final
    }

    // Ajouter les états de B à C en renommant les états pour éviter les conflits
    for (int i = 0; i < automateB->nbEtats; i++) {
        int newNumEtat = automateB->Etats[i].num_etat + maxEtatA + 1;  // Renommer les états de B
        ajouterEtat(&automateC, newNumEtat, false, false);  // Ne pas marquer comme initial ou final
    }

    // Ajouter les transitions de A à C
    transition* courantA = automateA->transitions;
    while (courantA != NULL) {
        ajouterTransition(&automateC, courantA->source, courantA->dest, courantA->etiquette);
        courantA = courantA->suivant;
    }

    // Ajouter les transitions de B à C en renommant les états
    transition* courantB = automateB->transitions;
    while (courantB != NULL) {
        etat source = {courantB->source.num_etat + maxEtatA + 1, false, false, 0};
        etat dest = {courantB->dest.num_etat + maxEtatA + 1, false, false, 0};
        ajouterTransition(&automateC, source, dest, courantB->etiquette);
        courantB = courantB->suivant;
    }

    // Ajouter les alphabets de A et B à C
    for (int i = 0; i < automateA->nbAlphabets; i++) {
        ajouterAlphabet(&automateC, automateA->alphabet[i]);
    }
    for (int i = 0; i < automateB->nbAlphabets; i++) {
        ajouterAlphabet(&automateC, automateB->alphabet[i]);
    }

    // Créer un nouvel état initial pour l'automate union
    int nouvelEtatInitial = maxEtatA + 1000;  // Un numéro unique pour le nouvel état initial
    
    ajouterEtat(&automateC, nouvelEtatInitial, true, false);  // Marquer comme état initial uniquement

    // Ajouter des transitions du nouvel état initial vers les états initiaux de A et B
    for (int i = 0; i < automateA->nbEtats; i++) {
        if (automateA->Etats[i].isInitial) {
            etat source = {nouvelEtatInitial, false, false, 0};
            etat dest = {automateA->Etats[i].num_etat, false, false, 0};
            ajouterTransition(&automateC, source, dest, "ε");  // Transition epsilon
        }
    }
    for (int i = 0; i < automateB->nbEtats; i++) {
        if (automateB->Etats[i].isInitial) {
            etat source = {nouvelEtatInitial, false, false, 0};
            etat dest = {automateB->Etats[i].num_etat + maxEtatA + 1, false, false, 0};
            ajouterTransition(&automateC, source, dest, "ε");  // Transition epsilon
        }
    }

    // Créer un nouvel état final pour l'automate union
    int nouvelEtatFinal = maxEtatA + 1001;  // Un numéro unique pour le nouvel état final
    ajouterEtat(&automateC, nouvelEtatFinal, false, true);  // Marquer comme état final uniquement

    // Ajouter des transitions des états finaux de A et B vers le nouvel état final
    for (int i = 0; i < automateA->nbEtats; i++) {
        if (automateA->Etats[i].isFinal) {
            etat source = {automateA->Etats[i].num_etat, false, false, 0};
            etat dest = {nouvelEtatFinal, false, false, 0};
            ajouterTransition(&automateC, source, dest, "ε");  // Transition epsilon
        }
    }
    for (int i = 0; i < automateB->nbEtats; i++) {
        if (automateB->Etats[i].isFinal) {
            etat source = {automateB->Etats[i].num_etat + maxEtatA + 1, false, false, 0};
            etat dest = {nouvelEtatFinal, false, false, 0};
            ajouterTransition(&automateC, source, dest, "ε");  // Transition epsilon
        }
    }
for (int i = 0; i < automateC.nbEtats; i++) {
    if (automateC.Etats[i].num_etat != nouvelEtatInitial) {
        automateC.Etats[i].isInitial = false;  // Seul le nouvel état initial est marqué comme initial
    }
}
for (int i = 0; i < automateC.nbEtats; i++) {
    if (automateC.Etats[i].num_etat != nouvelEtatFinal) {
        automateC.Etats[i].isFinal = false;  // Seul le nouvel état initial est marqué comme initial
    }
}
    // Générer le fichier .dot de l'union
    genererDotDepuisAutomate(&automateC, nomFichierUnion);
    printf("Fichier .dot de l'union généré : %s\n", nomFichierUnion);

    
    return automateC;
}
//-------------------------------concatenation-------------------------
Automate concatenation_deux_automates(Automate* A1, Automate* A2) {
    Automate A3;
    memset(&A3, 0, sizeof(Automate));
   
    // Calculer le décalage pour les états du second automate
    int maxEtatA = 0;
    for (int i = 0; i < A1->nbEtats; i++) {
        if (A1->Etats[i].num_etat > maxEtatA) {
            maxEtatA = A1->Etats[i].num_etat;
        }
    }

    // Copie des états de A1 (avec état initial mais pas final)
    for (int i = 0; i < A1->nbEtats; i++) {
        ajouterEtat(&A3, A1->Etats[i].num_etat, 
                    A1->Etats[i].isInitial,  // Garder l'état initial si c'était un état initial
                    false);                   // Ne pas marquer comme final
    }

    // Copie des états de A2 (avec état final mais pas initial)
    for (int i = 0; i < A2->nbEtats; i++) {
        int newNumEtat = A2->Etats[i].num_etat + maxEtatA + 1;
        ajouterEtat(&A3, newNumEtat,
                    false,                    // Ne pas marquer comme initial
                    A2->Etats[i].isFinal);   // Garder l'état final si c'était un état final
    }

    // Copie des transitions de A1
    transition* courant = A1->transitions;
    while (courant != NULL) {
        etat source = {courant->source.num_etat, false, false, 0};
        etat dest = {courant->dest.num_etat, false, false, 0};
        ajouterTransition(&A3, source, dest, courant->etiquette);
        ajouterAlphabet(&A3, courant->etiquette);
        courant = courant->suivant;
    }

    // Copie des transitions de A2 avec décalage
    transition* courant2 = A2->transitions;
    while (courant2 != NULL) {
        etat source = {courant2->source.num_etat + maxEtatA + 1, false, false, 0};
        etat dest = {courant2->dest.num_etat + maxEtatA + 1, false, false, 0};
        ajouterTransition(&A3, source, dest, courant2->etiquette);
        ajouterAlphabet(&A3, courant2->etiquette);
        courant2 = courant2->suivant;
    }

    // Ajout des transitions ε entre états finaux de A1 et états initiaux de A2
    for (int i = 0; i < A1->nbEtats; i++) {
        if (A1->Etats[i].isFinal) {
            for (int j = 0; j < A2->nbEtats; j++) {
                if (A2->Etats[j].isInitial) {
                    etat source = {A1->Etats[i].num_etat, false, false, 0};
                    etat dest = {A2->Etats[j].num_etat + maxEtatA + 1, false, false, 0};
                    ajouterTransition(&A3, source, dest, "ε");
                }
            }
        }
    }

    return A3;
}
   
    



// void calculerEpsilonFermeture(Automate* automate, int etat, int* fermeture, int* taille, bool* visite) {
//     // Ajouter l'état courant à la fermeture s'il n'est pas déjà visité
//     if (!visite[etat]) {
//         visite[etat] = true;
//         fermeture[(*taille)++] = etat;
        
//         // Rechercher toutes les epsilon-transitions à partir de cet état
//         transition* courant = automate->transitions;
//         while (courant != NULL) {
//             if (courant->source.num_etat == etat && strcmp(courant->etiquette, "ε") == 0) {
//                 // Appel récursif pour chaque epsilon-transition
//                 calculerEpsilonFermeture(automate, courant->dest.num_etat, fermeture, taille, visite);
//             }
//             courant = courant->suivant;
//         }
//     }
// }

void supprimerEpsilonTransitions(Automate* automate) {
    // Créer un nouvel automate vide pour le résultat
    Automate resultat = {0};

    // Etape 1: Copier tous les états dans le nouvel automate
    for (int i = 0; i < automate->nbEtats; i++) {
        ajouterEtat(&resultat, automate->Etats[i].num_etat, 
                    automate->Etats[i].isInitial, automate->Etats[i].isFinal);
    }

    // Etape 2: Calculer les fermetures epsilon de manière simple
    // Pour chaque état, on stocke les états atteignables par epsilon transitions
    int fermetureEpsilon[100][100]; // Maximum 100 états, et chaque état peut avoir max 100 états dans sa fermeture
    int nbEtatsfermeture[100]; // Nombre d'états dans la fermeture de chaque état
    
    // Initialiser les tableaux
    for (int i = 0; i < 100; i++) {
        nbEtatsfermeture[i] = 0;
    }
    
    // Pour chaque état de l'automate
    for (int i = 0; i < automate->nbEtats; i++) {
        // Inclure l'état lui-même dans sa fermeture
        fermetureEpsilon[i][nbEtatsfermeture[i]] = i;
        nbEtatsfermeture[i]++;
        
        // Marquer les états visités
        bool dejaVu[100];
        for (int j = 0; j < 100; j++) {
            dejaVu[j] = false;
        }
        dejaVu[i] = true;
        
        // On continue tant qu'on trouve de nouveaux états
        bool nouveauxEtats = true;
        while (nouveauxEtats) {
            nouveauxEtats = false;
            
            // Pour chaque état dans la fermeture actuelle
            for (int j = 0; j < nbEtatsfermeture[i]; j++) {
                int etatActuel = fermetureEpsilon[i][j];
                
                
                transition* trans = automate->transitions;
                while (trans != NULL) {
                    if (trans->source.num_etat == automate->Etats[etatActuel].num_etat && 
                        strcmp(trans->etiquette, "ε") == 0) {
                        
                        // Trouver l'indice de l'état de destination
                        int etatDest = -1;
                        for (int k = 0; k < automate->nbEtats; k++) {
                            if (automate->Etats[k].num_etat == trans->dest.num_etat) {
                                etatDest = k;
                                break;
                            }
                        }
                        
                        // Si on a trouvé l'état destination et qu'on ne l'a pas encore vu
                        if (etatDest != -1 && !dejaVu[etatDest]) {
                            // On l'ajoute à la fermeture
                            fermetureEpsilon[i][nbEtatsfermeture[i]] = etatDest;
                            nbEtatsfermeture[i]++;
                            dejaVu[etatDest] = true;
                            nouveauxEtats = true;  // On a trouvé un nouvel état, donc on continue
                        }
                    }
                    trans = trans->suivant;
                }
            }
        }
    }
    
    // Etape 3: Marquer les états finaux
    // Si un état peut atteindre un état final par epsilon-transitions, il devient final
    for (int i = 0; i < automate->nbEtats; i++) {
        // Chercher l'état correspondant dans le nouvel automate
        int indiceEtat = -1;
        for (int j = 0; j < resultat.nbEtats; j++) {
            if (resultat.Etats[j].num_etat == automate->Etats[i].num_etat) {
                indiceEtat = j;
                break;
            }
        }
        
        // Vérifier si un des états de sa fermeture est final
        for (int j = 0; j < nbEtatsfermeture[i]; j++) {
            int etatFermeture = fermetureEpsilon[i][j];
            if (automate->Etats[etatFermeture].isFinal) {
                resultat.Etats[indiceEtat].isFinal = true;
                break;
            }
        }
    }
    
    // Etape 4: Ajouter les transitions non-epsilon
    for (int i = 0; i < automate->nbEtats; i++) {
        // Pour chaque état dans la fermeture
        for (int j = 0; j < nbEtatsfermeture[i]; j++) {
            int etatFermeture = fermetureEpsilon[i][j];
            
            // Parcourir toutes les transitions
            transition* trans = automate->transitions;
            while (trans != NULL) {
                // Si c'est une transition non-epsilon depuis un état de la fermeture
                if (trans->source.num_etat == automate->Etats[etatFermeture].num_etat && 
                    strcmp(trans->etiquette, "ε") != 0) {
                    
                    // Trouver l'état source dans le nouvel automate
                    etat sourceNouveau;
                    for (int k = 0; k < resultat.nbEtats; k++) {
                        if (resultat.Etats[k].num_etat == automate->Etats[i].num_etat) {
                            sourceNouveau = resultat.Etats[k];
                            break;
                        }
                    }
                    
                    // Trouver l'état destination dans le nouvel automate
                    etat destNouveau;
                    for (int k = 0; k < resultat.nbEtats; k++) {
                        if (resultat.Etats[k].num_etat == trans->dest.num_etat) {
                            destNouveau = resultat.Etats[k];
                            break;
                        }
                    }
                    
                    // Ajouter la transition
                    ajouterTransition(&resultat, sourceNouveau, destNouveau, trans->etiquette);
                }
                trans = trans->suivant;
            }
        }
    }
    
    // Etape 5: Supprimer les états inaccessibles
    // On va utiliser un tableau simple pour marquer les états accessibles
    bool accessible[100];
    for (int i = 0; i < 100; i++) {
        accessible[i] = false;
    }
    
    // Marquer les états initiaux comme accessibles
    for (int i = 0; i < resultat.nbEtats; i++) {
        if (resultat.Etats[i].isInitial) {
            accessible[i] = true;
        }
    }
    
    // Répéter jusqu'à ce qu'on ne trouve plus de nouveaux états accessibles
    bool changement = true;
    while (changement) {
        changement = false;
        
        // Pour chaque transition
        transition* trans = resultat.transitions;
        while (trans != NULL) {
            // Trouver l'indice de l'état source dans le nouvel automate
            int indiceSource = -1;
            for (int i = 0; i < resultat.nbEtats; i++) {
                if (resultat.Etats[i].num_etat == trans->source.num_etat) {
                    indiceSource = i;
                    break;
                }
            }
            
            // Trouver l'indice de l'état destination dans le nouvel automate
            int indiceDest = -1;
            for (int i = 0; i < resultat.nbEtats; i++) {
                if (resultat.Etats[i].num_etat == trans->dest.num_etat) {
                    indiceDest = i;
                    break;
                }
            }
            
            // Si l'état source est accessible et que l'état destination ne l'est pas encore
            if (indiceSource != -1 && indiceDest != -1 && 
                accessible[indiceSource] && !accessible[indiceDest]) {
                accessible[indiceDest] = true;
                changement = true;  // On a trouvé un nouvel état accessible
            }
            
            trans = trans->suivant;
        }
    }
    
    // Créer l'automate final avec seulement les états accessibles
    Automate automateFinal = {0};
    
    // Ajouter les états accessibles
    for (int i = 0; i < resultat.nbEtats; i++) {
        if (accessible[i]) {
            ajouterEtat(&automateFinal, resultat.Etats[i].num_etat,
                      resultat.Etats[i].isInitial, resultat.Etats[i].isFinal);
        }
    }
    
    // Ajouter les transitions entre états accessibles
    transition* trans = resultat.transitions;
    while (trans != NULL) {
        // Trouver les indices des états source et destination
        int indiceSource = -1, indiceDest = -1;
        
        for (int i = 0; i < resultat.nbEtats; i++) {
            if (resultat.Etats[i].num_etat == trans->source.num_etat) {
                indiceSource = i;
            }
            if (resultat.Etats[i].num_etat == trans->dest.num_etat) {
                indiceDest = i;
            }
        }
        
        // Si les deux états sont accessibles, ajouter la transition
        if (indiceSource != -1 && indiceDest != -1 && 
            accessible[indiceSource] && accessible[indiceDest]) {
            ajouterTransition(&automateFinal, trans->source, trans->dest, trans->etiquette);
        }
        
        trans = trans->suivant;
    }
    
    // Remplacer l'automate original par l'automate résultat
    *automate = automateFinal;
}
//------------------------------------------crer simple automate--------------------------------------
// Automate creer_automate_simple(char c) {
//     Automate automate;
//     memset(&automate, 0, sizeof(Automate)); 

//     ajouterEtat(&automate, 0, true, false);
//     ajouterEtat(&automate, 1, false, false);
    
//     char transition[2] = {c, '\0'}; // Assurer que c est bien traité comme une chaîne
//     ajouterTransition(&automate, automate.Etats[0], automate.Etats[1], transition);

//     automate.nbEtats += 2;
//     automate.nbTransitions++;
//     ajouterAlphabet(&automate, transition); 

//     return automate;
// }

void etoile_automate(Automate *src, Automate *dest) {
    // Initialiser l'automate de destination
    memset(dest, 0, sizeof(Automate));
    
    // Copier l'alphabet
    for (int i = 0; i < src->nbAlphabets; i++) {
        ajouterAlphabet(dest, src->alphabet[i]);
    }
    
    // Déterminer l'état initial et final de l'automate source
    int etat_initial_src = -1;
    int etat_final_src = -1;
    
    for (int i = 0; i < src->nbEtats; i++) {
        if (src->Etats[i].isInitial) {
            etat_initial_src = src->Etats[i].num_etat;
        }
        if (src->Etats[i].isFinal) {
            etat_final_src = src->Etats[i].num_etat;
        }
    }
    
    if (etat_initial_src == -1 || etat_final_src == -1) {
        fprintf(stderr, "Erreur: automate sans état initial ou final\n");
        exit(EXIT_FAILURE);
    }
    
    // Créer un nouvel état initial et un nouvel état final
    int nouvel_etat_initial = src->nbEtats;
    int nouvel_etat_final = src->nbEtats + 1;
    
    // Copier tous les états de src vers dest
    for (int i = 0; i < src->nbEtats; i++) {
        ajouterEtat(dest, src->Etats[i].num_etat, false, false);
    }
    
    // Ajouter les nouveaux états initial et final
    ajouterEtat(dest, nouvel_etat_initial, true, false);
    ajouterEtat(dest, nouvel_etat_final, false, true);
    
    // Copier toutes les transitions
    transition *courant = src->transitions;
    while (courant != NULL) {
        ajouterTransition(dest, (etat){courant->source.num_etat, false, false, 0}, (etat){courant->dest.num_etat, false, false, 0}, courant->etiquette);
        courant = courant->suivant;
    }
    
    // Ajouter les transitions spécifiques pour l'opérateur étoile:
    
    // 1. Transition ε du nouvel état initial vers l'état initial original
    ajouterTransition(dest, (etat){nouvel_etat_initial, false, false, 0}, (etat){etat_initial_src, false, false, 0}, "ε");
    
    // 2. Transition ε de l'état final original vers le nouvel état final
    ajouterTransition(dest, (etat){etat_final_src, false, false, 0}, (etat){nouvel_etat_final, false, false, 0}, "ε");
    
    // 3. Transition ε directe du nouvel état initial vers le nouvel état final (zéro occurrence)
    ajouterTransition(dest, (etat){nouvel_etat_initial, false, false, 0}, (etat){nouvel_etat_final, false, false, 0},  "ε");
    
    // 4. Transition ε de l'état final original vers l'état initial original (pour la répétition)
    ajouterTransition(dest, (etat){etat_final_src, false, false, 0}, (etat){etat_initial_src, false, false, 0}, "ε");
    
    // Si "ε" n'est pas déjà dans l'alphabet, l'ajouter
    bool epsilon_exist = false;
    for (int i = 0; i < dest->nbAlphabets; i++) {
        if (strcmp(dest->alphabet[i], "ε") == 0) {
            epsilon_exist = true;
            break;
        }
    }
    
    if (!epsilon_exist) {
        ajouterAlphabet(dest, "ε");
    }
}
//----------------------------------------
void plus_automate(Automate *src, Automate *dest) {
    // Initialiser l'automate de destination
    memset(dest, 0, sizeof(Automate));
    
    // Copier l'alphabet
    for (int i = 0; i < src->nbAlphabets; i++) {
        ajouterAlphabet(dest, src->alphabet[i]);
    }
    
    // Copier tous les états de src vers dest
    for (int i = 0; i < src->nbEtats; i++) {
        ajouterEtat(dest, src->Etats[i].num_etat, src->Etats[i].isInitial, src->Etats[i].isFinal);
    }
    
    // Copier toutes les transitions
    transition *courant = src->transitions;
    while (courant != NULL) {
        ajouterTransition(dest, (etat){courant->source.num_etat, false, false, 0}, (etat){courant->dest.num_etat, false, false, 0}, courant->etiquette);
        courant = courant->suivant;
    }
    
    // Ajouter les transitions spécifiques pour l'opérateur plus:
    // Transition ε de l'état final original vers l'état initial original (pour la répétition)
    for (int i = 0; i < src->nbEtats; i++) {
        if (src->Etats[i].isFinal) {
            for (int j = 0; j < src->nbEtats; j++) {
                if (src->Etats[j].isInitial) {
                    ajouterTransition(dest, (etat){src->Etats[i].num_etat, false, false, 0}, (etat){src->Etats[j].num_etat, false, false, 0}, "ε");
                }
            }
        }
    }
    
    // Si "ε" n'est pas déjà dans l'alphabet, l'ajouter
    bool epsilon_exist = false;
    for (int i = 0; i < dest->nbAlphabets; i++) {
        if (strcmp(dest->alphabet[i], "ε") == 0) {
            epsilon_exist = true;
            break;
        }
    }
    
    if (!epsilon_exist) {
        ajouterAlphabet(dest, "ε");
    }
}
//------------------------------------------


Automate* construireAutomateFromRegex(const char* regex) {
    if (!regex || strlen(regex) == 0) {
        return NULL;
    }

    Automate* courant = NULL; // Automate en cours de construction
    Automate* base = NULL;
    int longeur = strlen(regex); // Longueur de l'expression régulière
    int i = 0;
    char c;

    while (i < longeur) {
        c = regex[i];

        // 1. Traitement des caractères normaux
        if (c != '+' && c != '*' && c != '/' && c != '(' && c != ')') {
            base = malloc(sizeof(Automate));
            memset(base, 0, sizeof(Automate));
            char str[2] = {c, '\0'};
            ajouterEtat(base, 0, true, false);
            ajouterEtat(base, 1, false, true);
            ajouterTransition(base, base->Etats[0], base->Etats[1], str);
            ajouterAlphabet(base, str);
            i++;

            // Vérifier si le prochain caractère est un opérateur * ou +
            if (i < longeur && (regex[i] == '*' || regex[i] == '+')) {
                Automate* temp = malloc(sizeof(Automate));
                if (regex[i] == '*') {
                    etoile_automate(base, temp); // Appliquer l'étoile à la lettre
                } else if (regex[i] == '+') {
                    plus_automate(base, temp); // Appliquer le plus à la lettre
                }
                free(base);
                base = temp;
                i++; // Passer l'opérateur
            }

            // Concaténation implicite
            if (courant == NULL) {
                courant = base;
            } else {
                Automate* temp = malloc(sizeof(Automate));
                *temp = concatenation_deux_automates(courant, base);
                free(courant);
                free(base);
                courant = temp;
            }
        }

        // 2. Traitement des parenthèses
        else if (c == '(') {
            i++; // Sauter la parenthèse ouvrante
            int debut = i; // Début de sous-expression
            int niveau = 1; // Niveau de parenthèse

            // Trouver la parenthèse fermante correspondante
            while (i < longeur && niveau > 0) {
                if (regex[i] == '(') niveau++;
                if (regex[i] == ')') niveau--;
                i++;
            }

            if (niveau != 0) {
                printf("Erreur : Parenthèses non équilibrées !\n");
                if (courant) free(courant);
                return NULL;
            }

            int longueur_sousEXP = i - debut - 1;
            char* sous_expr = (char*)malloc(longueur_sousEXP + 1);
            if (!sous_expr) {
                fprintf(stderr, "Erreur d'allocation mémoire\n");
                if (courant) free(courant);
                return NULL;
            }

            strncpy(sous_expr, regex + debut, longueur_sousEXP);
            sous_expr[longueur_sousEXP] = '\0';

            base = construireAutomateFromRegex(sous_expr); // Appel récursif
            free(sous_expr);

            if (base != NULL) {
                // Vérifier si le prochain caractère est un opérateur * ou +
                if (i < longeur && (regex[i] == '*' || regex[i] == '+')) {
                    Automate* temp = malloc(sizeof(Automate));
                    if (regex[i] == '*') {
                        etoile_automate(base, temp); // Appliquer l'étoile à l'expression entre parenthèses
                    } else if (regex[i] == '+') {
                        plus_automate(base, temp); // Appliquer le plus à l'expression entre parenthèses
                    }
                    free(base);
                    base = temp;
                    i++; // Passer l'opérateur
                }

                // Concaténation implicite
                if (courant == NULL) {
                    courant = base;
                } else {
                    Automate* temp = malloc(sizeof(Automate));
                    *temp = concatenation_deux_automates(courant, base);
                    free(courant);
                    free(base);
                    courant = temp;
                }
            }
        }

        // 3. Traitement des opérateurs / pour l'union
        else if (c == '/') {
            i++;
            int debut = i;
            int niveau = 0;
            int j = i;

            // Trouver la partie droite de l'union
            while (j < longeur && (niveau > 0 || (regex[j] != '/' && regex[j] != ')'))) {
                if (regex[j] == '(') niveau++;
                if (regex[j] == ')') niveau--;
                j++;
            }

            int longueur = j - debut;
            char* sous_expr = (char*)malloc(longueur + 1);
            if (!sous_expr) {
                fprintf(stderr, "Erreur d'allocation mémoire\n");
                if (courant) free(courant);
                return NULL;
            }

            strncpy(sous_expr, regex + debut, longueur);
            sous_expr[longueur] = '\0';

            Automate* droite = construireAutomateFromRegex(sous_expr);
            free(sous_expr);

            if (courant != NULL && droite != NULL) {
                Automate* temp = malloc(sizeof(Automate));
                *temp = unionAutomates(courant, droite, "temp.dot");
                free(courant);
                free(droite);
                courant = temp;
                i = j; // Avancer après l'expression droite
            } else {
                printf("Erreur : Opérateur / avec expression invalide !\n");
                if (courant) free(courant);
                if (droite) free(droite);
                return NULL;
            }
        }
        else if (c == ')') {
            i++;
        }
    }

    return courant;
}


// Fonction pour supprimer les états inaccessibles d'un automate
void supprimerEtatsInaccessibles(Automate* automate) {
    if (automate->nbEtats == 0) {
        return;
    }

    // Tableau pour marquer les états accessibles
    bool* accessibles = (bool*)calloc(automate->nbEtats, sizeof(bool));
    if (!accessibles) {
        printf("Erreur d'allocation mémoire\n");
        return;
    }

    // Marquer les états initiaux comme accessibles
    for (int i = 0; i < automate->nbEtats; i++) {
        if (automate->Etats[i].isInitial) {
            accessibles[i] = true;
        }
    }

    // Propagation de l'accessibilité
    bool changement = true;
    while (changement) {
        changement = false;
        
        transition* courant = automate->transitions;
        while (courant != NULL) {
            // Trouver l'indice de l'état source
            int indiceSource = -1;
            for (int i = 0; i < automate->nbEtats; i++) {
                if (automate->Etats[i].num_etat == courant->source.num_etat) {
                    indiceSource = i;
                    break;
                }
            }

            // Trouver l'indice de l'état destination
            int indiceDest = -1;
            for (int i = 0; i < automate->nbEtats; i++) {
                if (automate->Etats[i].num_etat == courant->dest.num_etat) {
                    indiceDest = i;
                    break;
                }
            }

            // Si la source est accessible et la destination ne l'est pas encore
            if (indiceSource != -1 && indiceDest != -1 && 
                accessibles[indiceSource] && !accessibles[indiceDest]) {
                accessibles[indiceDest] = true;
                changement = true;
            }
            
            courant = courant->suivant;
        }
    }

    // Créer un nouvel automate avec seulement les états accessibles
    Automate nouveauAutomate = {0};
    
    // Ajouter les états accessibles
    for (int i = 0; i < automate->nbEtats; i++) {
        if (accessibles[i]) {
            ajouterEtat(&nouveauAutomate, automate->Etats[i].num_etat,
                       automate->Etats[i].isInitial, automate->Etats[i].isFinal);
        }
    }

    // Ajouter les transitions entre états accessibles
    transition* trans = automate->transitions;
    while (trans != NULL) {
        // Vérifier si source et destination sont accessibles
        bool sourceAccessible = false, destAccessible = false;
        for (int i = 0; i < automate->nbEtats; i++) {
            if (accessibles[i] && automate->Etats[i].num_etat == trans->source.num_etat) {
                sourceAccessible = true;
            }
            if (accessibles[i] && automate->Etats[i].num_etat == trans->dest.num_etat) {
                destAccessible = true;
            }
        }

        if (sourceAccessible && destAccessible) {
            ajouterTransition(&nouveauAutomate, trans->source, trans->dest, trans->etiquette);
        }
        
        trans = trans->suivant;
    }

    // Copier les alphabets
    for (int i = 0; i < automate->nbAlphabets; i++) {
        ajouterAlphabet(&nouveauAutomate, automate->alphabet[i]);
    }

    // Libérer l'ancien automate et copier le nouveau
    free(automate->Etats);
    free(automate->alphabet);
    
    // Libérer les transitions de l'ancien automate
    transition* courantTrans = automate->transitions;
    while (courantTrans != NULL) {
        transition* suivant = courantTrans->suivant;
        free(courantTrans->etiquette);
        free(courantTrans);
        courantTrans = suivant;
    }

    // Copier le nouvel automate
    *automate = nouveauAutomate;

    free(accessibles);
}

Automate intersectionAutomates(Automate* automateA, Automate* automateB, const char* FichierIntersection) {
    Automate automateC;
    memset(&automateC, 0, sizeof(Automate));
    
    // 1. Vérifier s'il y a un alphabet commun
    bool alphabetCommun = false;
    for (int i = 0; i < automateA->nbAlphabets; i++) {
        for (int j = 0; j < automateB->nbAlphabets; j++) {
            if (strcmp(automateA->alphabet[i], automateB->alphabet[j]) == 0) {
                alphabetCommun = true;
                break;
            }
        }
        if (alphabetCommun) break;
    }

    if (!alphabetCommun) {
        printf("Aucun alphabet commun - l'intersection est vide.\n");
        // Créer un automate vide avec un état initial (pour la validité)
        ajouterEtat(&automateC, 0, true, false);
        genererDotDepuisAutomate(&automateC, FichierIntersection) ;
        return automateC;
    }

    // 2. Créer les états produits (paires d'états)
    for (int i = 0; i < automateA->nbEtats; i++) {
        for (int j = 0; j < automateB->nbEtats; j++) {
            int newNumEtat = automateA->Etats[i].num_etat * 1000 + automateB->Etats[j].num_etat;
            bool isInitial = automateA->Etats[i].isInitial && automateB->Etats[j].isInitial;
            bool isFinal = automateA->Etats[i].isFinal && automateB->Etats[j].isFinal;
            ajouterEtat(&automateC, newNumEtat, isInitial, isFinal);
        }
    }

    // 3. Ajouter les transitions communes
    transition* courantA = automateA->transitions;
    while (courantA != NULL) {
        transition* courantB = automateB->transitions;
        while (courantB != NULL) {
            // Vérifier les symboles communs dans les étiquettes (qui peuvent être multiples)
            char* copyA = strdup(courantA->etiquette);
            char* tokenA = strtok(copyA, ",");
            while (tokenA != NULL) {
                char* copyB = strdup(courantB->etiquette);
                char* tokenB = strtok(copyB, ",");
                while (tokenB != NULL) {
                    if (strcmp(tokenA, tokenB) == 0) {
                        // Créer les états source et destination produits
                        int sourceNum = courantA->source.num_etat * 1000 + courantB->source.num_etat;
                        int destNum = courantA->dest.num_etat * 1000 + courantB->dest.num_etat;
                        
                        // Trouver les états dans automateC
                        etat source = {0}, dest = {0};
                        bool foundSource = false, foundDest = false;
                        
                        for (int k = 0; k < automateC.nbEtats; k++) {
                            if (automateC.Etats[k].num_etat == sourceNum) {
                                source = automateC.Etats[k];
                                foundSource = true;
                            }
                            if (automateC.Etats[k].num_etat == destNum) {
                                dest = automateC.Etats[k];
                                foundDest = true;
                            }
                            if (foundSource && foundDest) break;
                        }
                        
                        if (foundSource && foundDest) {
                            ajouterTransition(&automateC, source, dest, tokenA);
                        }
                    }
                    tokenB = strtok(NULL, ",");
                }
                free(copyB);
                tokenA = strtok(NULL, ",");
            }
            free(copyA);
            courantB = courantB->suivant;
        }
        courantA = courantA->suivant;
    }

    // 4. Ajouter les alphabets communs
    for (int i = 0; i < automateA->nbAlphabets; i++) {
        for (int j = 0; j < automateB->nbAlphabets; j++) {
            if (strcmp(automateA->alphabet[i], automateB->alphabet[j]) == 0) {
                ajouterAlphabet(&automateC, automateA->alphabet[i]);
            }
        }
    }

    supprimerEtatsInaccessibles(&automateC);

    genererDotDepuisAutomate(&automateC, FichierIntersection);

    return automateC;
}

Automate determinisation(Automate a) {
    Automate resultat;
    memset(&resultat, 0, sizeof(Automate)); // Initialiser l'automate résultant

    // Copier l'alphabet
    resultat.alphabet = malloc(a.nbAlphabets * sizeof(char*));
    for (int i = 0; i < a.nbAlphabets; i++) {
        resultat.alphabet[i] = strdup(a.alphabet[i]);
    }
    resultat.nbAlphabets = a.nbAlphabets;

    int etatsComposites[100][100]; // Liste des états composites
    int taillesComposites[100];    // Taille de chaque état composite
    int nbEtatsComposites = 0;

    // Initialiser l'état initial composite
    taillesComposites[0] = 0;
    for (int i = 0; i < a.nbEtats; i++) {
        if (a.Etats[i].isInitial) {
            etatsComposites[0][taillesComposites[0]++] = a.Etats[i].num_etat;
        }
    }
    nbEtatsComposites++;
    ajouterEtat(&resultat, 0, true, false);

    for (int i = 0; i < nbEtatsComposites; i++) {
        // Vérifier si l'état composite est final
        for (int j = 0; j < taillesComposites[i]; j++) {
            for (int k = 0; k < a.nbEtats; k++) {
                if (a.Etats[k].num_etat == etatsComposites[i][j] && a.Etats[k].isFinal) {
                    resultat.Etats[i].isFinal = true;
                    break;
                }
            }
        }

        // Parcourir toutes les étiquettes de l'alphabet
        for (int j = 0; j < a.nbAlphabets; j++) {
            char* etiquette = a.alphabet[j];
            int nouvelEtat[100];
            int tailleNouvelEtat = 0;

            // Trouver les états accessibles avec l'étiquette
            for (int k = 0; k < taillesComposites[i]; k++) {
                for (transition* t = a.transitions; t != NULL; t = t->suivant) {
                    if (t->source.num_etat == etatsComposites[i][k] &&
                        strcmp(t->etiquette, etiquette) == 0) {
                        bool existe = false;
                        for (int l = 0; l < tailleNouvelEtat; l++) {
                            if (nouvelEtat[l] == t->dest.num_etat) {
                                existe = true;
                                break;
                            }
                        }
                        if (!existe) {
                            nouvelEtat[tailleNouvelEtat++] = t->dest.num_etat;
                        }
                    }
                }
            }

            // Vérifier si le nouvel état composite existe déjà
            int indexNouvelEtat = -1;
            for (int k = 0; k < nbEtatsComposites; k++) {
                if (taillesComposites[k] == tailleNouvelEtat) {
                    bool identique = true;
                    for (int l = 0; l < tailleNouvelEtat; l++) {
                        if (etatsComposites[k][l] != nouvelEtat[l]) {
                            identique = false;
                            break;
                        }
                    }
                    if (identique) {
                        indexNouvelEtat = k;
                        break;
                    }
                }
            }

            // Si le nouvel état composite n'existe pas, l'ajouter
            if (indexNouvelEtat == -1 && tailleNouvelEtat > 0) {
                indexNouvelEtat = nbEtatsComposites;
                for (int l = 0; l < tailleNouvelEtat; l++) {
                    etatsComposites[nbEtatsComposites][l] = nouvelEtat[l];
                }
                taillesComposites[nbEtatsComposites] = tailleNouvelEtat;
                nbEtatsComposites++;
                ajouterEtat(&resultat, indexNouvelEtat, false, false);
            }

            // Ajouter la transition
            if (tailleNouvelEtat > 0) {
                ajouterTransition(&resultat, 
                                  (etat){i, false, false, 0}, 
                                  (etat){indexNouvelEtat, false, false, 0}, 
                                  etiquette);
            }
        }
    }

    return resultat;
}

Automate renversement(Automate a) {
    Automate resultat;
    memset(&resultat, 0, sizeof(Automate)); // Initialiser l'automate résultant

    // Copier les états
    for (int i = 0; i < a.nbEtats; i++) {
        ajouterEtat(&resultat, a.Etats[i].num_etat, false, false);
    }

    // Inverser les transitions
    transition* courant = a.transitions;
    while (courant != NULL) {
        ajouterTransition(&resultat, courant->dest, courant->source, courant->etiquette);
        courant = courant->suivant;
    }

    // Inverser les états initiaux et finaux
    for (int i = 0; i < a.nbEtats; i++) {
        if (a.Etats[i].isInitial) {
            for (int j = 0; j < resultat.nbEtats; j++) {
                if (resultat.Etats[j].num_etat == a.Etats[i].num_etat) {
                    resultat.Etats[j].isFinal = true;
                    break;
                }
            }
        }
        if (a.Etats[i].isFinal) {
            for (int j = 0; j < resultat.nbEtats; j++) {
                if (resultat.Etats[j].num_etat == a.Etats[i].num_etat) {
                    resultat.Etats[j].isInitial = true;
                    break;
                }
            }
        }
    }

    return resultat;
}

Automate minimisationBrzozowski(Automate a) {
    // Étape 1 : Inversion
    Automate inverse = renversement(a);

    // Étape 2 : Déterminisation de l'inverse
    Automate determiniseInverse = determinisation(inverse);

    // Étape 3 : Inversion de nouveau
    Automate inverseDeterminise = renversement(determiniseInverse);

    // Étape 4 : Déterminisation de l'inverse déterminisé
    Automate resultat = determinisation(inverseDeterminise);

    // Regrouper les états dans l'automate résultant
    for (int i = 0; i < resultat.nbEtats; i++) {
        for (int j = i + 1; j < resultat.nbEtats; j++) {
            if (resultat.Etats[i].isFinal == resultat.Etats[j].isFinal &&
                resultat.Etats[i].isInitial == resultat.Etats[j].isInitial) {
                // Fusionner les états i et j
                for (transition* t = resultat.transitions; t != NULL; t = t->suivant) {
                    if (t->source.num_etat == resultat.Etats[j].num_etat) {
                        t->source.num_etat = resultat.Etats[i].num_etat;
                    }
                    if (t->dest.num_etat == resultat.Etats[j].num_etat) {
                        t->dest.num_etat = resultat.Etats[i].num_etat;
                    }
                }
                // Supprimer l'état j
                for (int k = j; k < resultat.nbEtats - 1; k++) {
                    resultat.Etats[k] = resultat.Etats[k + 1];
                }
                resultat.nbEtats--;
                j--; // Réévaluer l'indice j
            }
        }
    }

    // Supprimer les transitions dupliquées
    transition* courant = resultat.transitions;
    while (courant != NULL) {
        transition* precedent = courant;
        transition* suivant = courant->suivant;
        while (suivant != NULL) {
            if (courant->source.num_etat == suivant->source.num_etat &&
                courant->dest.num_etat == suivant->dest.num_etat &&
                strcmp(courant->etiquette, suivant->etiquette) == 0) {
                // Supprimer la transition dupliquée
                precedent->suivant = suivant->suivant;
                free(suivant->etiquette);
                free(suivant);
                suivant = precedent->suivant;
            } else {
                precedent = suivant;
                suivant = suivant->suivant;
            }
        }
        courant = courant->suivant;
    }

    // Libération de la mémoire des automates intermédiaires
    free(inverse.Etats);
    free(inverse.alphabet);
    courant = inverse.transitions;
    while (courant != NULL) {
        transition* suivant = courant->suivant;
        free(courant->etiquette);
        free(courant);
        courant = suivant;
    }

    free(determiniseInverse.Etats);
    free(determiniseInverse.alphabet);
    courant = determiniseInverse.transitions;
    while (courant != NULL) {
        transition* suivant = courant->suivant;
        free(courant->etiquette);
        free(courant);
        courant = suivant;
    }

    free(inverseDeterminise.Etats);
    free(inverseDeterminise.alphabet);
    courant = inverseDeterminise.transitions;
    while (courant != NULL) {
        transition* suivant = courant->suivant;
        free(courant->etiquette);
        free(courant);
        courant = suivant;
    }

    return resultat;
}
//------------------------------------------fichier.dots--------------------------------------
void gererfichierdots(Automate automate){
    Automate A1;
    Automate A2;
    memset(&A1, 0, sizeof(Automate));
    memset(&A2, 0, sizeof(Automate));
    A1=determinisation(automate);
    A2=minimisationBrzozowski(A1);
    genererDotDepuisAutomate(&automate,"automate2.dot");
    genererDotDepuisAutomate(&A1, "automate_determiniser.dot");
    genererDotDepuisAutomate(&A1, "automate_minimiser.dot");
    free(A1.Etats);
    free(A1.alphabet);
    free(A2.Etats);
    free(A2.alphabet);

  }
  //------------------------------------------mots_engendre_par_minimale-------------------------------
  void mots_engendre_par_minimale(Automate A1,char fichier[100]){
    Automate A2,A3;
    memset(&A2,0,sizeof(Automate));
    memset(&A3,0,sizeof(Automate));
    A2=determinisation(A1);
    A3=minimisationBrzozowski(A2);
    mots_depuis_fichier_txt(&A3,fichier);
}
//--------------------------Menu-----------------------------

void menu(Automate* automate) {
    int choix;
    char exp[90];
    char fichier[100], etique;
    char mots[100], fichier2[100],fichier7[100];

    do {    // Fonction pour afficher le menu principal
        afficherMessage("                    *********************************************************************************", BLUE);
        afficherMessage("                    *                                                                               *", BLUE);
        afficherMessage("                    *                                                                               *", BLUE);
        afficherMessage("                    *                                                                               *", BLUE);
        afficherMessage("                    *                                   Menu Automate                               *", BLUE);
        afficherMessage("                    *                                                                               *", BLUE);
        afficherMessage("                    *                                                                               *", BLUE);
        afficherMessage("                    *                                                                               *", BLUE);
        afficherMessage("                    *********************************************************************************", BLUE);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                1.Charger un automate depuis un fichier .dot", YELLOW);
        afficherMessage("                    _________________________________________________________________________________", BLUE);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                2.Afficher l'automate", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                3.Generer automate", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                4. Etat avec max de transitions", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                5.Etat avec etiquette", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                6.verifier un mot", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                7.verifier mots depuis fichier text", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                8.L'union de deux automates", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                9.concaténation de 2 automates", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        afficherMessage("                                                10.genere automate depuis expression reguliere", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                11.Supprimer epsilon-transitions", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                 12.Appliquer étoile sur un automate", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                 13.Intersection de deux automates", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                 14.Déterminisation de l'automate", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                 15.Minimisation de l'automate", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                16.fichiersDots", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                17.fichierstxt_minimale", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("                                                18.Quitter", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);
        printf("                                                                  \n");
        afficherMessage("choix : ", BLUE);
        scanf("%d", &choix);
    
        switch (choix) {
            case 1:
                printf("Entrez le nom du fichier .dot : ");
                scanf("%s", fichier);
                {
                    FILE* f = fopen(fichier, "r");
                    if (!f) {
                        printf("Erreur : Impossible d'ouvrir le fichier %s. Verifiez qu'il existe.\n", fichier);
                    } else {
                        fclose(f);
                        *automate = (Automate){0};
                        lireAutomateDepuisDot(automate, fichier);
                        printf("Automate charge depuis %s.\n", fichier);
                    }
                }
                break;
            case 2:
                afficherEtats(automate);
                afficherTransitions(automate);
                afficherListeEtats(automate);
                afficherListeEtiquettes(automate);
                afficherAlphabet(automate);
                break;
            case 3:
                printf("Entrez le nom du fichier .dot a generer : ");
                scanf("%s", fichier);
                genererDotDepuisAutomate(automate, fichier);
                printf("Fichier .dot genere : %s\n", fichier);
                break;
            case 4:
                Etat_avec_max_Trans_sortants(automate);
                break;
            case 5:
                printf("Entrez l'etiquette : ");
                scanf(" %c", &etique);
                afficher_Etat_avec_etique(automate, etique);
                break;
            case 6:
                printf("Entrez le mot a verifier : ");
                scanf("%s", mots);
                verifierMot(automate, mots);
                break;
            case 7:
                printf("Entrez le nom du fichier : ");
                scanf(" %[^\n]", fichier2);
                mots_depuis_fichier_txt(automate, fichier2);
                break;
            case 8: {
                char fichierA[100], fichierB[100], fichierUnion[100];
                Automate automateA, automateB;
            
                // Initialiser les automates
                memset(&automateA, 0, sizeof(Automate));
                memset(&automateB, 0, sizeof(Automate));
            
                // Demander à l'utilisateur de saisir les noms des fichiers
                printf("Entrez le nom du fichier .dot pour l'automate A : ");
                scanf("%s", fichierA);
                printf("Entrez le nom du fichier .dot pour l'automate B : ");
                scanf("%s", fichierB);
                printf("Entrez le nom du fichier .dot pour l'union : ");
                scanf("%s", fichierUnion);
            
                // Charger automateA et automateB depuis les fichiers .dot
                lireAutomateDepuisDot(&automateA, fichierA);
                lireAutomateDepuisDot(&automateB, fichierB);
            
                // Créer l'union des deux automates et générer le fichier .dot
                Automate automateC = unionAutomates(&automateA, &automateB, fichierUnion);
            
                // Afficher l'automate résultant
                printf("\nAutomate résultant de l'union :\n");
                afficherEtats(&automateC);
                afficherTransitions(&automateC);
                afficherAlphabet(&automateC);
            
                // Libérer la mémoire des automates temporaires
                free(automateA.Etats);
                free(automateB.Etats);
                free(automateA.alphabet);
                free(automateB.alphabet);
                break;
            }
            case 9: {
                printf("Entrez le nom du fichier du premier automate à concaténer : ");
                char fichier3[100];
                scanf("%99s", fichier3);
            
                printf("Entrez le nom du fichier du deuxième automate à concaténer : ");
                char fichier4[100];
                scanf("%99s", fichier4);
                
                printf("Entrez le nom du fichier de concaténation : ");
                char fichier5[100];
                scanf("%99s", fichier5);
            
                Automate automate1 = {0};
                Automate automate2 = {0};
            
                lireAutomateDepuisDot(&automate1, fichier3);
                lireAutomateDepuisDot(&automate2, fichier4);
            
                Automate automate3 = concatenation_deux_automates(&automate1, &automate2);
                genererDotDepuisAutomate(&automate3, fichier5);
                
                printf("Concaténation générée avec succès dans %s\n", fichier5);
                
                // Libérer la mémoire
                free(automate1.Etats);
                free(automate2.Etats);
                free(automate1.alphabet);
                free(automate2.alphabet);
                break;
            }
            case 10: {
                printf("Entrez l'expression reguliere : ");
                scanf("%s", exp);
                char fichierExp[100];
                printf("Entrez le nom du fichier .dot pour sauvegarder l'automate : ");
                scanf("%s", fichierExp);
                Automate* automateGenere = construireAutomateFromRegex(exp);
                if (automateGenere) {
                    genererDotDepuisAutomate(automateGenere, fichierExp);
                    printf("Automate généré à partir de l'expression régulière dans %s\n", fichierExp);
                    afficherEtats(automateGenere);
                    afficherAlphabet(automateGenere);
                    free(automateGenere->Etats);
                    free(automateGenere->alphabet);
                    free(automateGenere);
                } else {
                    printf("Erreur lors de la génération de l'automate.\n");
                }
                break;
            }
            case 11: {
                if (automate->nbTransitions == 0) {
                    printf("L'automate ne contient aucune transition.\n");
                    break;
                }

                char nomFichier[100];
                printf("Entrez le nom du fichier .dot pour sauvegarder le résultat : ");
                scanf("%s", nomFichier);
                
                // Supprimer les epsilon-transitions
                supprimerEpsilonTransitions(automate);
                
                // Sauvegarder le résultat
                genererDotDepuisAutomate(automate, nomFichier);
                
                printf("Automate sans epsilon-transitions généré dans %s\n", nomFichier);
                printf("\nAffichage du nouvel automate :\n");
                afficherTransitions(automate);
                afficherEtats(automate);
                break;
            }
            case 12: {
                Automate automateEtoile;
                etoile_automate(automate, &automateEtoile);
                printf("Automate étoile généré.\n");
                afficherEtats(&automateEtoile);
                afficherTransitions(&automateEtoile);
                break;
            }
            case 13: {
                char fichierA[100], fichierB[100], fichierIntersection[100];
                Automate automateA, automateB, automateC;
                
                // Initialisation
                memset(&automateA, 0, sizeof(Automate));
                memset(&automateB, 0, sizeof(Automate));
                
                printf("Entrez le nom du fichier .dot pour l'automate A : ");
                scanf("%s", fichierA);
                printf("Entrez le nom du fichier .dot pour l'automate B : ");
                scanf("%s", fichierB);
                printf("Entrez le nom du fichier .dot pour l'intersection : ");
                scanf("%s", fichierIntersection);
                
                // Chargement des automates
                lireAutomateDepuisDot(&automateA, fichierA);
                lireAutomateDepuisDot(&automateB, fichierB);
                
                // Calcul de l'intersection
                automateC = intersectionAutomates(&automateA, &automateB, fichierIntersection);
                
                // Affichage du résultat
                printf("\nAutomate resultant de l'intersection est genere avec succes !\n");
                
                // Libération de la mémoire
                free(automateA.Etats);
                free(automateB.Etats);
                free(automateA.alphabet);
                free(automateB.alphabet);
                free(automateC.Etats);
                
                break;
            }
            case 14: {
                printf("Entrez le nom du fichier .dot pour sauvegarder l'automate déterminisé : ");
                scanf("%s", fichier);
                Automate automateDeterminise = determinisation(*automate);

                // Vérifier si l'automate déterminisé est valide
                if (automateDeterminise.nbEtats == 0) {
                    printf("Erreur : L'automate déterminisé est vide ou invalide.\n");
                    break;
                }

                printf("Automate déterminisé avec succès.\n");
                genererDotDepuisAutomate(&automateDeterminise, fichier);

                // Vérifier si le fichier a été généré
                FILE* testFile = fopen(fichier, "r");
                if (testFile) {
                    fclose(testFile);
                    printf("Automate déterminisé généré dans %s\n", fichier);
                    afficherEtats(&automateDeterminise);
                    afficherTransitions(&automateDeterminise);
                    afficherAlphabet(&automateDeterminise);
                } else {
                    printf("Erreur : Impossible de générer le fichier %s.\n", fichier);
                }

                // Libérer la mémoire de l'automate déterminisé
                free(automateDeterminise.Etats);
                free(automateDeterminise.alphabet);
                break;
            }
            case 15: {
                printf("Entrez le nom du fichier .dot pour sauvegarder l'automate minimisé : ");
                scanf("%s", fichier);
                Automate automateMinimise = minimisationBrzozowski(*automate);

                // Vérifier si l'automate minimisé est valide
                if (automateMinimise.nbEtats == 0) {
                    printf("Erreur : L'automate minimisé est vide ou invalide.\n");
                    break;
                }

                printf("Automate minimisé avec succès.\n");
                genererDotDepuisAutomate(&automateMinimise, fichier);

                // Vérifier si le fichier a été généré
                FILE* testFile = fopen(fichier, "r");
                if (testFile) {
                    fclose(testFile);
                    printf("Automate minimisé généré dans %s\n", fichier);
                    afficherEtats(&automateMinimise);
                    afficherTransitions(&automateMinimise);
                    afficherAlphabet(&automateMinimise);
                } else {
                    printf("Erreur : Impossible de générer le fichier %s.\n",fichier);
                }

                // Libérer la mémoire de l'automate minimisé
                free(automateMinimise.Etats);
                free(automateMinimise.alphabet);
                break;
            }
            case 16:{
                //printf("entre le nom de fichier de automate initiale");
                //scanf("%s", fichier7);
                //Automate automate1;
               // memset(&automate1,0,sizeof(Automate));
                //lireAutomateDepuisDot(&automate1,fichier7);
                gererfichierdots(*automate);
                break;
            }
            case 17:{
                printf("entrer le nom de fichier.txt");
                scanf("%s",fichier7);
                mots_engendre_par_minimale(*automate,fichier7);
            }
            case 18:
                printf("Au revoir !\n");
                break;
            default:
                printf("Choix invalide.\n");
        }
    } while (choix != 18); // Assurez-vous que le programme reste dans la boucle tant que l'utilisateur ne choisit pas de quitter.
}

int main() {
    Automate *automate = malloc(sizeof(Automate));
    if (!automate) {
        printf("Erreur d'allocation mémoire pour l'automate\n");
        return 1;
    }
    
    // Initialisation des champs
    automate->Etats = NULL;
    automate->transitions = NULL;
    automate->alphabet = NULL;
    automate->nbEtats = 0;
    automate->nbTransitions = 0;
    automate->nbAlphabets = 0;
    
    menu(automate);
    
    // Libération de la mémoire
    free(automate->Etats);
    free(automate->alphabet);
    
    // Libérer les transitions
    transition *courant = automate->transitions;
    while (courant != NULL) {
        transition *suivant = courant->suivant;
        free(courant->etiquette);
        free(courant);
        courant = suivant;
    }
    
    free(automate);
    return 0;
}