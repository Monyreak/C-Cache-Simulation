#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CACHE_SIZE 100 // Adjust as needed
#define MAX_ITEM   100 // Adjust as needed
#define TABLE_SIZE 1024 // Hash table size

// Assume the list node structure
typedef struct node {
    char *element;
    int ref_bit;
    struct node *prev, *next;
} Node;

// Hash table node
typedef struct hashNode {
    char *key;
    struct hashNode *next;
} HashNode;

// Assume the cache structure
typedef struct {
    Node **items;
    HashNode **hashTable; // Hash table
    char policy;
    int size;
    int clock_ptr;
    int hit_count;
    int miss_count;
    int compulsory_misses;
    int capacity_misses;
} Cache;

// Creating new Node
Node *new_node(char *element) {
    Node *node = (Node *) malloc(sizeof(Node));
    node->element = element;
    node->ref_bit = 0;
    node->prev = node->next = NULL;
    return node;
}

// Creating new HashNode
HashNode *new_hashNode(char *key) {
    HashNode *node = (HashNode *) malloc(sizeof(HashNode));
    node->key = key;
    node->next = NULL;
    return node;
}

// Hash function
unsigned int hash(char *key) {
    unsigned int value = 0;
    for (char *p = key; *p != '\0'; p++) {
        value = value * 31 + *p;
    }
    return value % TABLE_SIZE;
}

// Insert a key into the hash table
void hash_insert(Cache *cache, char *key) {
    unsigned int index = hash(key);
    HashNode *new = new_hashNode(key);
    if (cache->hashTable[index] == NULL) {
        cache->hashTable[index] = new;
    } else {
        // Chaining for collision resolution
        HashNode *node = cache->hashTable[index];
        while (node->next != NULL) {
            node = node->next;
        }
        // Overwrite the last key if it already exists
        if (strcmp(node->key, key) == 0) {
            node->key = key;
        } else {
            node->next = new;
        }
    }
}

// Check if a key is in the hash table
bool hash_search(Cache *cache, char *key) {
    unsigned int index = hash(key);
    HashNode *node = cache->hashTable[index];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return true;
        }
        node = node->next;
    }
    return false;
}

// Creating new Cache
Cache *cache_new(char policy, int size) {
    Cache *cache = (Cache *) malloc(sizeof(Cache));
    cache->policy = policy;
    cache->size = size;
    cache->clock_ptr = 0;
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->compulsory_misses = 0;
    cache->capacity_misses = 0;
    cache->items = (Node **) malloc(sizeof(Node *) * size);
    cache->hashTable = (HashNode **) malloc(sizeof(HashNode *) * TABLE_SIZE);
    for (int i = 0; i < size; i++)
        cache->items[i] = NULL;
    for (int i = 0; i < TABLE_SIZE; i++)
        cache->hashTable[i] = NULL;
    return cache;
}

bool list_is_full(Node **items, int size) {
    for (int i = 0; i < size; i++) {
        if (items[i] == NULL)
            return false;
    }
    return true;
}

// Check if list contains the element
bool list_contains(Node **items, char *element, int size) {
    for (int i = 0; i < size; i++) {
        if (items[i] != NULL && strcmp(items[i]->element, element) == 0)
            return true;
    }
    return false;
}

// Move the element to the end of the list
void list_move_end(Node **items, char *element, int size) {
    Node *temp;
    int i;
    for (i = 0; i < size; i++) {
        if (items[i] != NULL && strcmp(items[i]->element, element) == 0) {
            temp = items[i];
            break;
        }
    }
    for (; i < size - 1; i++)
        items[i] = items[i + 1];
    items[size - 1] = temp;
}

// Remove the front element from the list
void list_remove_front(Node **items, int size) {
    free(items[0]);
    for (int i = 1; i < size; i++)
        items[i - 1] = items[i];
    items[size - 1] = NULL;
}

// Add the element to the end of the list
void list_push_back(Node **items, Node *node, int size) {
    for (int i = 0; i < size; i++) {
        if (items[i] == NULL) {
            items[i] = node;
            break;
        }
    }
}

// Get the element from the list
Node *list_get(Node **items, char *element, int size) {
    for (int i = 0; i < size; i++) {
        if (items[i] != NULL && strcmp(items[i]->element, element) == 0)
            return items[i];
    }
    return NULL;
}

// Get the element at the given index from the list
Node *list_get_index(Node **items, int index) {
    return items[index];
}

// Overwrite the element at the given index in the list
void list_overwrite_index(Node **items, int index, Node *node) {
    free(items[index]);
    items[index] = node;
}

// Implementing insert functions for each caching policy, abbreviated for brevity

bool insert_fifo(Cache *cache, char *element) {
    if (list_contains(cache->items, element, cache->size)) {
        cache->hit_count++;
        printf("HIT\n");
        return true;
    }

    // Always increment miss_count as we didn't find the element
    cache->miss_count++;
    printf("MISS\n");

    Node *new = new_node(element);

    if (list_is_full(cache->items, cache->size)) {
        // The cache is full, check if the element is in the hash table
        if (hash_search(cache, element)) {
            // The element is in the hash table, so increment capacity_misses
            cache->capacity_misses++;
        } else {
            // The element is not in the hash table, so increment compulsory_misses and add to the hash table
            cache->compulsory_misses++;
            hash_insert(cache, element);
        }
        list_remove_front(cache->items, cache->size);
    } else {
        // The cache is not full, check if the element is in the hash table
        if (hash_search(cache, element)) {
            // The element is in the hash table, so it's a conflict miss (treated as a capacity miss)
            cache->capacity_misses++;
        } else {
            // The element is not in the hash table, so increment compulsory_misses and add to the hash table
            cache->compulsory_misses++;
            hash_insert(cache, element);
        }
    }

    // In either case, we add the new node to the cache
    list_push_back(cache->items, new, cache->size);

    return false;
}

