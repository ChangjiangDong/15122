#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "./lib/xalloc.h"
#include "./lib/contracts.h"
#include "tst.h"
#include "queue.h"
#include "strbuf.h"

/* interface */

bool is_tst(tst t);
tst tst_new();
int tst_lookup(tst t, elem e);
void tst_insert(tst t, elem e);
void tst_free(tst t);

/******************************************************************************
* implementation
******************************************************************************/

/* predicate */
bool is_end_of_word(tree *root) {
  REQUIRES(root != NULL);

  return (*(char *)root->data == '\0');
}

int height(tree* root) {
  if (root == NULL) {
    return 0;
  } else {
    return root->height;
  }
}

// RETURNS: true iff all elements in root are in range (lower, upper);
bool is_ordered(tree* root, elem lower, elem upper, elem_compare_fn cmp) {
  if (root == NULL) {
    return true;
  } else {
    return root->data != NULL
      && (lower == NULL || cmp(lower, root->data) < 0)
      && (upper == NULL || cmp(root->data, upper) < 0)
      && is_ordered(root->left, lower, root->data, cmp)
      && is_ordered(root->right, root->data, upper, cmp)
      && is_ordered(root->middle, NULL, NULL, cmp);
  }
}

inline static int max(int m, int n) {
  return (m > n) ? m : n;
}

bool is_specified_height(tree* root) {
  if (root == NULL) {
    return true;
  } else {
    return root->height == max(height(root->left), height(root->right)) + 1
      && is_specified_height(root->left)
      && is_specified_height(root->right)
      && is_specified_height(root->middle);
  }
}

bool is_balanced(tree* root) {
  REQUIRES(is_specified_height(root));
  if (root == NULL) {
    return true;
  } else {
    return (abs(height(root->left) - height(root->right)) <= 1)
      && is_balanced(root->left) && is_balanced(root->right)
      && is_balanced(root->middle);
  }
}

bool is_valid_count(tree* root) {
  if (root == NULL) {
    return true;
  } else {
    return is_end_of_word(root) ? (root->count > 0) : (root->count == 0)
      && is_valid_count(root->left)
      && is_valid_count(root->middle)
      && is_valid_count(root->right)
      && is_valid_count(root->middle);
  }
}

bool is_tree(tree* root, elem_compare_fn cmp) {

  return is_ordered(root,NULL, NULL, cmp)
    && is_specified_height(root)
    && is_balanced(root)
    && is_valid_count(root);

  /*
  ASSERT(is_ordered(root,NULL, NULL, cmp));
  ASSERT(is_specified_height(root));
  ASSERT(is_balanced(root));
  ASSERT(is_valid_count(root));
  return true;
  */
}

bool is_tst(tst t) {
  return t != NULL && is_tree(t->root, t->cmp);
}

/* library */

int string_compare(elem e1, elem e2) {
  char* s1 = (char*) e1;
  char* s2 = (char*) e2;
  return (*s1) - (*s2);
}

/*
int char_compare(elem c1, elem c2) {
  return c1 - c2;
}
*/

tst tst_new() {
  tst T = (tst) xcalloc(sizeof(struct tst_header), 1);
  T->root = NULL;
  T-> cmp = &string_compare;
  //  T->cmp = &char_compare;

  ENSURES(is_tst(T));
  return T;
}

char * char_clone(char c) {
  char * buf = (char*) xcalloc(sizeof(char), 1);
  buf[0] = c;
  return buf;
}

void fix_height(tree* t) {
  REQUIRES(t != NULL);

  t->height = max(height(t->left), height(t->right)) + 1;

  return;
}

// GIVEN: a tree that ha been inserted a node on its right subtree
// RETURNS: the given tree after rotate right
tree* rotate_right(tree* t, elem_compare_fn cmp) {
  REQUIRES(t != NULL);
  REQUIRES(t->left != NULL);
  REQUIRES(is_tree(t->left, cmp));
  REQUIRES(is_tree(t->right, cmp));

  tree* left =t->left;
  t->left = left->right;
  left->right = t;
  fix_height(t);
  fix_height(left);

  ENSURES(left);
  return left;
}

// GIVEN: a tree that has been inserted a node on its left subtree
// RETURNS: the tree after rotate left
tree* rotate_left(tree* t, elem_compare_fn cmp) {
  REQUIRES(t != NULL);
  REQUIRES(t->right != NULL);
  REQUIRES(is_tree(t->left, cmp));
  REQUIRES(is_tree(t->right, cmp));

  tree* right = t->right;
  t->right = right->left;
  right->left = t;
  fix_height(t);
  fix_height(right);

  ENSURES(is_tree(right, cmp));
  return right;
}


