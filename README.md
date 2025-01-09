# Projet-SFSD
# File System Simulator

This project is a file system simulator implemented in C. It allows users to manage files, products, and memory blocks using either contiguous or chained allocation strategies. The program provides a menu-driven interface for performing various operations such as creating files, adding/deleting products, searching for products, compacting memory, defragmenting files, and more.

## Features

- **File Management**:
  - Create new files with a specified number of blocks.
  - Add products to existing files.
  - Delete products from files.
  - Delete files from memory and disk.

- **Memory Management**:
  - Allocate memory blocks using either contiguous or chained allocation.
  - Compact memory to optimize space usage.
  - Defragment specific files to reduce fragmentation.

- **Product Management**:
  - Add products to files.
  - Delete products from files.
  - Search for products in memory, files, or disk.

- **Disk Operations**:
  - Synchronize memory with disk.
  - Display disk contents.
  - Delete files from disk.

- **Metadata Management**:
  - Maintain metadata for each file, including file name, first block, number of blocks, and number of products.
  - Update metadata when files or products are modified.

## Code Structure

### Struct Definitions

- **`meta`**: Metadata for a file, including file name, first block, number of blocks, and number of products.
- **`produit`**: Represents a product with an ID, name, and price.
- **`Block`**: Represents a memory block with fields for free status, file ID, data buffer, and next block pointer.

### Global Variables

- **`memory`**: Array of `Block` structures representing the memory.
- **`total_blocks`**: Total number of memory blocks.
- **`block_size`**: Size of each memory block.
- **`metadataTable`**: Array of `meta` structures to store metadata for up to 100 files.
- **`metadataCount`**: Number of metadata entries in `metadataTable`.
- **`facteur_de_blocage`**: Blocking factor (number of products per block).

### Key Functions

- **`initializeMemory`**: Initializes memory with a specified number of blocks and block size.
- **`allocateBlocks`**: Allocates blocks for a file using either contiguous or chained allocation.
- **`addMetadata`**: Adds metadata for a new file to the metadata table.
- **`writeProductsToDisk`**: Writes products from a file to disk.
- **`deleteFileFromDisk`**: Deletes a file and its associated products from disk.
- **`deleteProductFromFile`**: Deletes a product from a file and updates metadata.
- **`synchronizeDiskWithFile`**: Synchronizes disk with file by updating metadata and writing products to disk.
- **`displayMemoryState`**: Displays the current state of memory.
- **`compactMemory`**: Compacts memory by moving all occupied blocks to the beginning.
- **`defragmentFile`**: Defragments a specific file by moving its blocks to a contiguous space.
- **`searchProductInMemory`**: Searches for a product in memory.
- **`searchProductInFile`**: Searches for a product in a file.
- **`searchProductInDisk`**: Searches for a product on disk.

### Main Function

The `main` function provides a menu-driven interface for interacting with the file system. Users can choose from various options to manage files, products, and memory.

## Usage

1. **Compile the Program**:
   ```bash
   gcc -o filesystem filesystem.c
