#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_BLOCKS 100
#define BLOCK_SIZE 256
typedef struct {
    int is_free;  // 1 si le bloc est libre, 0 sinon
    char data[BLOCK_SIZE];  // Contenu du bloc
    int next_block;  // Pour l'organisation chaînée
} Block;
Block memory[MAX_BLOCKS];  // Tableau qui represte mémoire secondaire
void initialize_memory() {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        memory[i].is_free = 1;  // Marquer tous les blocs comme libres
        memory[i].next_block = -1;  // Pas de lien entre les blocs au départ
    }
    printf("Mémoire secondaire initialisée avec %d blocs.\n", MAX_BLOCKS);
}
//structure du fichier
typedef struct {
    char name[50];  // Nom du fichier
    int start_block;  // Premier bloc alloué
    int nbr_bloc;  // Nombre de blocs alloués
    int nbr_enregistrements;  // Nombre d'enregistrements
    char global_mode[10];  // Mode global (contiguë/chaînée)
    char internal_mode[10];  // Mode interne (trié/non trié)
} File;
File files[MAX_BLOCKS];  // Liste des fichiers
int nbr_fichier = 0;  // Nombre de fichiers créés
void create_file(char *name, int nbr_enregistrements, char *global_mode, char *internal_mode) {
    if (nbr_fichier >= MAX_BLOCKS) {
        printf("Erreur : Nombre maximal de fichiers atteint.\n");
        return;
    }
    // Vérifier si un fichier avec le meme nom existe déjà
    for (int i = 0; i < nbr_fichier; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("Erreur : Un fichier avec ce nom existe déjà.\n");
            return;
        }
    }
    // Allouer les blocs nécessaires	//calcul du nbr de bloc necessaire
    int blocks_needed = (nbr_enregistrements * sizeof(int)) / BLOCK_SIZE + 1;
    int start_block = -1;
    int prev_block = -1;
    for (int i = 0, allocated = 0; i < MAX_BLOCKS && allocated < blocks_needed; i++) {
        if (memory[i].is_free) {
            memory[i].is_free = 0;
            if (start_block == -1) start_block = i;
            if (prev_block != -1) memory[prev_block].next_block = i;
            prev_block = i;
            allocated++;
        }
    }
    if (start_block == -1) {
        printf("Erreur : Pas assez de blocs libres.\n");
        return;
    }
    // Initialiser le fichier
    strcpy(files[nbr_fichier].name, name);
    files[nbr_fichier].start_block = start_block;
    files[nbr_fichier].nbr_bloc = blocks_needed;
    files[nbr_fichier].nbr_enregistrements = nbr_enregistrements;
    strcpy(files[nbr_fichier].global_mode, global_mode);
    strcpy(files[nbr_fichier].internal_mode, internal_mode);
    nbr_fichier++;
    printf("Fichier '%s' créé avec %d blocs.\n", name, blocks_needed);
}
//fonction qui insert un enregistrement
void insert_record(char *file_name, int record_id) {
    for (int i = 0; i < nbr_fichier; i++) {
        if (strcmp(files[i].name, file_name) == 0) {
            int block_index = files[i].start_block;
            while (block_index != -1) {
                Block *block = &memory[block_index];
                if (strlen(block->data) + sizeof(int) <= BLOCK_SIZE) {
                    // Ajouter l'ID de l'enregistrement
                    sprintf(block->data + strlen(block->data), "%d ", record_id);
                    files[i].nbr_enregistrements++;
                    printf("Enregistrement %d inséré dans le fichier '%s'.\n", record_id, file_name);
                    return;
                }
                block_index = block->next_block;
            }
            printf("Erreur : Pas assez d'espace pour insérer l'enregistrement.\n");
            return;
        }
    }
    
//fonction qui recherche un enregistrement par son ID
void search_record(char *file_name, int record_id) {
    for (int i = 0; i < nbr_fichier; i++) {
        if (strcmp(files[i].name, file_name) == 0) {
            int block_index = files[i].start_block;
            while (block_index != -1) {
                Block *block = &memory[block_index];
                char *token = strtok(block->data, " ");
                while (token != NULL) {
                    if (atoi(token) == record_id) {
                        printf("Enregistrement %d trouvé dans le fichier '%s'.\n", record_id, file_name);
                        return;
                    }
                    token = strtok(NULL, " ");
                }
                block_index = block->next_block;
            }
            printf("Enregistrement %d introuvable dans le fichier '%s'.\n", record_id, file_name);
            return;
        }
    }
    
}
//fonction qui supprime un enregistrement logiquement ou physiquement et logiquement
void delete_record(char *file_name, int record_id) {
    int choice;
    printf("Choisissez le type de suppression pour l'enregistrement %d dans le fichier '%s':\n", record_id, file_name);
    printf("1. Suppression logique uniquement\n");
    printf("2. Suppression logique suivie de suppression physique\n");
    printf("Votre choix (1/2) : ");
    scanf("%d", &choice);
    if (choice == 1) {
        // Suppression logique uniquement
        if (delete_record_logical(file_name, record_id)) {
            printf("Suppression logique de l'enregistrement %d réussie.\n", record_id);
        } else {
            printf("Erreur : L'enregistrement %d n'a pas été trouvé dans le fichier '%s'.\n", record_id, file_name);
        }
    } else if (choice == 2) {
        // Suppression logique suivie de suppression physique
        if (delete_record_logical(file_name, record_id)) {
            printf("Suppression logique de l'enregistrement %d réussie.\n", record_id);
            delete_record_physical(file_name, record_id);
        } else {
            printf("Erreur : L'enregistrement %d n'a pas été trouvé dans le fichier '%s'.\n", record_id);
        }
    } else {
        printf("Choix invalide. Veuillez entrer 1 ou 2.\n");  
}
}
//supprimer logiquement un record
int delete_record_logical(char *file_name, int record_id) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, file_name) == 0) {
            int block_index = files[i].start_block;
            while (block_index != -1) {
                Block *block = &memory[block_index];
                char *token = strtok(block->data, " ");
                char new_data[BLOCK_SIZE] = "";
                int found = 0;
                while (token != NULL) {
                    if (atoi(token) == record_id) {
                        found = 1;  // Enregistrement trouvé et marqué comme supprimé
                    } else {
                        strcat(new_data, token);
                        strcat(new_data, " ");
                    }
                    token = strtok(NULL, " ");
                }
                strcpy(block->data, new_data);
                if (found) {
                    files[i].record_count--;  // Réduire le nombre d'enregistrements
                    return 1;  // Succès
                }
                block_index = block->next_block;
            }
        }
    }
    return 0;  // Enregistrement non trouvé
}
//supprimer physiquement un record 
void delete_record_physical(char *file_name, int record_id) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, file_name) == 0) {
            int block_index = files[i].start_block;
            int prev_block = -1;
            char temp_data[MAX_BLOCKS * BLOCK_SIZE] = "";  // Stockage temporaire des données compactées
            int found = 0;
            // Parcourir les blocs du fichier
            while (block_index != -1) {
                Block *block = &memory[block_index];
                char *token = strtok(block->data, " ");
                while (token != NULL) {
                    if (atoi(token) != record_id) {
                        strcat(temp_data, token);
                        strcat(temp_data, " ");
                    } else {
                        found = 1;  // Enregistrement trouvé et supprimé
                    }
                    token = strtok(NULL, " ");
                }
                // Libérer le bloc actuel
                block->is_free = 1;
                block->next_block = -1;
                memset(block->data, 0, BLOCK_SIZE);
                // Passer au bloc suivant
                prev_block = block_index;
                block_index = memory[prev_block].next_block;
            }
            if (!found) {
                printf("Enregistrement %d introuvable dans le fichier '%s'.\n", record_id, file_name);
                return;
            }
            // Réorganiser les blocs avec les données compactées
            block_index = files[i].start_block;
            int allocated = 0;
            char *data_ptr = temp_data;
            while (allocated < files[i].block_count && *data_ptr != '\0') {
                Block *block = &memory[block_index];
                strncpy(block->data, data_ptr, BLOCK_SIZE - 1);
                block->data[BLOCK_SIZE - 1] = '\0';  // Sécurité pour éviter les dépassements
                data_ptr += strlen(block->data);
                allocated++;
                if (allocated < files[i].block_count && *data_ptr != '\0') {
                    int next_free = -1;
                    for (int j = 0; j < MAX_BLOCKS; j++) {
                        if (memory[j].is_free) {
                            next_free = j;
                            break;
                        }
                    }
                    if (next_free == -1) {
                        printf("Erreur : Pas assez de blocs libres pour réorganiser les données.\n");
                        return;
                    }
                    memory[block_index].next_block = next_free;
                    block_index = next_free;
                } else {
                    memory[block_index].next_block = -1;
                }
            }
            // Mettre à jour les métadonnées
            files[i].record_count--;  // Réduire le nombre d'enregistrements
            files[i].block_count = allocated;
            printf("Enregistrement %d supprimé physiquement dans le fichier '%s'.\n", record_id, file_name);
            return;
        }
    }
    printf("Erreur : Fichier '%s' introuvable.\n", file_name);
}
void main_menu() {
    int choice;
    do {
        printf("\nMenu Principal\n");
        printf("1. Initialiser la mémoire secondaire\n");
        printf("2. Créer un fichier\n");
        printf("3. Quitter\n");
        printf("Votre choix : ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                initialize_memory(100, 64); // Exemple : 100 blocs de 64 octets
                break;
            case 2:
                void create_file(char *name, int nbr_enregistrements, char *global_mode, char *internal_mode);
                break;
            case 3:
                printf("Au revoir !\n");
                break;
            default:
                printf("Choix invalide. Veuillez réessayer.\n");
        }
    } while (choice != 3);
}
