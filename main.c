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

Vector2 cameraTarget = {0};
Vector2 cameraOffset = {0};
float cameraZoom = 1;

Font customFont = {0};

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
    calculate_layout(root, 100, 1, horizontal_spacing, vertical_spacing);
}

void DrawNodeConnections(Person* node) {
    for (int i = 0; i < node->num_children; i++) {
        Person* child = node->children[i];
        // Draw line from parent to child
        float midPointY = (node->position.y + child->position.y)/2;


        DrawLineEx(node->position, (Vector2) {node->position.x, midPointY}, 2, DARKGRAY);
        DrawLineEx(
            (Vector2) {node->position.x, midPointY},
            (Vector2) {child->position.x, midPointY},
            2,
            DARKGRAY
        );
        DrawLineEx((Vector2) {child->position.x, midPointY}, child->position, 2, DARKGRAY);
        DrawNodeConnections(child);
    }
}

void DrawAllNodes(Person* node, float width, float height) {
    float textSize = MeasureText(node->name, width);
    DrawRectangleV( 
        (Vector2) {node->position.x - (textSize + textSize/5)/2, node->position.y - (height + height/5)/2}, 
        (Vector2) {(textSize + textSize/5), (height + height/5)}, 
        SKYBLUE
    );
    DrawTextEx(
        customFont,
        node->name,
        (Vector2) {node->position.x - textSize / 2, node->position.y - (height/2)},
        width,
        1,
        BLACK
    );

    for (int i = 0; i < node->num_children; i++) {
        DrawAllNodes(node->children[i], width, height);
    }
}

void CheckInput()
{
  float dt = 10;
  if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
    cameraTarget.y -= dt/cameraZoom;
  }
  if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
    cameraTarget.y += dt/cameraZoom;
  }
  if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
    cameraTarget.x -= dt/cameraZoom;
  }
  if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
    cameraTarget.x += dt/cameraZoom;
  }
  if (IsKeyPressed(KEY_F12)) {
    TakeScreenshot("screenshot.png");
  }
}

int main()
{
  HashTable *listOfPeople = create_hash_table(100); 
  float hspacing = 150;
  float vspacing = 200;
  parse_csv(listOfPeople, "test.csv");
  Person *root = find_person(listOfPeople, "John");
  layout_tree(root, hspacing, vspacing);


  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(1280, 720, "Family Tree");
  customFont = LoadFontEx("fonts/times.ttf", 64, NULL, 0);
  cameraTarget = root->position;
  cameraOffset = (Vector2) { GetScreenWidth()/2, GetScreenHeight()/2 };
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(WHITE);
    BeginMode2D((Camera2D) {
        .target = cameraTarget,
        .offset = cameraOffset,
        .rotation = 0,
        .zoom = cameraZoom
        });
    DrawNodeConnections(root);
    DrawAllNodes(root, hspacing/5, vspacing/5);
    CheckInput();
    EndDrawing();
  }
  UnloadFont(customFont);
  CloseWindow();
  free_hashtable(listOfPeople);
  return 0;
}

