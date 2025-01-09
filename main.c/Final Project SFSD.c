#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Struct definitions
typedef struct {
    char file_name[60];
    int firstBlock;
    int nbBloc;
    int nbProduit;
} meta;

typedef struct {
    int id;
    char nom[31];
    float prix;
} produit;

typedef struct {
    bool is_free;
    int file_id;
    char data[512]; // Buffer for storing product or metadata
    int next_block;
} Block;

// Global variables
Block *memory;
int total_blocks;
int block_size;
meta metadataTable[100];
int metadataCount = 0;
int facteur_de_blocage = 14; // Blocking factor (number of products per buffer)

// Function prototypes
void initializeMemory(int num_blocks, int b_size);
void displayMemoryState();
void updateTableIndex();
void writeTableIndexToDisk(const char *diskFile);
void writeMetadataToDisk(const char *diskFile);
void updateMemoryState();
int allocateBlocks(int file_id, int num_blocks, int allocationType);
int allocateChainedBlocks(int file_id, int num_blocks);
int allocateContiguousBlocks(int file_id, int num_blocks);
void addMetadata(meta newMeta);
void writeProductsToDisk(const char *inputFile, const char *diskFile);
void deleteFileFromDisk(const char *filename, const char *diskFile);
void deleteProductFromFile(const char *filename, int product_id);
void synchronizeDiskWithFile(const char *filename, const char *diskFile, int numAddedProducts);
void displayDiskContents(const char *diskFile);
void fillProductFile(const char *filename);
void searchProductInFile(const char *filename, int product_id);
void searchProductInMemory(int product_id);
void searchProductInDisk(const char *diskFile, int product_id);
void writeProductsToMemory(const char *filename);
void updateMetadata(const char *filename, int numAddedProducts);
void deleteBlockFromMemory(int blockIndex);
void deleteProductFromFile(const char *filename, int product_id);
void loadMemoryFromFile(const char *memoryFile);
void saveMemoryToFile(const char *memoryFile);
void updateChainedBlocksBuffered(int startBlock, const char *newData, int dataSize);
void deleteContiguousBlocks(int startBlock, int numBlocks);
int allocateContiguousBlocksBuffered(int file_id, int num_blocks);
int allocateChainedBlocksBuffered(int file_id, int num_blocks);
void deleteProductsFromDisk(const char *filename, const char *diskFile);

// Suppression de blocs contigus
void deleteContiguousBlocks(int startBlock, int numBlocks) {
    for (int i = startBlock; i < startBlock + numBlocks; i++) {
        memory[i].is_free = true;
        memory[i].file_id = 0;
        memory[i].next_block = -1;
        memset(memory[i].data, 0, sizeof(memory[i].data));
    }
    printf("%d contiguous blocks deleted starting from block %d.\n", numBlocks, startBlock);
}

// Suppression de blocs chaînés
void deleteChainedBlocks(int startBlock) {
    int currentBlock = startBlock;
    while (currentBlock != -1) {
        int nextBlock = memory[currentBlock].next_block;
        memory[currentBlock].is_free = true;
        memory[currentBlock].file_id = 0;
        memory[currentBlock].next_block = -1;
        memset(memory[currentBlock].data, 0, sizeof(memory[currentBlock].data));
        currentBlock = nextBlock;
    }
    printf("Chained blocks starting from block %d deleted.\n", startBlock);
}

// Allocation de blocs contigus
int allocateContiguousBlocksBuffered(int file_id, int num_blocks) {
    int start = -1, count = 0;
    for (int i = 1; i < total_blocks; i++) {
        if (memory[i].is_free) {
            if (count == 0) start = i;
            count++;
            if (count == num_blocks) break;
        } else {
            count = 0; // Reset if a block is not free
        }
    }

    if (count < num_blocks) {
        printf("Failed to allocate %d contiguous blocks.\n", num_blocks);
        return -1;
    }

    char buffer[512];
    memset(buffer, 0, sizeof(buffer));

    for (int i = start; i < start + num_blocks; i++) {
        memory[i].is_free = false;
        memory[i].file_id = file_id;
        snprintf(buffer, sizeof(buffer), "Block %d allocated.\n", i);
        strcat(memory[i].data, buffer);
    }

    printf("Allocated %d contiguous blocks starting at block %d for file ID %d.\n", num_blocks, start, file_id);
    return start;
}

// Allocation de blocs chaînés
int allocateChainedBlocksBuffered(int file_id, int num_blocks) {
    int start = -1, count = 0;
    int prevBlock = -1;

    for (int i = 1; i < total_blocks; i++) {
        if (memory[i].is_free) {
            if (start == -1) start = i;

            memory[i].is_free = false;
            memory[i].file_id = file_id;
            if (prevBlock != -1) {
                memory[prevBlock].next_block = i;
            }
            prevBlock = i;
            count++;

            if (count == num_blocks) {
                memory[i].next_block = -1; // End the chain
                break;
            }
        }
    }

    if (count < num_blocks) {
        printf("Failed to allocate %d chained blocks.\n", num_blocks);
        return -1;
    }

    printf("Allocated %d chained blocks starting at block %d for file ID %d.\n", num_blocks, start, file_id);
    return start;
}

