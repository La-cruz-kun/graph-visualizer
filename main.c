#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 50
#define MAX_CHILDREN 20


typedef unsigned long ulong;
typedef struct Person {
  char name[MAX_NAME_LEN];
  struct Person *parent;
  struct Person **children;
  int num_children;
  Vector2 position;
} Person;

typedef struct {
  Person** entries;
  size_t size;
} HashTable;

ulong hash(char *str)
{
  ulong hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash;
}

HashTable *create_hash_table(size_t size)
{
  HashTable *ht = malloc(sizeof(HashTable));
  ht->size = size;
  ht->entries = calloc(size, sizeof(Person*));
  return ht;
}

Person *find_person(HashTable *ht, char *name)
{
  ulong nameHash = hash(name) % ht->size;
  
  if (strcmp(ht->entries[nameHash]->name, name) == 0)
    return ht->entries[nameHash];
  printf("could't find person %s\n", name);
  return NULL;
}

Person *create_person(HashTable *ht, char *name)
{
  ulong nameHash = hash(name) % ht->size;

  Person *person = malloc(sizeof(Person));
  if (!person) {
    perror("unable to allocate memeory for Person struct");
    return NULL;
  }
  strncpy(person->name, name, MAX_NAME_LEN);
  person->name[MAX_NAME_LEN - 1] = '\n';

  person->num_children = 0;
  person->children = NULL;
  person->parent = NULL;
  person->position = (Vector2) {0};
  ht->entries[nameHash] = person;
  return person;
}

void parse_csv(HashTable *ht, const char *filename) 
{
  FILE *file = fopen(filename, "r");
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    char *parent_name = strtok(line, ",");

    Person *parent = create_person(ht, parent_name);

    while (true) {
      char *child_name = strtok(NULL, "\n");
      if (child_name == NULL)
        break;
      Person *child = create_person(ht, child_name);
      child->parent = parent;

      parent->children = realloc(parent->children, (parent->num_children + 1) * sizeof(Person *));
      parent->children[parent->num_children++] = child;
    }
  }
  fclose(file);
}
      


void display_hashtable(HashTable *ht)
{
  for (int i = 0; i < 100; i++) {
    if (ht->entries[i] == NULL)
      continue;
    printf("%s\n", ht->entries[i]->name);
  }
}

void destroy_hashtable(HashTable *ht)
{
  free(ht->entries);
  free(ht);
}

int main()
{
  ulong hashed = hash("John");
  printf("%lu\n", hashed);
  HashTable *listOfPeople = create_hash_table(100); 

  parse_csv(listOfPeople, "test.csv");
  display_hashtable(listOfPeople);
  return 0;
}