tree* rebalance_right(tree* t, elem_compare_fn cmp) {
  REQUIRES(t != NULL);
  REQUIRES(t->right != NULL);
  REQUIRES(is_tree(t->left, cmp));
  REQUIRES(is_tree(t->right, cmp));

  if (height(t->right) - height(t->left) == 2) {
    if (height(t->right->right) > height(t->right->left)) {
      t = rotate_left(t, cmp);
    } else {
      ASSERT(height(t->right->right) < height(t->right->left));
      t->right = rotate_right(t->right, cmp);
      t = rotate_left(t, cmp);
    }
  } else {
    fix_height(t);
  }

  ENSURES(is_tree(t, cmp));
  return t;
}

// GIVEN: a tree that has been inserted a node on its left subtree
// RETURNS: the tree after rebalanced
tree* rebalance_left(tree* t, elem_compare_fn cmp) {
  REQUIRES(t != NULL);
  REQUIRES(t->left != NULL);
  REQUIRES(is_tree(t->left, cmp));
  REQUIRES(is_tree(t->right, cmp));

  if (height(t->left) - height(t->right) == 2) {
    tree* left = t->left;
    if (height(left->left) > height(left->right)) {
      t = rotate_right(t, cmp);
    } else {
      ASSERT(height(left->right) > height(left->left));
      t->left = rotate_left(left, cmp);
      t = rotate_right(t, cmp);
    }
  } else {
    fix_height(t);
  }

  ENSURES(is_tree(t, cmp));
  return t;
}


// WHERE: strlen(s) > 0;
// RETURNS: the given tree after insert s
tree* tree_insert(tree* t, char* s, elem_compare_fn cmp) {
  REQUIRES(is_tree(t, cmp));

  if (t == NULL) {
    t = (tree*) xcalloc(sizeof(tree), 1);
    t->data = char_clone(s[0]);
    t->height = 1;
    t->left = NULL;
    t->middle = NULL;
    t->right = NULL;
    if (*s == '\0') {
      t->count = 1;
    } else {
      t->count = 0;
      t->middle = tree_insert(t->middle, &s[1], cmp);
    }
    return t;
  } else {
    if (*s == '\0') {
      if (is_end_of_word(t)) {
        t->count++;
        return t;
      } else {
        return tree_insert(NULL, s, cmp);
      }
    }

    int r = cmp(s, t->data);
    if (r == 0) {
      if (*s == '\0') {
        t->count++;
      } else {
        t->middle = tree_insert(t->middle, &s[1], cmp);
      }
    } else if (r < 0) {
      t->left = tree_insert(t->left, s, cmp);
      t = rebalance_left(t, cmp);
      //      fix_height(t);
      //rebalance(t);
    } else {
      ASSERT(r > 0);
      t->right = tree_insert(t->right, s, cmp);
      t = rebalance_right(t, cmp);
      //      fix_height(t);
    }
  }

  ENSURES(is_tree(t, cmp));
  return t;
}

void tst_insert(tst T, elem e) {
  REQUIRES(is_tst(T) && e != NULL);

  if (strlen((char*) e) == 0) {
    return;
  }

  T->root = tree_insert(T->root, (char*) e, T->cmp);

  ENSURES(is_tst(T));
}


// WHERE: strlen(s) > 0;
int tree_lookup(tree* root, char* s, elem_compare_fn cmp) {
  REQUIRES(is_tree(root, cmp));
  REQUIRES(s != NULL);

  if (root == NULL) {
    return 0;
  } else {
    int r = cmp(s, root->data);
    if (r == 0) {
      if (*s == '\0') {
        return root->count;
      } else {
        ASSERT(strlen(s) > 0);
        return tree_lookup(root->middle, &s[1], cmp);
      }
    } else if (r < 0) {
      return tree_lookup(root->left, s, cmp);
    } else {
      ASSERT(r > 0);
      return tree_lookup(root->right, s, cmp);
    }
  }
}

int tst_lookup(tst T, elem e) {
  REQUIRES(is_tst(T));

  if (strlen((char*) e) == 0) {
    return 0;
  }
  return tree_lookup(T->root, (char*) e, T->cmp);
}

