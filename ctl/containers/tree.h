#include <stddef.h>
#include <stdbool.h>

#if !defined(_TREE_INCLUDED)
#   define _TREE_INCLUDED
#   if !defined(_CONCAT3)
#       define _CONCAT3_(a, b, c) a ## _ ## b ## _ ## c
#       define _CONCAT3(a, b, c) _CONCAT3_(a, b, c)
#   endif

/* --------- PUBLIC API ---------- */

// Include Parameters:
//  The type of the elements the tree should contain:
//      #define REQ_PARAM_type                  T
//
//  The number of children the tree can have:
//      #define REQ_PARAM_children              N
//
//  An allocator function (obeying ISO C's malloc/calloc semantics):
//      Default: malloc
//      #define OPT_PARAM_malloc(bytes)        malloc(bytes)
//
//  A reallocator function (obeying ISO C's realloc semantics):
//      Default: realloc
//      #define OPT_PARAM_realloc(ptr, bytes)  realloc(ptr, bytes)
//
//  A free function (obeying ISO C's free semantics):
//      Default: free
//      #define OPT_PARAM_free(ptr)            free(ptr)

// Functions:
//  The tree type
#define TreeNode(T, N)              _CONCAT3(_TreeNode, T, N)

// Create a new stack
//  TreeNode(T, N)* Tree_New()
#define Tree_New(T, N)              _CONCAT3(_Tree_New, T, N)

// Delete a tree node
//  void Tree_Delete(TreeNode(T, N)* node)
#define Tree_DeleteNode(T, N)       _CONCAT3(_Tree_DeleteNode, T, N)

// Recursively delete a tree
//  void Tree_DeleteTree(TreeNode(T, N)* root)
#define Tree_DeleteTree(T, N)       _CONCAT3(_Tree_DeleteTree, T, N)

// Add node child to node parent
//  ssize_t Tree_AddChild(TreeNode(T, N)* parent, TreeNode(T, N)* child)
#define Tree_AddChild(T, N)         _CONCAT3(_Tree_AddChild, T, N)

// Remove a child from a parent
//  ssize_t Tree_RemoveChild(TreeNode(T, N)* parent, TreeNode(T, N)* child)
#define Tree_RemoveChild(T, N)      _CONCAT3(_Tree_RemoveChild, T, N)



/* --------- END PUBLIC API ---------- */
#endif

#if !defined (REQ_PARAM_type)
#   error "Tree requires a type specialization"
#endif

#if !defined (REQ_PARAM_children)
#   error "Tree requires a children specialization"
#endif

#if !defined (OPT_PARAM_malloc)
#   if !defined (_DEFAULT_ALLOCATOR)
#       define _DEFAULT_ALLOCATOR
#   endif

#   define OPT_PARAM_malloc(bytes) calloc(1, bytes)
#endif

#if !defined (OPT_PARAM_realloc)
#   if !defined (_DEFAULT_ALLOCATOR)
#       warning "Non-default malloc used with default realloc"
#   endif

#   define OPT_PARAM_realloc realloc
#endif

#if !defined (OPT_PARAM_free)
#   if !defined (_DEFAULT_ALLOCATOR)
#       warning "Non-default malloc used with default free"
#   endif

#   define OPT_PARAM_free free
#endif

#if defined(_DEFAULT_ALLOCATOR)
#   include <stdlib.h>
#endif

// useful macros internally

#define T REQ_PARAM_type
#define N REQ_PARAM_children
#define malloc OPT_PARAM_malloc
#define realloc OPT_PARAM_realloc
#define free OPT_PARAM_free

// data types

typedef struct TreeNode(T, N) {
    struct TreeNode(T, N)* parent;
    struct TreeNode(T, N)* child[N];
    T val;
} TreeNode(T, N);

// functions

TreeNode(T, N)* Tree_New(T, N)(void)
{
    TreeNode(T, N)* node = malloc(sizeof(TreeNode(T, N)));
    return node;
}

void Tree_DeleteNode(T, N)(TreeNode(T, N)* node)
{
    free(node);
}

void Tree_DeleteTree(T, N)(TreeNode(T, N)* root)
{
    for (size_t ii = 0; ii < N; ++ii) {
        Tree_DeleteNode(T, N)(root->child[ii]);
    }
    Tree_DeleteNode(T, N)(root);
}

ssize_t Tree_AddChild(T, N)(TreeNode(T, N)* parent, TreeNode(T, N)* child)
{
    for (size_t ii = 0; ii < N; ++ii) {
        if (parent->child[ii] == NULL) {
            parent->child[ii] = child;
            return ii;
        }
    }

    return -1;
}

ssize_t Tree_RemoveChild(T, N)(TreeNode(T, N)* parent, TreeNode(T, N)* child)
{
    for (size_t ii = 0; ii < N; ++ii) {
        if (parent->child[ii] == child) {
            parent->child[ii] = NULL;
            return ii;
        }
    }

    return -1;
}

// cleanup macros
#undef T
#undef N
#undef malloc
#undef realloc
#undef free

#undef _DEFAULT_ALLOCATOR

#undef REQ_PARAM_type
#undef OPT_PARAM_children
#undef OPT_PARAM_malloc
#undef OPT_PARAM_realloc
#undef OPT_PARAM_free
