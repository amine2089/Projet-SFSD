#include<stdio.h>
#include<stdlib.h>
#define BLOCK_SIZE 512 // Taille d'un bloc en octets (modifiable)
#define MAX_BLOCKS 1000 // Nombre maximum de blocs (modifiable)

// Structure pour représenter un bloc
typedef struct {
    int block_id;      // Identifiant unique du bloc
    int is_free;       // 1 = libre, 0 = occupé
    char data[BLOCK_SIZE]; // Données contenues dans le bloc
} Block;

// Table d'allocation des blocs
Block memory[MAX_BLOCKS];
typedef enum {
    CONTIGUOUS,
    LINKED
} FileMode;

typedef struct {
    char name[50];      // Nom du fichier
    int size;           // Taille en nombre d'enregistrements
    FileMode mode;      // Mode d'organisation
    int start_block;    // Bloc de départ (contigu) ou premier bloc (chaîné)
} File;

// Liste des fichiers
#define MAX_FILES 100
File file_table[MAX_FILES];
int file_count = 0; // Compteur de fichiers
#define RECORD_SIZE 128 // Taille d'un enregistrement en octets

typedef struct {
    int record_id;      // Identifiant unique de l'enregistrement
    char data[RECORD_SIZE - sizeof(int)]; // Données associées
} Record;
void initialize_memory() {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        memory[i].block_id = i;
        memory[i].is_free = 1; // Tous les blocs sont libres au départ
    }
}
int create_file(char *name, int size, FileMode mode) {
    if (file_count >= MAX_FILES) {
        printf("Erreur : Trop de fichiers.\n");
        return -1;
    }

    File new_file;
    strcpy(new_file.name, const char *name);
    new_file.size = size;
    new_file.mode = mode;

    // Allocation des blocs
    int allocated_blocks = 0;
    for (int i = 0; i < MAX_BLOCKS && allocated_blocks < size; i++) {
        if (memory[i].is_free) {
            memory[i].is_free = 0; // Bloque le bloc
            if (allocated_blocks == 0) {
                new_file.start_block = i; // Premier bloc
            }
            allocated_blocks++;
        }
    }

    if (allocated_blocks < size) {
        printf("Erreur : Mémoire insuffisante.\n");
        return -1;
    }

    file_table[file_count++] = new_file;
    return 0;
}
Record* search_record(File *file, int record_id) {
    int block_index = file->start_block;
    while (block_index != -1) {
        Block *current_block = &memory[block_index];
        for (int i = 0; i < BLOCK_SIZE / RECORD_SIZE; i++) {
            Record *rec = (Record *)(current_block->data + i * RECORD_SIZE);
            if (rec->record_id == record_id) {
                return rec; // Enregistrement trouvé
            }
        }

        if (file->mode == CONTIGUOUS) {
            block_index++;
        } else {
            block_index = *((int *)(current_block->data + BLOCK_SIZE - sizeof(int))); // Pointeur au dernier octet
        }
    }
    return NULL; // Non trouvé
}