// Mise à jour des blocs contigus
void updateContiguousBlocksBuffered(int startBlock, const char *newData, int dataSize) {
    char buffer[512];
    int remainingData = dataSize;
    int dataOffset = 0;

    for (int i = startBlock; i < total_blocks && remainingData > 0; i++) {
        memset(buffer, 0, sizeof(buffer));
        int copySize = remainingData < sizeof(buffer) ? remainingData : sizeof(buffer);
        strncpy(buffer, newData + dataOffset, copySize);
        strncpy(memory[i].data, buffer, copySize);

        remainingData -= copySize;
        dataOffset += copySize;
    }

    printf("Contiguous blocks updated starting from block %d.\n", startBlock);
}

// Mise à jour des blocs chaînés
void updateChainedBlocksBuffered(int startBlock, const char *newData, int dataSize) {
    int currentBlock = startBlock;
    int remainingData = dataSize;
    int dataOffset = 0;

    while (currentBlock != -1 && remainingData > 0) {
        int copySize = remainingData < sizeof(memory[currentBlock].data) ? remainingData : sizeof(memory[currentBlock].data);
        strncpy(memory[currentBlock].data, newData + dataOffset, copySize);

        remainingData -= copySize;
        dataOffset += copySize;
        currentBlock = memory[currentBlock].next_block;
    }

    printf("Chained blocks updated starting from block %d.\n", startBlock);
}


void deleteProductFromFile(const char *filename, int product_id) {
    FILE *file = fopen(filename, "r");
    FILE *temp = fopen("temp.txt", "w");

    if (!file || !temp) {
        printf("Error opening files for deletion.\n");
        return;
    }

    char line[128];
    bool productDeleted = false;
    while (fgets(line, sizeof(line), file)) {
        produit p;
        sscanf(line, "%d,%30[^,],%f", &p.id, p.nom, &p.prix);
        if (p.id == product_id) {
            productDeleted = true;
        } else {
            fprintf(temp, "%s", line);
        }
    }

    fclose(file);
    fclose(temp);
    remove(filename);
    rename("temp.txt", filename);

    if (productDeleted) {
        // Update Metadata
        for (int i = 0; i < metadataCount; i++) {
            if (strcmp(metadataTable[i].file_name, filename) == 0) {
                metadataTable[i].nbProduit--;
                if (metadataTable[i].nbProduit < 0) {
                    metadataTable[i].nbProduit = 0;
                }
                break;
            }
        }

        // Synchronize memory and disk
        updateTableIndex();
        writeTableIndexToDisk("disk.txt");
        writeProductsToDisk(filename, "disk.txt");
        saveMemoryToFile("central_memory.txt");

        // Display updated state
        printf("Product with ID %d deleted from file %s.\n", product_id, filename);
        displayMemoryState();
        displayDiskContents("disk.txt");
    } else {
        printf("Product with ID %d not found in file %s.\n", product_id, filename);
    }
}



void synchronizeDiskWithFile(const char *filename, const char *diskFile, int numNewProducts) {
    updateMetadata(filename, numNewProducts); // Increment metadata only for new products
    writeProductsToDisk(filename, diskFile);

    printf("Disk synchronized with file %s\n", filename);
}


void saveMemoryToFile(const char *memoryFile) {
    FILE *file = fopen(memoryFile, "w");
    if (!file) {
        printf("Error saving memory state to %s.\n", memoryFile);
        return;
    }

    const int BUFFER_SIZE = 1024; // Define the size of the buffer
    char buffer[BUFFER_SIZE];
    int buffer_offset = 0;

    // Write Metadata (Index Table)
    buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset, "--- Metadata (Index Table) ---\n");
    for (int i = 0; i < metadataCount; i++) {
        buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset,
                                  "File: %s, First Block: %d, Blocks: %d, Products: %d\n",
                                  metadataTable[i].file_name, metadataTable[i].firstBlock,
                                  metadataTable[i].nbBloc, metadataTable[i].nbProduit);

        if (buffer_offset >= BUFFER_SIZE - 128) {
            fwrite(buffer, sizeof(char), buffer_offset, file);
            buffer_offset = 0; // Reset buffer
        }
    }

    // Write Block States and Product Data
    buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset, "\n--- Memory Blocks ---\n");
    for (int i = 0; i < total_blocks; i++) {
        if (memory[i].is_free) {
            buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset,
                                      "Block %d: FREE\n", i);
        } else {
            buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset,
                                      "Block %d: File ID: %d, Next Block: %d, Data:\n",
                                      i, memory[i].file_id, memory[i].next_block);

            if (strlen(memory[i].data) > 0) { // Only write non-empty data
                buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset,
                                          "%s\n", memory[i].data);
            } else {
                buffer_offset += snprintf(buffer + buffer_offset, BUFFER_SIZE - buffer_offset, "EMPTY\n");
            }
        }

        if (buffer_offset >= BUFFER_SIZE - 128) { // Flush buffer if nearing capacity
            fwrite(buffer, sizeof(char), buffer_offset, file);
            buffer_offset = 0; // Reset buffer
        }
    }

    // Write any remaining data in the buffer
    if (buffer_offset > 0) {
        fwrite(buffer, sizeof(char), buffer_offset, file);
    }

    fclose(file);
    printf("Memory state saved to %s using a buffer.\n", memoryFile);
}


