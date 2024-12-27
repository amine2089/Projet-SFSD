#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include<time.h>

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
int facteur_de_blocage = 4; // Blocking factor (number of products per buffer)

// Function prototypes
void initializeMemory(int num_blocks, int b_size);
void displayMemoryState();
int allocateBlocks(int file_id, int num_blocks);
void addMetadata(meta newMeta);
void writeProductsToDisk(const char *inputFile, const char *diskFile);
void writeMetadataToDisk(const char *diskFile);
void displayDiskContents(const char *diskFile);
void fillProductFile(const char *filename);

// Function implementations
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
	printf("Memory initialized with %d blocks of %d bytes each\n", num_blocks, b_size);
}

void displayMemoryState() {
	printf("Memory state:\n");
	for (int i = 0; i < total_blocks; i++) {
		if (memory[i].is_free) {
			printf("[FREE]");
		} else {
			printf("[FILE ID: %d]", memory[i].file_id);
		}
	}
	printf("\n");
}

int allocateBlocks(int file_id, int num_blocks) {
    int start = -1, count = 0;

    for (int i = 0; i < total_blocks; i++) {
        if (memory[i].is_free) {
            if (count == 0) start = i;
            count++;
            if (count == num_blocks) break;
        } else {
            count = 0; // Reset if a block is not free
        }
    }

    if (count < num_blocks) {
        printf("Failed to allocate %d blocks for file ID %d\n", num_blocks, file_id);
        return -1; // Allocation failed
    }

    for (int i = start; i < start + num_blocks; i++) {
        memory[i].is_free = false;
        memory[i].file_id = file_id;
        memset(memory[i].data, 0, sizeof(memory[i].data));
        memory[i].next_block = (i < start + num_blocks - 1) ? i + 1 : -1;
    }

    printf("Allocated %d blocks starting at block %d for file ID %d\n", num_blocks, start, file_id);
    return start;
}