bool insert_lru(Cache *cache, char *element) {
    if (list_contains(cache->items, element, cache->size)) {
        cache->hit_count++;
        list_move_end(cache->items, element, cache->size);
        printf("HIT\n");
        return true;
    }

    // Always increment miss_count as we didn't find the element
    cache->miss_count++;
    printf("MISS\n");

    Node *new = new_node(element);

    if (list_is_full(cache->items, cache->size)) {
        // The cache is full, so increment capacity_misses
        if (hash_search(cache, element)) {
            // The element is in the hash table, so increment capacity_misses
            cache->capacity_misses++;
        } else {
            // The element is not in the hash table, so increment compulsory_misses and add to the hash table
            cache->compulsory_misses++;
            hash_insert(cache, element);
        }
        list_remove_front(cache->items, cache->size);
    } else {
        // The cache is not full, so increment compulsory_misses
        if (hash_search(cache, element)) {
            // The element is in the hash table, so it's a conflict miss (treated as a capacity miss)
            cache->capacity_misses++;
        } else {
            // The element is not in the hash table, so increment compulsory_misses and add to the hash table
            cache->compulsory_misses++;
            hash_insert(cache, element);
        }
    }

    // In either case, we add the new node to the cache
    list_push_back(cache->items, new, cache->size);

    return false;
}

bool insert_clock(Cache *cache, char *element) {
    if (list_contains(cache->items, element, cache->size)) {
        cache->hit_count++;
        Node *curr = list_get(cache->items, element, cache->size);
        curr->ref_bit = 1;
        printf("HIT\n");
        return true;
    }

    // Always increment miss_count as we didn't find the element
    cache->miss_count++;
    printf("MISS\n");

    // Prepare the new node here, as it will be needed in either case
    Node *new = new_node(element);

    if (list_is_full(cache->items, cache->size)) {
        // The cache is full, so increment capacity_misses
        if (hash_search(cache, element)) {
            // The element is in the hash table, so increment capacity_misses
            cache->capacity_misses++;
        } else {
            // The element is not in the hash table, so increment compulsory_misses and add to the hash table
            cache->compulsory_misses++;
            hash_insert(cache, element);
        }

        Node *curr = list_get_index(cache->items, cache->clock_ptr);
        while (curr->ref_bit == 1) {
            curr->ref_bit = 0;
            cache->clock_ptr = (cache->clock_ptr + 1) % cache->size;
            curr = list_get_index(cache->items, cache->clock_ptr);
        }
        // Replace the node here
        list_overwrite_index(cache->items, cache->clock_ptr, new);
    } else {
        // The cache is not full, so increment compulsory_misses
        if (hash_search(cache, element)) {
            // The element is in the hash table, so it's a conflict miss (treated as a capacity miss)
            cache->capacity_misses++;
        } else {
            // The element is not in the hash table, so increment compulsory_misses and add to the hash table
            cache->compulsory_misses++;
            hash_insert(cache, element);
        }
        // Add the new node to the cache
        list_push_back(cache->items, new, cache->size);
    }
    return false;
}

bool cache_insert(Cache *cache, char *element) {
    switch (cache->policy) {
    case 'F': return insert_fifo(cache, element);
    case 'L': return insert_lru(cache, element);
    case 'C': return insert_clock(cache, element);
    default: printf("Invalid cache policy.\n"); return false;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No arguments provided. Usage: ./cacher [-N size] <policy>\n");
        return 1;
    }
    char policy = 'F'; // Default policy
    int cache_size = CACHE_SIZE; // Default size, change to your default value

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-N", 2) == 0 && i + 1 < argc) {
            // -N is followed by size
            cache_size = atoi(argv[i + 1]);
            if (cache_size <= 0) {
                fprintf(stderr, "Invalid size for cache. Must be a positive integer.\n");
                return 1;
            }
            i++; // Skip size argument
        } else if (strcmp(argv[i], "-F") == 0) {
            policy = 'F';
        } else if (strcmp(argv[i], "-L") == 0) {
            policy = 'L';
        } else if (strcmp(argv[i], "-C") == 0) {
            policy = 'C';
        } else {
            fprintf(stderr,
                "Unknown option or policy: %s. Use -F for FIFO, -L for LRU, or -C for CLOCK.\n",
                argv[i]);
            return 1;
        }
    }

    Cache *cache = cache_new(policy, cache_size);

    char *item = (char *) malloc(sizeof(char) * MAX_ITEM);
    while (fgets(item, MAX_ITEM, stdin) != NULL) {
        cache_insert(cache, item);
        item = (char *) malloc(sizeof(char) * MAX_ITEM);
    }
    fprintf(stdout, "%d %d\n", cache->compulsory_misses, cache->capacity_misses);

    for (int i = 0; i < cache_size; i++) {
        if (cache->items[i] != NULL) {
            free(cache->items[i]->element); // Free the element string
            free(cache->items[i]);
        }
    }
    free(cache->items);
    // Free hash table
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode *node = cache->hashTable[i];
        while (node) {
            HashNode *tmp = node;
            node = node->next;
            free(tmp);
        }
    }
    free(cache->hashTable);
    free(cache);
    return 0;
}