void loadMemoryFromFile(const char *memoryFile) {
    FILE *file = fopen(memoryFile, "r");
    if (!file) {
        printf("No existing memory file. Starting with fresh memory.\n");
        return;
    }

    for (int i = 0; i < total_blocks; i++) {
        int is_free; // Use an integer for fscanf
        fscanf(file, "%d %d %d %[^\n]\n", // %[^\n] to read until newline for data
               &is_free, &memory[i].file_id, &memory[i].next_block, memory[i].data);
        memory[i].is_free = is_free; // Assign the integer to the _Bool field
    }

    fclose(file);
    printf("Memory state loaded from %s.\n", memoryFile);
}



void updateMetadata(const char *filename, int numProductsChange) {
    for (int i = 0; i < metadataCount; i++) {
        if (strcmp(metadataTable[i].file_name, filename) == 0) {
            metadataTable[i].nbProduit += numProductsChange; // Adjust by change amount
            if (metadataTable[i].nbProduit < 0) {
                metadataTable[i].nbProduit = 0; // Prevent negative values
            }
            return;
        }
    }
    printf("Metadata for %s not found.\n", filename);
}




void displayMemoryState() {
    printf("Memory state:\n");
    for (int i = 0; i < total_blocks; i++) {
        if (i == 0) {
            printf("[TableIndex]");
        } else if (memory[i].is_free) {
            printf("[FREE]");
        } else {
            printf("[FILE ID: %d]", memory[i].file_id);
        }
    }
    printf("\n");
}
void writeProductsToDisk(const char *inputFile, const char *diskFile) {
    FILE *input = fopen(inputFile, "r");
    FILE *disk = fopen(diskFile, "ab"); // Open in append binary mode to retain existing data

    if (!input || !disk) {
        printf("Error opening files for writing.\n");
        if (input) fclose(input);
        if (disk) fclose(disk);
        return;
    }

    char buffer[512]; // Single block buffer
    memset(buffer, 0, sizeof(buffer));

    char line[128];
    int buffer_offset = 0;

    // Write product data into disk blocks
    while (fgets(line, sizeof(line), input)) {
        // Copy product data into the buffer
        int len = snprintf(buffer + buffer_offset, sizeof(buffer) - buffer_offset, "%s", line);
        if (len < 0 || buffer_offset + len >= sizeof(buffer)) {
            // Buffer is full, write it to disk
            fwrite(buffer, sizeof(char), buffer_offset, disk);
            memset(buffer, 0, sizeof(buffer)); // Clear the buffer
            buffer_offset = 0;
        }
        buffer_offset += len;
    }

    // Write any remaining data in the buffer
    if (buffer_offset > 0) {
        fwrite(buffer, sizeof(char), buffer_offset, disk);
    }

    fclose(input);

    // Append metadata to disk
    FILE *metaDisk = fopen(diskFile, "ab"); // Reopen disk file in append mode for metadata
    if (!metaDisk) {
        printf("Error opening disk file for writing metadata.\n");
        return;
    }

    memset(buffer, 0, sizeof(buffer)); // Reuse buffer for metadata
    int meta_offset = 0;

    for (int i = 0; i < metadataCount; i++) {
        int len = snprintf(buffer + meta_offset, sizeof(buffer) - meta_offset,
                           "%s,%d,%d,%d\n",
                           metadataTable[i].file_name,
                           metadataTable[i].firstBlock,
                           metadataTable[i].nbBloc,
                           metadataTable[i].nbProduit);

        if (len < 0 || meta_offset + len >= sizeof(buffer)) {
            // Buffer is full, write it to disk
            fwrite(buffer, sizeof(char), meta_offset, metaDisk);
            memset(buffer, 0, sizeof(buffer));
            meta_offset = 0;
        }
        meta_offset += len;
    }

    if (meta_offset > 0) {
        fwrite(buffer, sizeof(char), meta_offset, metaDisk);
    }

    fclose(metaDisk);
    fclose(disk);

    printf("Products and metadata appended to disk using buffered approach.\n");
}