void writeProductsToDisk(const char *inputFile, const char *diskFile) {
	FILE *input = fopen(inputFile, "r");
	FILE *disk = fopen(diskFile, "r+");

	if (!input || !disk) {
		printf("Error opening files for writing\n");
		return;
	}

	Block buffer;
	int bufferCount = 0;
	char line[128];

	while (fgets(line, sizeof(line), input)) {
		produit p;
		sscanf(line, "%d,%30[^,],%f", &p.id, p.nom, &p.prix);

		char productData[128];
		snprintf(productData, sizeof(productData), "%d,%s,%.2f", p.id, p.nom, p.prix);

		strcat(buffer.data, productData);
		strcat(buffer.data, "\n");
		bufferCount++;

		if (bufferCount == facteur_de_blocage) {
			fwrite(buffer.data, sizeof(char), strlen(buffer.data), disk);
			memset(buffer.data, 0, sizeof(buffer.data));
			bufferCount = 0;
		}
	}

	if (bufferCount > 0) {
		fwrite(buffer.data, sizeof(char), strlen(buffer.data), disk);
	}

	fclose(input);
	fclose(disk);
	printf("Products written to disk from %s to %s\n", inputFile, diskFile);
}
void deleteFileFromDisk(const char *filename, const char *diskFile) {
    FILE *disk = fopen(diskFile, "r");
    FILE *temp = fopen("temp_disk.txt", "w");

    if (!disk || !temp) {
        printf("Error opening files for file deletion\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), disk)) {
        if (strstr(line, filename) == NULL) { // Skip lines containing the filename
            fprintf(temp, "%s", line);
        }
    }

    fclose(disk);
    fclose(temp);
    remove(diskFile);
    rename("temp_disk.txt", diskFile);
    printf("File %s deleted from disk %s\n", filename, diskFile);
}

void synchronizeDiskWithFile(const char *filename, const char *diskFile) {
    writeProductsToDisk(filename, diskFile);
    printf("Disk synchronized with file %s\n", filename);
}
void deleteProductFromFile(const char *filename, int product_id) {
    FILE *file = fopen(filename, "r");
    FILE *temp = fopen("temp.txt", "w");

    if (!file || !temp) {
        printf("Error opening files for deletion\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        produit p;
        sscanf(line, "%d,%30[^,],%f", &p.id, p.nom, &p.prix);
        if (p.id != product_id) {
            fprintf(temp, "%s", line);
        }
    }

    fclose(file);
    fclose(temp);
    remove(filename);
    rename("temp.txt", filename);
    printf("Product with ID %d deleted from %s\n", product_id, filename);
}


void writeMetadataToDisk(const char *diskFile) {
	FILE *disk = fopen(diskFile, "a");

	if (!disk) {
		printf("Error opening file for writing metadata\n");
		return;
	}

	Block buffer;
	for (int i = 0; i < metadataCount; i++) {
		char metadataStr[128];
		snprintf(metadataStr, sizeof(metadataStr), "%s,%d,%d,%d\n", metadataTable[i].file_name, metadataTable[i].firstBlock, metadataTable[i].nbBloc, metadataTable[i].nbProduit);

		strcat(buffer.data, metadataStr);

		if (strlen(buffer.data) + strlen(metadataStr) >= sizeof(buffer.data)) {
			fwrite(buffer.data, sizeof(char), strlen(buffer.data), disk);
			memset(buffer.data, 0, sizeof(buffer.data));
		}
	}

	if (strlen(buffer.data) > 0) {
		fwrite(buffer.data, sizeof(char), strlen(buffer.data), disk);
	}

	fclose(disk);
	printf("Metadata written to disk in %s\n", diskFile);
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
void addMetadata(meta newMeta) {
    if (metadataCount < 100) { // Ensure the metadata table doesn't overflow
        metadataTable[metadataCount++] = newMeta; // Add new metadata to the table
        printf("Metadata added: %s, Start Block: %d, Blocks: %d, Products: %d\n",
               newMeta.file_name, newMeta.firstBlock, newMeta.nbBloc, newMeta.nbProduit);
    } else {
        printf("Metadata table is full. Cannot add new metadata.\n");
    }
}


void fillProductFile(const char *filename) {
	FILE *file = fopen(filename, "w");
	if (!file) {
		printf("Error creating product file\n");
		return;
	}

	int num_products;
	printf("Enter the number of products to add: ");
	scanf("%d", &num_products);

	for (int i = 0; i < num_products; i++) {
		produit p;
		printf("Enter ID, name, and price for product %d: ", i + 1);
		scanf("%d,%30[^,],%f", &p.id, p.nom, &p.prix);
		fprintf(file, "%d,%s,%.2f\n", p.id, p.nom, p.prix);
	}

	fclose(file);
	printf("Products saved to %s\n", filename);
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
}

int main() {
    srand(time(NULL));
    initializeMemory(10, 512);

    int choice;

    do {
        printf("\n--- File System Menu ---\n");
        printf("1. Add more products to an existing file\n");
        printf("2. Create a new file and add products\n");
        printf("3. Delete a product from an existing file\n");
        printf("4. Delete an entire file from the disk\n");
        printf("5. Display disk contents\n");
        printf("6. Display memory state\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                char filename[50];
                printf("Enter the name of the existing file: ");
                scanf("%s", filename);
                fillProductFile(filename);
                synchronizeDiskWithFile(filename, "disk.txt");
                displayDiskContents("disk.txt");
                writeMetadataToDisk("disk.txt");
                updateMemoryState(); // Update memory after changes
                displayMemoryState();
                break;
            }
            case 2: {
    char filename[50];
    printf("Enter the name for the new file: ");
    scanf("%s", filename);
    FILE *newFile = fopen(filename, "w");
    if (newFile) {
        fclose(newFile);
        fillProductFile(filename);

        int numBlocks;
        printf("Enter the number of blocks to allocate for the new file: ");
        scanf("%d", &numBlocks);
        int startBlock = allocateBlocks(metadataCount + 1, numBlocks);

        if (startBlock != -1) {
            meta newMeta;
            snprintf(newMeta.file_name, sizeof(newMeta.file_name), "%s", filename);
            newMeta.firstBlock = startBlock;
            newMeta.nbBloc = numBlocks;
            newMeta.nbProduit = 0; // You can update this with actual product count if needed
            addMetadata(newMeta);

            synchronizeDiskWithFile(filename, "disk.txt");
            writeMetadataToDisk("disk.txt");
            updateMemoryState(); // Ensure all blocks are allocated properly
            displayDiskContents("disk.txt");
            displayMemoryState();
        } else {
            printf("Failed to allocate blocks for the new file.\n");
        }
    } else {
        printf("Error creating the new file.\n");
    }
    break;
}

            case 3: {
                char filename[50];
                int product_id;
                printf("Enter the name of the file to delete the product from: ");
                scanf("%s", filename);
                printf("Enter the ID of the product to delete: ");
                scanf("%d", &product_id);
                deleteProductFromFile(filename, product_id);
                synchronizeDiskWithFile(filename, "disk.txt");
                displayDiskContents("disk.txt");
                updateMemoryState(); // Update memory after changes
                displayMemoryState();
                break;
            }
            case 4: {
                char filename[50];
                printf("Enter the name of the file to delete from disk: ");
                scanf("%s", filename);
                deleteFileFromDisk(filename, "disk.txt");

                // Remove the file's metadata
                for (int i = 0; i < metadataCount; i++) {
                    if (strcmp(metadataTable[i].file_name, filename) == 0) {
                        // Shift metadata entries to remove the deleted file
                        for (int j = i; j < metadataCount - 1; j++) {
                            metadataTable[j] = metadataTable[j + 1];
                        }
                        metadataCount--;
                        break;
                    }
                }

                updateMemoryState(); // Update memory after changes
                displayDiskContents("disk.txt");
                displayMemoryState();
                break;
            }
            case 5:
                displayDiskContents("disk.txt");
                break;
            case 6:
                displayMemoryState();
                break;
            case 0:
                printf("Exiting program.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    } while (choice != 0);

    return 0;
}
