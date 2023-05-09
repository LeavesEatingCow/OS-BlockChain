#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 25000
#define HASH_SIZE 20 // SHA-1 hash size

typedef unsigned char byte;
void write_block(byte *data, int size, char *filename);

// Data structure for a block
typedef struct block {
    int index;
    byte hash[HASH_SIZE];
    char *filename;
    byte *data;
    struct block *next;
} Block;

// Calculate hash of data
void hash_data(byte *data, int size, byte *hash) {
    // Basic hash function
    for (int i = 0; i < size; i++) {
        hash[i%HASH_SIZE] ^= data[i];
    }
}

// Create a new block with given data
Block *new_block(byte *data, int size, int index, char *dir_path) {
    Block *block = (Block*) malloc(sizeof(Block));
    hash_data(data, size, block->hash);
    char filename[64];
    sprintf(filename, "%s/block_%d", dir_path, index);
    block->filename = strdup(filename);
    write_block(data, size, block->filename);
    block->data = data;
    block->index = index;
    block->next = NULL;
    return block;
}


// Write block data to file
void write_block(byte *data, int size, char *filename) {
    FILE *fp = fopen(filename, "wb");
    fwrite(data, size, 1, fp);
    fclose(fp);
}

// Read block data from file
byte *read_block(char *filename, int *size) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error: cannot open file %s\n", filename);
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    byte *data = (byte*) malloc(*size);
    fread(data, *size, 1, fp);
    fclose(fp);
    return data;
}

// Write manifest file with block hash IDs and pointers
void write_manifest(Block **block_array, int num_blocks, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error: cannot open file %s\n", filename);
        return;
    }

    // Write parent hash ID
    fprintf(fp, "Parent: ");
    for (int i = 0; i < HASH_SIZE; i++) {
        fprintf(fp, "%02x", block_array[0]->hash[i]);
    }
    fprintf(fp, "\n\n");

    // Write block information
    for (int i = 0; i < num_blocks; i++) {
        fprintf(fp, "Block %d:\n", i);
        fprintf(fp, "  Hash: ");
        for (int j = 0; j < HASH_SIZE; j++) {
            fprintf(fp, "%02x", block_array[i]->hash[j]);
        }
        fprintf(fp, "\n");
        fprintf(fp, "  Filename: %s\n", block_array[i]->filename);
        if (block_array[i]->next != NULL) {
            fprintf(fp, "  Next: %s\n", block_array[i]->next->filename);
        } else {
            fprintf(fp, "  Next: (none)\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_dir>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Error: cannot open file %s\n", argv[1]);
        return 1;
    }

    byte *data = (byte*) malloc(BLOCK_SIZE);
    int size;
    int num_blocks = 0;
    Block *blocks[1000];
    while ((size = fread(data, 1, BLOCK_SIZE, fp)) > 0) {
        Block *block = new_block(data, size, num_blocks, argv[2]);
        blocks[num_blocks++] = block;
    }
    fclose(fp);
    free(data);

    // Link blocks together
    for (int i = 0; i < num_blocks - 1; i++) {
        blocks[i]->next = blocks[i+1];
    }

    // Write manifest file
    char manifest_file[32];
    sprintf(manifest_file, "%s/manifest.txt", argv[2]);
    write_manifest(blocks, num_blocks, manifest_file);

    // Free blocks
    for (int i = 0; i < num_blocks; i++) {
        free(blocks[i]->filename);
        free(blocks[i]);
    }

    return 0;
}