void initializeMemory(int num_blocks, int b_size) {
    total_blocks = num_blocks;
    block_size = b_size;
    memory = (Block *)malloc(num_blocks * sizeof(Block));

    for (int i = 0; i < num_blocks; i++) {
        memory[i].is_free = true;
        memory[i].file_id = 0;
        memory[i].data[0] = '\0';
        memory[i].next_block = -1;
    }

    // Reserve Block 0 for tableIndex
    memory[0].is_free = false;
    memory[0].file_id = -1; // Reserved for tableIndex
    printf("Memory initialized with %d blocks of %d bytes each\n", num_blocks, b_size);
}
void updateTableIndex() {
    memset(memory[0].data, 0, sizeof(memory[0].data)); // Clear previous data
    for (int i = 0; i < metadataCount; i++) {
        char metaDataStr[128];
        snprintf(metaDataStr, sizeof(metaDataStr), "%s,%d,%d,%d\n",
                 metadataTable[i].file_name, metadataTable[i].firstBlock,
                 metadataTable[i].nbBloc, metadataTable[i].nbProduit);
        strcat(memory[0].data, metaDataStr);
    }
    printf("TableIndex updated in memory block 0.\n");
}

void writeTableIndexToDisk(const char *diskFile) {
    FILE *disk = fopen(diskFile, "w");
    if (!disk) {
        printf("Error opening disk file for writing tableIndex.\n");
        return;
    }

    Block buffer;
    memset(&buffer, 0, sizeof(buffer));
    strcpy(buffer.data, memory[0].data);

    fwrite(buffer.data, sizeof(char), strlen(buffer.data), disk);
    fclose(disk);

    printf("TableIndex written to disk in %s\n", diskFile);
}

void deleteProductsFromDisk(const char *filename, const char *diskFile) {
    const int BUFFER_SIZE = 512; // Taille du buffer
    char buffer[BUFFER_SIZE];
    FILE *disk = fopen(diskFile, "r");
    FILE *temp = fopen("temp_products.txt", "w");

    if (!disk || !temp) {
        printf("Error opening files for product deletion.\n");
        if (disk) fclose(disk);
        if (temp) fclose(temp);
        return;
    }

    bool productDeleted = false;

    // Lire les données du fichier disque ligne par ligne
    while (fgets(buffer, sizeof(buffer), disk)) {
        produit p;
        if (sscanf(buffer, "%d,%30[^,],%f", &p.id, p.nom, &p.prix) == 3) {
            // Vérifier si le produit est associé au fichier donné
            if (strcmp(p.nom, filename) != 0) {
                // Écrire dans le fichier temporaire si le produit ne correspond pas
                fputs(buffer, temp);
            } else {
                productDeleted = true; // Marquer le produit pour suppression
            }
        } else {
            // Copier les lignes qui ne contiennent pas de produits
            fputs(buffer, temp);
        }
    }

    fclose(disk);
    fclose(temp);

    // Remplacer le fichier disque original par le fichier temporaire
    remove(diskFile);
    rename("temp_products.txt", diskFile);

    if (productDeleted) {
        printf("Products associated with '%s' deleted from disk.\n", filename);
    } else {
        printf("No products associated with '%s' found on disk.\n", filename);
    }
}




void deleteFileFromDisk(const char *filename, const char *diskFile) {
    // Open disk file and temporary file
    FILE *disk = fopen(diskFile, "r");
    FILE *temp = fopen("temp_disk.txt", "w");

    if (!disk || !temp) {
        printf("Error opening files for file deletion\n");
        if (disk) fclose(disk);
        if (temp) fclose(temp);
        return;
    }

    char line[128];
    bool fileFound = false;

    // Iterate through disk file to filter out lines associated with the file
    while (fgets(line, sizeof(line), disk)) {
        if (strstr(line, filename) != NULL) { // Match filename
            fileFound = true;
        } else {
            fprintf(temp, "%s", line); // Write unrelated lines to temp
        }
    }

    fclose(disk);
    fclose(temp);

    if (!fileFound) {
        printf("File '%s' not found on disk.\n", filename);
        remove("temp_disk.txt"); // Clean up temp file
        return;
    }

    // Replace the original disk file with the updated temp file
    remove(diskFile);
    rename("temp_disk.txt", diskFile);

    // Search and remove products related to the file
    deleteProductsFromDisk(filename, diskFile);

    printf("File '%s' and its associated products deleted from disk successfully.\n", filename);
}


void updateMemoryState() {
    // Clear all memory blocks first
    for (int i = 0; i < total_blocks; i++) {
        memory[i].is_free = true;
        memory[i].file_id = 0;
        memory[i].next_block = -1;
        memset(memory[i].data, 0, sizeof(memory[i].data));
    }

    // Re-allocate based on metadata
    for (int i = 0; i < metadataCount; i++) {
        meta *currentMeta = &metadataTable[i];
        int startBlock = currentMeta->firstBlock;
        for (int j = 0; j < currentMeta->nbBloc; j++) {
            memory[startBlock + j].is_free = false;
            memory[startBlock + j].file_id = i + 1;
            memory[startBlock + j].next_block = (j < currentMeta->nbBloc - 1) ? startBlock + j + 1 : -1;
        }
    }
    updateTableIndex();
}


