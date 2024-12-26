include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WIDTH 1000
#define LENGTH 850
#define FPS 60
#define COLUMNS 10
#define SEGMENTS 334

// RGB structure for possible color management in the display (if needed for visualization)
typedef struct {
    int r;
    int b;
    int g;
} RGB;

// Product structure containing product details
typedef struct produit {
    int ID;
    char nom[31];
    float prix;
} produit;

// Block structure representing a block in memory (data + metadata)
typedef struct bloc {
    int num;
    int bytes;
    char produit[SEGMENTS];  // For product data
    struct bloc *svt;        // Pointer to the next block (for chained files)
    char metadata[SEGMENTS]; // For metadata (file name, size, etc.)
    struct bloc *metaNext;   // Pointer to the next metadata block (if needed)
} bloc;

// Index structure used for indexing files in memory
typedef struct index {
    char id[10];
    bloc *b;
    int num;
    struct index *suivant;
} index;

// Info structure to store information about the files
typedef struct {
    bloc *prems;
    int nbBloc;
    int nbProduit;
    int ajout;
    int supprime;
} info;

// Function to create a new product
produit* createProduct(char id[9]) {
    produit *p = (produit*)malloc(sizeof(produit));
    p->ID = atoi(id);
    snprintf(p->nom, sizeof(p->nom), "Product %s", id);
    p->prix = (rand() % 10000) / 100.0; // Random price between 0.00 and 100.00
    return p;
}

// Function to create a new block
void allocation(info *inf) {
    bloc *b = (bloc*)malloc(sizeof(bloc));
    inf->nbBloc++;
    b->num = inf->nbBloc;
    b->bytes = 0;
    b->svt = NULL;
    b->metaNext = NULL;
    
    if (inf->prems == NULL) {
        inf->prems = b;
    } else {
        bloc *t = inf->prems;
        while (t->svt != NULL) {
            t = t->svt;
        }
        t->svt = b;
    }
}

// Function to add product data to a block
void addProductToBlock(info *inf, produit *p) {
    bloc *b = inf->prems;
    while (b->svt != NULL) {
        b = b->svt;
    }
    
    int product_size = snprintf(NULL, 0, "%d,%s,%f", p->ID, p->nom, p->prix);
    
    // Check if the block is full
    if (b->bytes + product_size > SEGMENTS) {
        bloc *new_block = (bloc*)malloc(sizeof(bloc));
        new_block->num = inf->nbBloc++;
        new_block->bytes = 0;
        new_block->svt = NULL;
        b->svt = new_block;
        b = new_block;
    }

    // Add the product data to the block
    snprintf(b->produit + b->bytes, SEGMENTS - b->bytes, "%d,%s,%f", p->ID, p->nom, p->prix);
    b->bytes += product_size;
}

// Function to create file metadata
void createFile(info *inf, const char *filename, int num_records) {
    bloc *new_block = (bloc*)malloc(sizeof(bloc));
    new_block->num = inf->nbBloc++;
    new_block->bytes = 0;
    new_block->svt = NULL;
    
    // Write metadata
    snprintf(new_block->metadata, SEGMENTS, "File Name: %s, Num Records: %d", filename, num_records);
    inf->prems = new_block;

    // Add product data to blocks
    for (int i = 0; i < num_records; i++) {
        char id[9];
        snprintf(id, sizeof(id), "%08d", i + 1);
        produit *p = createProduct(id);
        addProductToBlock(inf, p);
        free(p);
    }
}

// Function to print block information (for debugging and visualization)
void printBlockInfo(bloc *b) {
    while (b != NULL) {
        printf("Block %d: %d bytes\n", b->num, b->bytes);
        printf("Data: %s\n", b->produit);
        b = b->svt;
    }
}

// Function to search for a product in the blocks by ID
bloc* searchProductByID(index *index_table, const char *product_id) {
    for (int i = 0; i < 10; i++) { // Adjust based on the actual size of the index table
        if (strcmp(index_table[i].id, product_id) == 0) {
            return index_table[i].b;
        }
    }
    printf("Product not found!\n");
    return NULL;
}

// Function to delete a product (logical deletion)
void logicalDeleteProduct(produit *p) {
    p->ID = -1; // Mark as deleted
    printf("Product with ID %d has been logically deleted.\n", p->ID);
}

// Function to display file metadata
void displayMetadata(bloc *b) {
    printf("Metadata: %s\n", b->metadata);
}

// Function to free memory for index
void freeIndex(index *a) {
    free(a);
}

// Main program to demonstrate file creation, product insertion, etc.
int main() {
    srand(time(NULL));

    // Initialize info structure to manage the file and blocks
    info inf = {NULL, 0, 0, 0, 0};
    
    // Create a file and add products
    createFile(&inf, "ProductsFile", 5);
    
    // Print block information (to show data and metadata)
    printBlockInfo(inf.prems);
    
    // Create and add a product
    produit *newProduct = createProduct("00006");
    addProductToBlock(&inf, newProduct);
    free(newProduct);
    
    // Search for a product by ID
    char search_id[10] = "00006";
    bloc *found_block = searchProductByID(NULL, search_id); // Assuming index_table is available
    if (found_block != NULL) {
        printBlockInfo(found_block);
    }
    
    // Logical deletion of a product
    logicalDeleteProduct(newProduct);

    // Free allocated memory (clean up)
    freeIndex(NULL); // Assuming index table is initialized elsewhere

    return 0;
}