void tree_free(tree* t) {

  if (t == NULL) {
    return;
  } else {
    free(t->data);
    tree_free(t->left);
    tree_free(t->middle);
    tree_free(t->right);
    free(t);
  }
}

void tst_free(tst T) {
  REQUIRES(is_tst(T));

  tree_free((tree*) T->root);
  free(T);
  return;
}


/******************************************************************************/
// Ex 3 of lec 21

// WHERE: 0 <= start && start <= strlen(s);
// EFFECT: for each member m of tn that matches s[start,)
// add the concatenation of sb and m to q
// DETAILS: the sb should have the same string as before afetr
// the function returns

void tree_lookup_generalized(tree *tn, char *s, size_t start,
                             struct strbuf *sb, Queue q, elem_compare_fn cmp) {
  REQUIRES(is_tree(tn, cmp));
  REQUIRES(s != NULL);
  REQUIRES(start <= strlen(s));
  REQUIRES(is_strbuf(sb));
  REQUIRES(is_Queue(q));

  if (tn == NULL) return;

  if (s[start] == '*') {
    tree_lookup_generalized(tn->left, s, start, sb, q, cmp);
    tree_lookup_generalized(tn->right, s, start, sb, q, cmp);

    if (is_end_of_word(tn)) {
      tree_lookup_generalized(tn, s, start + 1, sb, q, cmp);
      return;
    } else {
      tree_lookup_generalized(tn, s, start + 1, sb, q, cmp);
      strbuf_push(sb, *(char *)tn->data);
      tree_lookup_generalized(tn->middle, s, start + 1, sb, q, cmp);
      // tree_lookup_generalized(tn->middle, s, start, sb, q, cmp);
      strbuf_pop(sb);
      return;
    }
  } else {
    int r = cmp(&s[start], tn->data);
    if (r == 0) {
      if (s[start] == '\0') {
        Queue_insert(q, strbuf_str(sb));
        return;
      } else {
        strbuf_push(sb, s[start]);
        tree_lookup_generalized(tn->middle, s, start + 1, sb, q, cmp);
        strbuf_pop(sb);
        return;
      }
    } else if (r < 0) {
      tree_lookup_generalized(tn->left, s, start, sb, q, cmp);
      return;
    } else {
      ASSERT(r > 0);
      tree_lookup_generalized(tn->right, s, start, sb, q, cmp);
      return;
    }
  }
  /*
    if (s[start + 1] == '\0' && tn->is_end_of_word) {
      strbuf_push(sb, *(char*)tn->data);
      Queue_insert(q, strbuf_str(sb));
      tree_lookup_generalized(tn->middle, s, start, sb, q, cmp);
      strbuf_pop(sb);
      return;
    } else if (s[start + 1] == '\0') {
      tree_lookup_generalized(tn->middle, s, start, sb, q, cmp);
    } else {
      ASSERT(s[start + 1] != '\0');

      strbuf_push(sb, *(char*)tn->data);
      tree_lookup_generalized(tn->middle, s, start, sb, q, cmp);
      tree_lookup_generalized(tn->middle, s, start + 1, sb, q, cmp);
      strbuf_pop(sb);
    }
  } else {
    if (tn == NULL) return;
    int r = cmp(&s[start], tn->data);
    if (r < 0) {
      tree_lookup_generalized(tn->left, s, start, sb, q, cmp);
      return;
    } else if (r > 0) {
      tree_lookup_generalized(tn->right, s, start, sb, q, cmp);
      return;
    } else {
      strbuf_push(sb, *(char*)tn->data);
      if (s[start+1] == '\0' && tn->is_end_of_word) {
        Queue_insert(q, strbuf_str(sb));
      } else if (s[start + 1] != '\0') {
        tree_lookup_generalized(tn->middle, s, start + 1, sb, q, cmp);
      }
      strbuf_pop(sb);
      return;
    }
  }
  */
  ENSURES(is_Queue(q));
}

// DETAILS: s might contains '*' which representa arbitrary number of arbitrary
// character
// EFFECT: add all members in the given T that match the given string
// s to q
void tst_member_generalized(tst T, char *s, Queue q) {
  REQUIRES(is_tst(T));
  REQUIRES(s != NULL);
  REQUIRES(is_Queue(q));

  struct strbuf *sb = strbuf_new(8);
  tree_lookup_generalized(T->root, s, 0, sb, q, T->cmp);
  free(strbuf_dealloc(sb));

  ENSURES(is_Queue(q));
  return;
}