int allocateChainedBlocks(int file_id, int num_blocks) {
    int start = -1, count = 0;

    for (int i = 1; i < total_blocks; i++) { // Start from block 1
        if (memory[i].is_free) {
            if (start == -1) start = i; // Mark the start
            memory[i].is_free = false;
            memory[i].file_id = file_id;
            memory[i].next_block = (count == num_blocks - 1) ? -1 : i + 1; // Link blocks
            count++;
            if (count == num_blocks) break;
        }
    }

    if (count < num_blocks) {
        printf("Failed to allocate %d chained blocks for file ID %d\n", num_blocks, file_id);
        return -1; // Allocation failed
    }

    printf("Allocated %d chained blocks starting at block %d for file ID %d\n", num_blocks, start, file_id);
    return start;
}
int allocateContiguousBlocks(int file_id, int num_blocks) {
    int start = -1, count = 0;

    for (int i = 1; i < total_blocks; i++) { // Start from block 1
        if (memory[i].is_free) {
            if (count == 0) start = i; // Mark the start
            count++;
            if (count == num_blocks) break;
        } else {
            count = 0; // Reset if a block is not free
        }
    }

    if (count < num_blocks) {
        printf("Failed to allocate %d contiguous blocks for file ID %d\n", num_blocks, file_id);
        return -1; // Allocation failed
    }

    for (int i = start; i < start + num_blocks; i++) {
        memory[i].is_free = false;
        memory[i].file_id = file_id;
        memory[i].next_block = -1; // No linking for contiguous allocation
    }

    printf("Allocated %d contiguous blocks starting at block %d for file ID %d\n", num_blocks, start, file_id);
    return start;
}


void addMetadata(meta newMeta) {
    if (metadataCount < 100) {
        metadataTable[metadataCount++] = newMeta;
        updateTableIndex(); // Update tableIndex in memory
    } else {
        printf("Metadata table is full\n");
    }
}




void fillProductFile(const char *filename) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        printf("Error creating or opening product file\n");
        return;
    }

    int num_products;
    printf("Enter the number of products to add: ");
    scanf("%d", &num_products);

    for (int i = 0; i < num_products; i++) {
        produit p;
        printf("Enter ID, name, and price for product %d (format: id,name,price): ", i + 1);
        scanf("%d,%30[^,],%f", &p.id, p.nom, &p.prix);
        fprintf(file, "%d,%s,%.2f\n", p.id, p.nom, p.prix);
    }

    fclose(file);

    // Ensure products are written to memory after being added to the file
    writeProductsToMemory(filename); 
    printf("Products saved to %s and written to memory.\n", filename);
}

void searchProductInMemory(int product_id) {
    for (int i = 0; i < total_blocks; i++) {
        if (!memory[i].is_free) {
            char *line = strtok(memory[i].data, "\n");
            while (line) {
                produit p;
                sscanf(line, "%d,%30[^,],%f", &p.id, p.nom, &p.prix);
                if (p.id == product_id) {
                    printf("Product found in memory (Block %d): ID = %d, Name = %s, Price = %.2f\n", i, p.id, p.nom, p.prix);
                    return;
                }
                line = strtok(NULL, "\n");
            }
        }
    }
    printf("Product with ID %d not found in memory.\n", product_id);
}

void displayDiskContents(const char *diskFile) {
    FILE *disk = fopen(diskFile, "r");
    if (!disk) {
        printf("Error opening file for reading\n");
        return;
    }

    char line[512];
    printf("Disk contents:\n");
    while (fgets(line, sizeof(line), disk)) {
        printf("%s", line);
    }

    fclose(disk);
}

void searchProductInFile(const char *filename, int product_id) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s for reading.\n", filename);
        return;
    }

    char line[128];
    produit p;
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%d,%30[^,],%f", &p.id, p.nom, &p.prix);
        if (p.id == product_id) {
            printf("Product found in file %s: ID = %d, Name = %s, Price = %.2f\n", filename, p.id, p.nom, p.prix);
            fclose(file);
            return;
        }
    }

    printf("Product with ID %d not found in file %s.\n", product_id, filename);
    fclose(file);
}
void searchProductInDisk(const char *diskFile, int product_id) {
    const int BUFFER_SIZE = 512; // Taille du buffer
    char buffer[BUFFER_SIZE];
    bool productFound = false;

    FILE *disk = fopen(diskFile, "r");
    if (!disk) {
        printf("Error: Could not open disk file %s for reading.\n", diskFile);
        return;
    }

    // Lire les données du fichier disque ligne par ligne
    while (fgets(buffer, sizeof(buffer), disk)) {
        produit p;
        if (sscanf(buffer, "%d,%30[^,],%f", &p.id, p.nom, &p.prix) == 3) {
            if (p.id == product_id) {
                printf("Product found on disk: ID = %d, Name = %s, Price = %.2f\n", p.id, p.nom, p.prix);
                productFound = true;
                break;
            }
        }
    }

    fclose(disk);

    if (!productFound) {
        printf("Product with ID %d not found on disk %s.\n", product_id, diskFile);
    }
}


