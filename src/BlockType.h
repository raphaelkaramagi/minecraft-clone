#ifndef BLOCKTYPE_H
#define BLOCKTYPE_H

// Add more block types here as needed
enum class BlockType : unsigned char { // Using unsigned char for smaller memory footprint per block
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3
    // Water, Sand, Wood, etc.
};

#endif // BLOCKTYPE_H 