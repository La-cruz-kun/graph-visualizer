#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 50
#define MAX_CHILDREN 20
#define MAX_QUEUE_SIZE 1000


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

typedef struct {
  Person **items;
  int front;
  int rear;
} Queue;

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
  
  if (!(ht->entries[nameHash])) {
      printf("no such person\n");
      return NULL;
  }
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

    Person *parent;
    if (find_person(ht, parent_name))
      parent = find_person(ht, parent_name);
    else
      parent = create_person(ht, parent_name);

    while (true) {
      char *child_name = strtok(NULL, ",\n");
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
    printf("name: %s, parent: %s\n", ht->entries[i]->name, ht->entries[i]->parent->name);
  }
}

void free_hashtable(HashTable *ht)
{
  for (int i = 0; i < 100; i++) {
    if (ht->entries[i]) {
      free(ht->entries[i]->children);
      free(ht->entries[i]);
    }
  }
  free(ht->entries);
  free(ht);
}

Queue *create_queue()
{
  Queue *q = malloc(sizeof(Queue));
  q->items = malloc(MAX_QUEUE_SIZE * sizeof(Person *));
  q->front = -1;
  q->rear = -1;
  return q;
}

void enqueue(Queue *q, Person *p)
{
  if (q->rear == MAX_QUEUE_SIZE - 1) {
    printf("queue is full\n");
    return;
  }
  if (q->front == - 1) {
    q->front = 0;
  }
  q->rear++;
  q->items[q->rear] = p;
}

Person *dequeue(Queue *q)
{
  if (q->front == -1 || q->front > q->rear) {
    printf("Queue is full\n");
    return NULL;
  }
  Person *p = q->items[q->front];
  q->front++;
  if (q->front > q->rear) {
    q->front = q->rear = -1;
  }
  return p;
}

bool is_empty(Queue *q)
{
  return (q->front == -1);
}

void bfs_traversal(Person *root)
{
  if (!(root)) {
    printf("Tree is empty\n");
    return;
  }

  Queue *q = create_queue();
  enqueue(q, root);

  
  while (!is_empty(q)) {
    Person *current = dequeue(q);
    printf("Visited: %s\n", current->name);
    int enqueue_val = 0;

    for (int i = 0; i < current->num_children; i++) {
      enqueue(q, current->children[i]);
      enqueue_val++;
    }
    printf("enqueued %i\n", enqueue_val);
  }
  free(q->items);
  free(q);
}

void print_entries_per_depth(Person* root) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }

    Queue* q = create_queue();
    enqueue(q, root);
    int depth = 0;

    while (!is_empty(q)) {
        // Number of nodes at the current depth
        int nodes_at_current_depth = q->rear - q->front + 1;
        printf("Depth %d: %d entries\n", depth, nodes_at_current_depth);

        // Process all nodes at this depth
        for (int i = 0; i < nodes_at_current_depth; i++) {
            Person* current = dequeue(q);

            // Enqueue children for the next depth
            for (int j = 0; j < current->num_children; j++) {
                enqueue(q, current->children[j]);
            }
        }

        depth++; // Move to the next depth
    }

    // Cleanup
    free(q->items);
    free(q);
}

typedef struct {
    float x;      // Node's x-coordinate
    float y;      // Node's y-coordinate
    float left;   // Leftmost boundary of the subtree
    float right;  // Rightmost boundary of the subtree
} SubtreeLayout;

SubtreeLayout calculate_layout(
    Person* node,
    float x_offset,          // Starting x-offset for this subtree
    int depth,               // Current depth (root = 0)
    float horizontal_spacing,
    float vertical_spacing
) {
    SubtreeLayout layout = {0};

    if (!node) return layout;

    // Leaf node: place at current x_offset
    if (node->num_children == 0) {
        node->position.x = x_offset;
        node->position.y = depth * vertical_spacing;
        layout.x = x_offset;
        layout.left = x_offset;
        layout.right = x_offset;
        return layout;
    }

    // Process children recursively
    SubtreeLayout* children_layouts = malloc(node->num_children * sizeof(SubtreeLayout));
    float total_children_width = 0;

    for (int i = 0; i < node->num_children; i++) {
        children_layouts[i] = calculate_layout(
            node->children[i],
            x_offset + total_children_width,
            depth + 1,
            horizontal_spacing,
            vertical_spacing
        );
        total_children_width += (children_layouts[i].right - children_layouts[i].left) + horizontal_spacing;
    }

    // Remove extra spacing after last child
    total_children_width -= horizontal_spacing;

    // Center the parent above its children
    layout.x = x_offset + total_children_width / 2;
    layout.left = children_layouts[0].left;
    layout.right = children_layouts[node->num_children - 1].right;

    // Assign position to the current node
    node->position.x = layout.x;
    node->position.y = depth * vertical_spacing;

    free(children_layouts);
    return layout;
}

void layout_tree(Person* root, float horizontal_spacing, float vertical_spacing) {
    if (!root) return;
    calculate_layout(root, 0, 0, horizontal_spacing, vertical_spacing);
}

int main()
{
  ulong hashed = hash("John");
  printf("%lu\n", hashed);
  HashTable *listOfPeople = create_hash_table(100); 

  parse_csv(listOfPeople, "test.csv");
  print_entries_per_depth(find_person(listOfPeople, "John"));
  free_hashtable(listOfPeople);
  return 0;
}