void compactMemory() {
    int targetIndex = 1; // Start after block 0 (reserved)
    for (int i = 1; i < total_blocks; i++) {
        if (!memory[i].is_free) { // Find occupied blocks
            if (i != targetIndex) { // Move only if necessary
                memory[targetIndex] = memory[i]; // Copy block
                memory[i].is_free = true;        // Clear original block
                memory[i].file_id = 0;
                memory[i].next_block = -1;
                memset(memory[i].data, 0, sizeof(memory[i].data));
            }
            targetIndex++;
        }
    }
    updateTableIndex(); // Update metadata for the compacted layout
    printf("Memory compacted successfully.\n");
}

void defragmentFile(const char *filename) {
    int fileIndex = -1;
    for (int i = 0; i < metadataCount; i++) {
        if (strcmp(metadataTable[i].file_name, filename) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("Erreur : fichier %s non trouvé.\n", filename);
        return;
    }

    meta *fileMeta = &metadataTable[fileIndex];
    int startBlock = fileMeta->firstBlock;
    int newStartBlock = -1;

    // Trouver un espace contigu pour les blocs
    for (int i = 1; i <= total_blocks - fileMeta->nbBloc; i++) {
        bool canAllocate = true;
        for (int j = 0; j < fileMeta->nbBloc; j++) {
            if (!memory[i + j].is_free) {
                canAllocate = false;
                break;
            }
        }
        if (canAllocate) {
            newStartBlock = i;
            break;
        }
    }

    if (newStartBlock == -1) {
        printf("Défragmentation échouée : pas assez d'espace contigu.\n");
        return;
    }

    // Déplacer les données vers le nouvel emplacement contigu
    for (int i = 0; i < fileMeta->nbBloc; i++) {
        memory[newStartBlock + i] = memory[startBlock + i];
        memory[startBlock + i].is_free = true;
        memory[startBlock + i].file_id = 0;
        memory[startBlock + i].next_block = -1;
        memset(memory[startBlock + i].data, 0, sizeof(memory[startBlock + i].data));
    }

    fileMeta->firstBlock = newStartBlock;

    printf("Défragmentation effectuée pour le fichier %s.\n", filename);
    displayMemoryState(); // Facultatif : Montre l'état après défragmentation
}
int allocateBlocks(int file_id, int num_blocks, int allocationType) {
    if (allocationType == 1) {
        return allocateChainedBlocks(file_id, num_blocks);
    } else if (allocationType == 2) {
        return allocateContiguousBlocks(file_id, num_blocks);
    }
    return -1; // Invalid allocation type
}

void linkLastToNewFile(int lastBlock, int newFileFirstBlock, int allocationType) {
    if (allocationType == 1 && lastBlock != -1 && newFileFirstBlock != -1) {
        memory[lastBlock].next_block = newFileFirstBlock;
        printf("Linked last block %d to new file's first block %d.\n", lastBlock, newFileFirstBlock);
    }
}



void writeProductsToMemory(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening product file %s for reading.\n", filename);
        return;
    }

    int blockIndex = -1;

    // Find the starting block for this file in the metadata
    for (int i = 0; i < metadataCount; i++) {
        if (strcmp(metadataTable[i].file_name, filename) == 0) {
            blockIndex = metadataTable[i].firstBlock;
            break;
        }
    }

    if (blockIndex == -1) {
        printf("Metadata for file %s not found. Unable to write to memory.\n", filename);
        fclose(file);
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        // Ensure space is available in the current block
        if (strlen(memory[blockIndex].data) + strlen(line) < sizeof(memory[blockIndex].data)) {
            strcat(memory[blockIndex].data, line); // Append product data
        } else {
            // Move to the next block in the chain
            if (memory[blockIndex].next_block != -1) {
                blockIndex = memory[blockIndex].next_block;
                strcat(memory[blockIndex].data, line); // Write to the next block
            } else {
                printf("No more blocks available for file %s. Data truncated.\n", filename);
                break;
            }
        }
    }

    fclose(file);
    printf("Products from %s written to memory.\n", filename);
}


void deleteBlockFromMemory(int blockIndex) {
    if (blockIndex <= 0 || blockIndex >= total_blocks) {
        printf("Erreur : index du bloc invalide.\n");
        return;
    }

    memory[blockIndex].is_free = true;
    memory[blockIndex].file_id = 0;
    memory[blockIndex].next_block = -1;
    memset(memory[blockIndex].data, 0, sizeof(memory[blockIndex].data));

    printf("Bloc %d supprimé de la mémoire.\n", blockIndex);
}

