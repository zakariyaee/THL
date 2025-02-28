#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define GREEN 32
#define RED 31
#define YELLOW 33
#define BLUE 34

typedef struct etat {
    int num_etat;
    bool isFinal;
    bool isInitial;
    int count; // Nombre de transitions sortantes
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
    char **alphabet; // Tableau dynamique d'alphabets (etiquettes)
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

    // Ajout des etats source et destination, s'ils n'existent pas deja
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

void genere_Fichier_dot(Automate* automate, char nom[50]) {
    int nbreEta, Eta, nbreInitiale, nbreFinale;
    FILE *file = fopen(nom, "w");
    if (!file) {
        perror("Erreur d'ouverture");
        return;
    }
    fprintf(file, "digraph Automate {\n    rankdir=LR;\n  ");
    
    // Saisie des etats
    printf("Saisir le nombre des etats : ");
    scanf("%d", &nbreEta);
    for (int i = 0; i < nbreEta; i++) {
        printf("Entrer l'etat num %d de cet automate : ", i + 1);
        scanf("%d", &Eta);
        fprintf(file, "node [shape = circle]; %d;\n", Eta);
        ajouterEtat(automate, Eta, false, false);
    }
    
    // Saisie des etats initiaux
    fprintf(file, "node [shape = point, width = 0] ");
    printf("Entrer le nombre des etats initiaux : ");
    scanf("%d", &nbreInitiale);
    for (int i = 0; i < nbreInitiale; i++) {
        fprintf(file, "start%d ", i + 1);
    }
    fprintf(file, ";\n");
    for (int i = 0; i < nbreInitiale; i++) {
        printf("Saisir l'etat initial num %d : ", i + 1);
        scanf("%d", &Eta);
        fprintf(file, "start%d -> %d;\n", i + 1, Eta);
        ajouterEtat(automate, Eta, true, false);
    }
    
    // Saisie des etats finaux
    printf("Saisir le nombre des etats finaux : ");
    scanf("%d", &nbreFinale);
    fprintf(file, "node [shape = point, width = 0] ");
    for (int i = 0; i < nbreFinale; i++) {
        fprintf(file, "final%d ", i + 1);
    }
    fprintf(file, ";\n");
    for (int i = 0; i < nbreFinale; i++) {
        printf("Saisir l'etat final num %d : ", i + 1);
        scanf("%d", &Eta);
        fprintf(file, "%d -> final%d;\n", Eta, i + 1);
        ajouterEtat(automate, Eta, false, true);
    }
    
    // Saisie des transitions sans demander le nombre total a l'avance.
    // L'utilisateur entre -1 comme etat source pour terminer la saisie.
    char *etiquettes = malloc(128 * sizeof(char));
    if (!etiquettes) {
        perror("Erreur d'allocation pour etiquettes");
        fclose(file);
        return;
    }
    int init, final;
    printf("Entrez les transitions (entrez -1 pour l'etat source pour terminer) :\n");
    while (1) {
        printf("Entrer l'etat source (ou -1 pour terminer) : ");
        scanf("%d", &init);
        if (init == -1) break;
        printf("Entrer l'etat destination : ");
        scanf("%d", &final);
        printf("Entrer les etiquettes de la transition (separees par des virgules si multiple) : ");
        scanf("%127s", etiquettes);
        fprintf(file, "%d -> %d [label=\"%s\"];\n", init, final, etiquettes);
        etat source = {init, false, false, 0};
        etat dest = {final, false, false, 0};
        ajouterTransition(automate, source, dest, etiquettes); 
    }
    fprintf(file, "}\n");
    free(etiquettes);
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

void menu(Automate* automate) {
    int choix;
    char fichier[100], etique;
    char mots[100], fichier2[100];

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
        afficherMessage("                                                8.Quitter", YELLOW);
        afficherMessage("                    __________________________________________________________________________________", BLUE);

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
                printf("Entrez le nom du fichier : ");
                scanf(" %[^\n]", fichier);
                genere_Fichier_dot(automate, fichier);
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
            case 8:
                printf("Au revoir !\n");
                break;
            default:
                printf("Choix invalide.\n");
        }
    } while (choix != 8);
}

int main() {
    Automate automate;
    memset(&automate, 0, sizeof(Automate));  // Initialise a zero
    menu(&automate);
    return 0;
}
