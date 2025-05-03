#include <avl_tree.h>
#include <malloc.h>
#include <util.h>

// AVL Tree implementation. As much as I hate recursion, there's a lot of it
// Copyright (C) 2025 Panagiotis

// here so they're not global definitions
#define CALC_HEIGHT(node) ((node) ? (node)->height : 0)
#define CALC_BALANCE(node)                                                     \
  ((node) ? CALC_HEIGHT((node)->left) - CALC_HEIGHT((node)->right) : 0)

AVLheader *AVLAllocateNode(avlkey key) {
  AVLheader *node = calloc(sizeof(AVLheader), 1);
  node->key = key;
  node->height = 1; // sane default
  return node;
}

#define COUNT 10
void AVLDebug(AVLheader *root, int space) {
  // Base case
  if (!root)
    return;

  // Increase distance between levels
  space += COUNT;

  // Process right child first
  AVLDebug(root->right, space);

  // Print current node after space
  // count
  printf("\n");
  for (int i = COUNT; i < space; i++)
    printf(" ");
  printf("%ld\n", root->key);

  // Process left child
  AVLDebug(root->left, space);
}

AVLheader *AVLRotationsRight(AVLheader *y) {
  AVLheader *x = y->left;
  AVLheader *T2 = x->right;

  x->right = y;
  y->left = T2;

  y->height = MAX(CALC_HEIGHT(y->left), CALC_HEIGHT(y->right)) + 1;
  x->height = MAX(CALC_HEIGHT(x->left), CALC_HEIGHT(x->right)) + 1;

  return x;
}

AVLheader *AVLRotationsLeft(AVLheader *x) {
  AVLheader *y = x->right;
  AVLheader *T2 = y->left;

  y->left = x;
  x->right = T2;

  x->height = MAX(CALC_HEIGHT(x->left), CALC_HEIGHT(x->right)) + 1;
  y->height = MAX(CALC_HEIGHT(y->left), CALC_HEIGHT(y->right)) + 1;

  return y;
}

AVLheader *AVLMinKeyNode(AVLheader *start) {
  AVLheader *browse = start;
  while (browse && browse->left)
    browse = browse->left;
  return browse;
}

AVLheader *AVLAllocateL(AVLheader *node, avlkey key, AVLheader **target) {
  /* 1. BST insertion */
  if (!node) {
    AVLheader *actual = AVLAllocateNode(key);
    assert(!(*target));
    *target = actual;
    return actual;
  }

  if (key < node->key)
    node->left = AVLAllocateL(node->left, key, target);
  else if (key > node->key)
    node->right = AVLAllocateL(node->right, key, target);
  else {
    // todo: maybe it will be better to do *target = -1 in the future
    debugf("[data_structures::avl] Tried adding a duplicate! key{%lx}\n", key);
    panic();
  }

  /* 2. update our height */
  node->height = 1 + MAX(CALC_HEIGHT(node->left), CALC_HEIGHT(node->right));

  /* 3. use bf to tell if/how it became unbalanced */
  int balance = CALC_BALANCE(node);

  // Left Left Case
  if (balance > 1 && key < node->left->key)
    return AVLRotationsRight(node);

  // Right Right Case
  if (balance < -1 && key > node->right->key)
    return AVLRotationsLeft(node);

  // Left Right Case
  if (balance > 1 && key > node->left->key) {
    node->left = AVLRotationsLeft(node->left);
    return AVLRotationsRight(node);
  }

  // Right Left Case
  if (balance < -1 && key < node->right->key) {
    node->right = AVLRotationsRight(node->right);
    return AVLRotationsLeft(node);
  }

  /* return the (unchanged) node pointer */
  return node;
}

AVLheader *AVLUnregisterL(AVLheader *root, avlkey key, AVLheader **target) {
  /* 1. BST delete */
  if (!root)
    return root;

  if (key < root->key)
    root->left = AVLUnregisterL(root->left, key, target);
  else if (key > root->key)
    root->right = AVLUnregisterL(root->right, key, target);
  else {
    // node with only one child or no child
    if (!root->left || !root->right) {
      AVLheader *temp = root->left ? root->left : root->right;

      // No child case
      if (!temp) {
        temp = root;
        root = 0;
      } else                                   // One child case
        memcpy(root, temp, sizeof(AVLheader)); // Copy the contents of
                                               // the non-empty child
      assert(!(*target));
      *target = temp;
      // free(temp);
    } else {
      // node with two children: Get the inorder
      // successor (smallest in the right subtree)
      AVLheader *temp = AVLMinKeyNode(root->right);

      // Copy the inorder successor's data to this node
      root->key = temp->key;
      root->value = temp->value;
      // Delete the inorder successor
      root->right = AVLUnregisterL(root->right, temp->key, target);
    }
  }

  // If the tree had only one node then return
  if (!root)
    return root;

  /* 2. update our height */
  root->height = 1 + MAX(CALC_HEIGHT(root->left), CALC_HEIGHT(root->right));

  /* 3. use bf to tell if/how it became unbalanced */
  int balance = CALC_BALANCE(root);

  // Left Left Case
  if (balance > 1 && CALC_BALANCE(root->left) >= 0)
    return AVLRotationsRight(root);

  // Left Right Case
  if (balance > 1 && CALC_BALANCE(root->left) < 0) {
    root->left = AVLRotationsLeft(root->left);
    return AVLRotationsRight(root);
  }

  // Right Right Case
  if (balance < -1 && CALC_BALANCE(root->right) <= 0)
    return AVLRotationsLeft(root);

  // Right Left Case
  if (balance < -1 && CALC_BALANCE(root->right) > 0) {
    root->right = AVLRotationsRight(root->right);
    return AVLRotationsLeft(root);
  }

  return root;
}

void *AVLAllocate(void **AVLfirstPtr, avlkey key, avlval value) {
  AVLheader *target = 0;
  *AVLfirstPtr = AVLAllocateL(*AVLfirstPtr, key, &target);
  assert(target);
  target->value = value;
  return target;
}

bool AVLUnregister(void **AVLfirstPtr, avlkey key) {
  AVLheader *target = 0;
  *AVLfirstPtr = AVLUnregisterL(*AVLfirstPtr, key, &target);
  return !!target;
}

bool AVLFree(void **AVLfirstPtr, avlkey key) {
  AVLheader *target = 0;
  *AVLfirstPtr = AVLUnregisterL(*AVLfirstPtr, key, &target);
  if (!target)
    return false;
  free(target);
  return true;
}

avlval AVLLookup(void *raw, avlkey key) {
  AVLheader *root = raw;
  if (!root)
    return 0;
  if (key > root->key)
    return AVLLookup(root->right, key);
  else if (key < root->key)
    return AVLLookup(root->left, key);
  return root->value;
}