int main() {
    srand(time(NULL));
    int mem_blocks;
    printf("How many blocks is your memory made out of ? ");
    scanf("%d", &mem_blocks);
    initializeMemory(mem_blocks, 512);
    loadMemoryFromFile("central_memory.txt");
    int allocationType; // 1 for Chained, 2 for Contiguous
    printf("Choose allocation type for the file system (1 for Chained, 2 for Contiguous): ");
    scanf("%d", &allocationType);

    if (allocationType != 1 && allocationType != 2) {
        printf("Invalid allocation type. Exiting.\n");
        return 1;
    }



    int choice;

    do {
        printf("\n--- File System Menu ---\n");
        printf("1. Create a new file\n");
        printf("2. Add products to an existing file\n");
        printf("3. Delete a product from a file\n");
        printf("4. Delete a file from memory\n");
        printf("5. Display disk contents\n");
        printf("6. Display memory state\n");
        printf("7. Search for a product in memory\n");
        printf("8. Compactage de la mémoire secondaire\n");
        printf("9. Défragmentation d’un fichier spécifique\n");
        printf("10. Delete a file from disk\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
          case 1: { // Create a new file
    char filename[50];
    printf("Enter the file name: ");
    scanf("%s", filename);

    int numBlocks;
    printf("Enter the number of blocks to allocate: ");
    scanf("%d", &numBlocks);

    if (numBlocks > mem_blocks) {
        printf("Error: Number of blocks exceeds maximum capacity.\n");
        break;
    }

    int startBlock = allocateBlocks(metadataCount + 1, numBlocks, allocationType);

    if (startBlock != -1) {
        // Link the last block of the previous file to the first block of this file
        if (metadataCount > 0) {
            int lastBlock = metadataTable[metadataCount - 1].firstBlock;
            while (memory[lastBlock].next_block != -1) {
                lastBlock = memory[lastBlock].next_block;
            }
            linkLastToNewFile(lastBlock, startBlock, allocationType);
        }

        meta newMeta = {0};
        snprintf(newMeta.file_name, sizeof(newMeta.file_name), "%s", filename);
        newMeta.firstBlock = startBlock;
        newMeta.nbBloc = numBlocks;
        newMeta.nbProduit = 0; // Initialize product count
        addMetadata(newMeta);

        for (int i = 0; i < numBlocks * facteur_de_blocage; i++) {
            produit p;
            printf("Enter ID, name, and price for product %d (or -1 to stop): ", i + 1);
            scanf("%d,%30[^,],%f", &p.id, p.nom, &p.prix);

            if (p.id == -1) break;

            FILE *file = fopen(filename, "a");
            if (file) {
                fprintf(file, "%d,%s,%.2f\n", p.id, p.nom, p.prix);
                fclose(file);
                newMeta.nbProduit++;
            } else {
                printf("Error opening file %s for writing.\n", filename);
                break;
            }
        }

        writeProductsToMemory(filename); // Write products to memory
        synchronizeDiskWithFile(filename, "disk.txt", newMeta.nbProduit); // Sync metadata and disk
        updateMemoryState();
        displayMemoryState();
    } else {
        printf("Failed to allocate blocks for the file.\n");
    }
    break;
}


            case 2: { // Add products to an existing file
                char filename[50];
                printf("Enter the file name: ");
                scanf("%s", filename);

                int fileIndex = -1;
                for (int i = 0; i < metadataCount; i++) {
                    if (strcmp(metadataTable[i].file_name, filename) == 0) {
                        fileIndex = i;
                        break;
                    }
                }

                if (fileIndex == -1) {
                    printf("Error: File %s not found.\n", filename);
                    break;
                }

                meta *fileMeta = &metadataTable[fileIndex];
                int maxProducts = fileMeta->nbBloc * facteur_de_blocage;

                int numProductsToAdd;
                printf("Enter the number of products to add: ");
                scanf("%d", &numProductsToAdd);

                int totalProducts = fileMeta->nbProduit + numProductsToAdd;
                if (totalProducts > maxProducts) {
                    printf("Insufficient space in current allocation.\n");
                    printf("Reallocate more blocks? (1 for Yes, 0 for No): ");
                    int reallocate;
                    scanf("%d", &reallocate);

                    if (reallocate) {
                        int additionalBlocks = (totalProducts + facteur_de_blocage - 1) / facteur_de_blocage - fileMeta->nbBloc;
                        printf("Is the file allocation type (1 for Chained, 2 for Contiguous): ");
                        int allocationType;
                        scanf("%d", &allocationType);

                        if (allocationType == 1) {
                            int currentBlock = fileMeta->firstBlock;
                            while (memory[currentBlock].next_block != -1) {
                                currentBlock = memory[currentBlock].next_block;
                            }

                            for (int i = 0; i < additionalBlocks; i++) {
                                int newBlock = allocateChainedBlocks(fileIndex + 1, 1);
                                if (newBlock == -1) {
                                    printf("Failed to allocate additional blocks for chained allocation.\n");
                                    break;
                                }
                                memory[currentBlock].next_block = newBlock;
                                currentBlock = newBlock;
                            }
                        } else if (allocationType == 2) {
                            int startBlock = allocateContiguousBlocks(fileIndex + 1, additionalBlocks);
                            if (startBlock == -1) {
                                printf("Failed to allocate additional contiguous blocks.\n");
                                break;
                            }
                        } else {
                            printf("Invalid allocation type.\n");
                            break;
                        }

                        fileMeta->nbBloc += additionalBlocks;
                        maxProducts = fileMeta->nbBloc * facteur_de_blocage;
                    } else {
                        printf("Add fewer products to fit the allocated blocks.\n");
                        break;
                    }
                }

                for (int i = 0; i < numProductsToAdd; i++) {
                    produit p;
                    printf("Enter ID, name, and price for product %d: ", fileMeta->nbProduit + i + 1);
                    scanf("%d,%30[^,],%f", &p.id, p.nom, &p.prix);

                    FILE *file = fopen(filename, "a");
                    if (file) {
                        fprintf(file, "%d,%s,%.2f\n", p.id, p.nom, p.prix);
                        fclose(file);
                    } else {
                        printf("Error opening file %s for writing.\n", filename);
                        break;
                    }
                }

                writeProductsToMemory(filename); // Write to memory
                synchronizeDiskWithFile(filename, "disk.txt", numProductsToAdd); // Update metadata and sync
                updateMemoryState();
                displayMemoryState();
                break;
            }

            case 3: { // Delete a product
                char filename[50];
                int product_id;
                printf("Enter the file name: ");
                scanf("%s", filename);
                printf("Enter the product ID to delete: ");
                scanf("%d", &product_id);

                deleteProductFromFile(filename, product_id);
                synchronizeDiskWithFile(filename, "disk.txt", -1); // Decrement product count
                updateMemoryState();
                displayMemoryState();
                break;
            }
            case 4: { 
    char filename[50];
    printf("Enter the file name to delete from memory: ");
    scanf("%s", filename);

    int fileIndex = -1;

    // Find the file in the metadata
    for (int i = 0; i < metadataCount; i++) {
        if (strcmp(metadataTable[i].file_name, filename) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("Error: File '%s' not found in memory.\n", filename);
        break;
    }

    meta *fileMeta = &metadataTable[fileIndex];

    // Delete the blocks
    if (fileMeta->nbBloc > 0) {
        if (fileMeta->firstBlock != -1) {
            if (memory[fileMeta->firstBlock].next_block != -1) {
                deleteChainedBlocks(fileMeta->firstBlock);
            } else {
                deleteContiguousBlocks(fileMeta->firstBlock, fileMeta->nbBloc);
            }
        }
    }

    // Remove metadata
    for (int i = fileIndex; i < metadataCount - 1; i++) {
        metadataTable[i] = metadataTable[i + 1];
    }
    metadataCount--;

    updateTableIndex();
    saveMemoryToFile("central_memory.txt");

    printf("File '%s' and associated blocks deleted from memory successfully.\n", filename);
    break;
}


            case 5: { // Display disk contents
    printf("--- Disk Contents ---\n");
    displayDiskContents("disk.txt");
    break;
}


            case 6: { // Display memory state
                displayMemoryState();
                break;
            }

            case 7: { // Search for a product
                int searchOption;
                int product_id;
                char filename[50];
                printf("Search Options:\n");
                printf("1. Search in a file\n");
                printf("2. Search in memory\n");
                printf("3. Search in disk\n");
                printf("Enter your choice: ");
                scanf("%d", &searchOption);

                printf("Enter the product ID to search for: ");
                scanf("%d", &product_id);

                switch (searchOption) {
                    case 1:
                        printf("Enter the file name: ");
                        scanf("%s", filename);
                        searchProductInFile(filename, product_id);
                        break;
                    case 2:
                        searchProductInMemory(product_id);
                        break;
                    case 3:
                        searchProductInDisk("disk.txt", product_id);
                        break;
                    default:
                        printf("Invalid search option.\n");
                        break;
                }
                break;
            }
            case 8: { // Compactage
    printf("Starting memory compaction...\n");
    compactMemory();
    saveMemoryToFile("central_memory.txt");
    printf("Memory compacted and saved successfully.\n");
    break;
}


            case 9: { // Défragmentation
    char filename[50];
    printf("Enter the file name to defragment: ");
    scanf("%s", filename);

    printf("Starting defragmentation for file '%s'...\n", filename);
    defragmentFile(filename);
    saveMemoryToFile("central_memory.txt");
    printf("Defragmentation completed for file '%s'.\n", filename);
    break;
}
case 10: { 
    char filename[50];
    printf("Enter the file name to delete from disk: ");
    scanf("%s", filename);

    deleteFileFromDisk(filename, "disk.txt");

    printf("File '%s' deleted from disk successfully.\n", filename);
    break;
}



            case 0: {
                saveMemoryToFile("central_memory.txt");
                printf("Exiting program.\n");
                break;
            }

            default: {
                printf("Invalid choice. Please try again.\n");
                break;
            }
        }
    } while (choice != 0);

    return 0;
}
