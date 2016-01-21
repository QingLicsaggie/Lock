#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H
#define MAX_NAMESPACE_ID  32
/**
 * @file
 * @brief Lock Manager structures, macros and defines
 **/

/**
 *@brief Doubly Linked List definition
 **/

typedef struct list_head{
  /*
   * pointer to the next entry in the list
   */
  struct list_head *next;

  /*
   * pointer to the previous entry in the list
   */
  struct list_head *prev;
}list_head_t;


/*
 * @brief AVL tree node structure
 *
*/
typedef struct tree_node_s{
  union{
    /**
     * @brief List header for added nodes to the free list.
     */
    list_head_t list;
    
    /**
     * @brief List header for pending locks
     */
    list_head_t pendingList;
  };
  
  /**
   * @brief Pointer to this nodes parent node
   */
  struct tree_node_s *parent;

  /**
   * @brief Pointer to this node's children
   */
  struct tree_node_s *child[2];
  /*
   * @brief start LBA
   */
  unsigned int start_lba;
  
  /*
   * @brief end LBA
   */
  unsigned int end_lba;

  /*
   *@brief the height of this node
   */
  signed char height;
  /*
  * @brief ID of eventIndex
  */
  unsigned int eventIndex;
  /*
   * @brief Lock type, set to 1 if this is a write lock
   */
  unsigned char type;
}tree_node_t;

/**
 * @brief Return the larger of two signed values
 * 
 * @param[in] a Value to compare
 * @param[in] b Value to compare
 *
 * @reval The greater of a or b
 **/
#define MAX(a, b) ((a) >= (b) ? (a): (b))

/**
 * @brief: Extra the height from the specified tree node
 *
 * @param[in] p Pointer to the tree node
 *
 * @reval -1 The node pointer is NULL
 * @reval >=0 The height of the specified node
 **/
#define height(p)  ((p) != NULL ? (p)->height: -1)

/**
 * @brief Maximum number of supported tree nodes
 *
 **/
#define MAX_NODES 150

/**
 * @brief Array element index of the left child.
 */
#define LEFT 0

/**
 * @brief Array element index of the right child.
 */
#define RIGHT 1


/**
 * @brief definitions for return value of insertion
 *
 **/
enum NODE_INSERT_RESULT{
  NODE_ADDED,         //0 -- @brief The node was added to the tree successfully.
  NODE_COLLISION,     //1 -- @brief The node was not added to the tree due to a collision and the queue bit not being set.
  NODE_QUEUED,         //2 --@brief The node was queued beind an existing node.
  NODE_FAILED         //3 -- @brief The node was faied to insert
};


extern tree_node_t nodes[MAX_NODES];

extern list_head_t freeNodes;

void treeInit(void);

tree_node_t *allocNodes();

void freeNode(tree_node_t *node);

tree_node_t *rotateNode(tree_node_t **treeRoot, tree_node_t *subTreeRoot, int direction);

tree_node_t *rotateDouble(tree_node_t **treeRoot, tree_node_t *subTreeRoot, int direction);

void rebalance(tree_node_t **root, tree_node_t *newNode);

extern enum NODE_INSERT_RESULT insertNode(tree_node_t **root, tree_node_t *newNode, unsigned int canQueue);

void removeNode(tree_node_t **root, tree_node_t *node);

void obtainEventInexMarker(tree_node_t *root);
#endif//lock_manager.h
