#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include"lock_manager.h"

/**
 * @brief The root node of the trees for each namespace.
 */
tree_node_t *rootArray[MAX_NAMESPACE_ID ] = { NULL };
/**
 * @brief Tree node array.
 * 
 * @note This may need to be dynamic based on the number of flash targets.
 */
tree_node_t   nodes[MAX_NODES];
unsigned int  tree_array[MAX_NODES + 1];
unsigned int  next_index[MAX_NAMESPACE_ID];
list_head_t   freeNodes;
unsigned int  allocated;

static inline void listInsert(list_head_t *old, list_head_t *new)
{
    new->next = old;
    new->prev = old->prev;
    new->next->prev = new;
    new->prev->next = new;  
}

#define listInit(list)              (list)->next = (list)->prev = (list)
#define listAddHead(list, new)      listInsert((list)->next, new)
#define listAddTail(list, new)      listInsert(list, new)
#define listEmpty(list)             ((list)->next == (list))
#define listGetHead(list, type)     ((type *) ((list)->next))
#define lowbit(i)                   ((i)&(-i))

static inline void listDel(list_head_t * entry) 
{
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
  listInit(entry);
}

/**
 * @brief Initialize the tree data structures.
 * zero out the tree node structure and add them 
 *
 * @retval N/A
 **/
void treeInit(void){
  unsigned int n;
  /*
   * Clear the memory used for the tree nodes, root array and namespace lock activation.
   */
  memset(nodes, 0, sizeof(nodes));
  memset(rootArray, 0, sizeof(rootArray));
  memset(next_index, 0, sizeof(next_index));
  /*
   * Initialize the list of free tree nodes.
   */
  listInit(&freeNodes);
  for (n=0; n<MAX_NODES; n++)
    listAddHead(&freeNodes, &nodes[n].list);
}


/*
 * @brief Allocate a new tree node
 *
 * This function removes an entry from the head of the freeNodes list.
 *
 * @retval Pointer to a new node if the allocation was successful, NULL if 
 * there are no nodes on the free list
 */
tree_node_t *allocNodes(void){
  tree_node_t *node = NULL;
  /*
   * If the list is not empty we should remove the head item.
  */
  if (!listEmpty(&freeNodes))
    {
     /*
      * Get the entry from the head of this list.
      */
      node = listGetHead(&freeNodes, tree_node_t);

      /*
       * Removed it from the list.
       */
      listDel(&node->list);

      allocated++;
      //printf("%d allocated \n", allocated);
    }
    else
      printf("Out of nodes!\n");

    return (node);
}

/**
 * @brief Free a node previously allocated with #allocNode
 * 
 * Add the specified node to the head of #freeNodes list
 *
 * @param[in] node Pointer to the tree node to be freed.
 *
 * @retval N/A
 **/
void freeNode(tree_node_t *node){
  /**
   * Empty out the fields that we want to free
   *
   **/
  node->child[LEFT] = node->child[RIGHT] = node->parent = NULL;
  node->height = 0;
  node->start_lba = -1;
  node->end_lba   = -1;
  listInit(&node->list);
  listAddTail(&freeNodes, &node->list);
  allocated--;
  //printf("%d allocated \n", allocated);
}
/**
 * @brief Perform a rotation
 *
 * @param[in] treeRoot -- pointer to the tree root pointer
 * @param[in] subTreeRoot -- Pointer to the subtree root where the rotaton takes place 
 * @param[in] direction  -- Direction to rotate, 0 = left, 1 = right.
 *
 * @return Pointer to the new subtree root
 */
tree_node_t *rotateNode(tree_node_t **treeRoot, tree_node_t *subTreeRoot, int direction){
  /**
   * do the rotation
   */
  tree_node_t *pivot = subTreeRoot->child[!direction];
  subTreeRoot->child[!direction] = pivot->child[direction];
  pivot->child[direction] = subTreeRoot;
  
  /**
   * update the parent: totally we have to update three parents
   *
   **/
  if(subTreeRoot->child[!direction])
    subTreeRoot->child[!direction]->parent = subTreeRoot;
    
    pivot->parent = subTreeRoot->parent;
    subTreeRoot->parent = pivot;
  
    if(pivot->parent){
      if(pivot->parent->child[RIGHT]==subTreeRoot)
	pivot->parent->child[RIGHT] = pivot;
      else
	pivot->parent->child[LEFT] = pivot;
    }
    
    /**
     * update the height factor
     *
     **/
    subTreeRoot->height = MAX(height(subTreeRoot->child[LEFT]), height(subTreeRoot->child[RIGHT])) + 1;
    pivot->height = MAX(height(pivot->child[LEFT]), height(pivot->child[RIGHT])) + 1;
    
    /*
     * check for a new tree root
     */
    if(!pivot->parent)
      *treeRoot = pivot;
    
    return pivot;
}


/**
 * @brief Perform a double rotation
 *
 * @param[in] treeRoot -- Pointer to  the root pointer
 * @param[in] subTreeRoot -- pointer to the subtree root where the rotate takes place.
 * @param[in] direction   -- Diection to rotate, LEFT or Right
 **/
tree_node_t *rotateDouble(tree_node_t **treeRoot, tree_node_t *subTreeRoot, int direction){
  subTreeRoot->child[!direction] = rotateNode(treeRoot, subTreeRoot->child[!direction], !direction);
  return rotateNode(treeRoot, subTreeRoot, direction);
}


/**
 *
 * This function traverses the tree from newNode to the root updating the
 * height factors for each node along the way. If the height of the left and
 * the right branches of the nodes are different by 2 or more, a rotation takes 
 * place to rebalance the tree
 **/
void rebalance(tree_node_t **root, tree_node_t *newNode){
  int val;
  
  /*
   * Update the height factor ...
   */
  for(; newNode; newNode = newNode->parent){
    /*
     * If the balance has not changed from this point we can stop
     */
    if( (val = MAX(height(newNode->child[LEFT]), height(newNode->child[RIGHT])) + 1) == newNode->height)
      break;
   
    newNode->height = val;
    
    /*
     * Right path is longer than the left path
     */
    if ((height(newNode->child[RIGHT]) - height(newNode->child[LEFT])) >= 2){
      /*
       * The right path on the right child is longer than the left path on the right child
       */
      if( height(newNode->child[RIGHT]->child[RIGHT]) >= height(newNode->child[RIGHT]->child[LEFT]))
	newNode = rotateNode(root, newNode, LEFT);
      else
	newNode = rotateDouble(root, newNode, LEFT);
      
      break;
    }
    else if((height(newNode->child[RIGHT])) - height(newNode->child[LEFT]) <= -2){
      if(height(newNode->child[LEFT]->child[RIGHT]) <= height(newNode->child[LEFT]->child[LEFT]))
	newNode = rotateNode(root, newNode, RIGHT);
      else
	newNode = rotateDouble(root, newNode, RIGHT);
      break;
    }
  }
}

/**
 * @brief  insert elem into the head in the order of event index
 *
 * @param[in] head -- the link list head 
 * @param[in] elem -- the elem to insert
 *
 * @retval N/A
 **/

static inline void listAddInorder(tree_node_t *head, tree_node_t *elem){
  tree_node_t *iter = head;
  tree_node_t *prev = NULL;
  if(listEmpty(&head->pendingList)){
    head->pendingList.next = &elem->list;
    elem->list.prev        = &head->pendingList;

    elem->list.next        = &head->pendingList;
    head->pendingList.prev = &elem->list;
    return;
  }
  else{
      prev = iter;
      iter = listGetHead(&iter->list, tree_node_t);
      
      if(iter->eventIndex > elem->eventIndex){
	listAddTail(&iter->list, &elem->list);
	return;
      }

      while(iter != head && iter->eventIndex < elem->eventIndex){
	prev = iter;
	iter = listGetHead(&iter->list, tree_node_t);	
      }
      
      prev->list.next = &elem->list;
      elem->list.prev = &prev->list;
      
      iter->list.prev = &elem->list;
      elem->list.next = &iter->list;
  }  
}

/**
 * @brief Insert a node into the tree
 *
 * This function is called to insert a node in to the AVL tree.
 *
 * @param[in] root -- Pointer to the root pointer
 * @param[in] newNode -- Pointer to the node to be added.
 * @param[in] canQueue -- Specify if the lock request should be queued if it could not be granted.
 * 
 *
 * @retval #NODE_ADDED -- The node was added to the tree successfully.
 * @retval #NODE_QUEUED -- The node was added to the pending list of an existing node in the tree
 * @retval #NODE_COLLISION -- The node was not added to the tree due to a collision and the queue bit not being set
 */

enum NODE_INSERT_RESULT insertNode(tree_node_t **root, tree_node_t *newNode, unsigned int canQueue){ 
  if(*root != NULL){
    unsigned int direction;
    tree_node_t *prev = NULL;
    tree_node_t *iter = *root;
    while(iter != NULL){
      prev = iter;
      /*
       * Range check against the node in the tree and all of its pending list
       */
      do{
	/*
	 * check for collision
	 */
	if(newNode->start_lba <= iter->end_lba && newNode->end_lba >= iter->start_lba){
	  if(canQueue){
	    /*
	     * insert the node to the pending list in the order of event index
	     */	 
	    listAddInorder(prev, newNode);
	    printf("NODE_QUEUED\n");
	    return (NODE_QUEUED);
	  }
	  else{
	    /*
	     * If there is a collision and we can not queue, we return NODE_COLISION
	     */
	    printf("NODE_COLLISION\n");
	    return (NODE_COLLISION);
	  }
	}
      }while((iter = listGetHead(&iter->list, tree_node_t)) != prev);
      /*
       * If we got there, there were no collisions, just need to
       * to decide which branch of the tree to take.
       */
      if(newNode->start_lba > iter->start_lba){
	iter = iter->child[RIGHT];
	direction = RIGHT;
      }
      else{
	iter = iter->child[LEFT];
	direction = LEFT;
      }
    }
    /*
     *Add the new node to the tree
     */
    prev->child[direction] = newNode;
    newNode->parent = prev;
    
    /*see if the tree needs to be rebalanced.*/
    rebalance(root, prev);
  }
  else
    *root = newNode;
  
  printf("NODE_ADDED \n");
  return NODE_ADDED;
}

/**
 * @brief Locate the inorder predecessor for the specified node
 *
 * The in-order predecessor is the right most node of the specified nodes left child.
 *
 * @param[in] node -- pointer to the starting node
 *
 * @retval Pointer to the node with the closest value less than the specified node
 **/
static tree_node_t *predecessor(tree_node_t *node){
  for(node = node->child[LEFT]; node->child[RIGHT]; node = node->child[RIGHT])
    continue;
  return node;
}


/**
 * @brief Remove a node from the tree
 * 
 * @param[in] root -- Pointer to the root node pointer
 * @param[in] node -- Pointer to the node to be removed
 * 
 * @retval N/A
 */
void removeNode(tree_node_t **root, tree_node_t *node){
  int side;
  if(node->child[LEFT] && node->child[RIGHT]) //both children
    {
      printf("delete case 1 with both children\n");
      tree_node_t *replace;
      
      /*
      * Get the right most child from the left branch of this node
      */
      replace = predecessor(node);
      
      /*
      * remove the predecessor from the tree
      */
      removeNode(root, replace);
      
      /*
       * update children and parent
       */
      if(replace->child[LEFT] = node->child[LEFT])
	replace->child[LEFT]->parent = replace;
     
      replace->child[RIGHT] = node->child[RIGHT];
      replace->child[RIGHT]->parent = replace;
      
      /*
       * If we have a parent, point it to the replacement node.
       * If we do not have a parent we must be the root
       */
      if(replace->parent = node->parent){
	side = (node->parent->child[RIGHT] == node);
	node->parent->child[side] = replace;
      }
      *root = replace;
      
      replace->height = node->height;
    }
  else if(node->child[LEFT]) //only a left child
    {
      printf("delete case 2 with left child\n");
      if(node != *root){
	side = (node->parent->child[RIGHT] == node);
	node->parent->child[side] = node->child[LEFT];
	node->child[LEFT]->parent = node->parent;
      }
      else{
	*root = node->child[LEFT];
	(*root)->parent = NULL;
      }
    }
  else if(node->child[RIGHT]) //only a right child
    {
      printf("delete case 3 with right child\n");
      if(node != *root){
	side = (node->parent->child[RIGHT] == node);
	node->parent->child[side] = node->child[RIGHT];
	node->child[RIGHT]->parent = node->parent;
      }
      else{
	*root = node->child[RIGHT];
	(*root)->parent = NULL;
      }
    }
  else
    {
      printf("delete case 4 no children ");
      if(node != *root){
	printf("not root\n");
	side = (node->parent->child[RIGHT] == node);
	node->parent->child[side] = NULL;
      }
      else{
	printf("  root\n");
	*root = NULL;
      }
    }
}


/**
* @brief print out the avl tree for debugging
*
* @param[in] root
*/
void treeDump(tree_node_t *root){
  if(root == NULL)
    return;
  else{
    printf("Index %d, range[ %d -- %d ], height %d, W(%d) \n", root->eventIndex, root->start_lba, root->end_lba, root->height, root->type); 
    
    // each pending
    if (!listEmpty(&root->pendingList)) {
        tree_node_t *pnode = listGetHead(&root->pendingList, tree_node_t);
        tree_node_t *snode = root;
	printf("Pending list\n");
        do{
	  printf("Index %2d, range[ %2d -- %2d ], height %d, W(%2d) \n", pnode->eventIndex, pnode->start_lba, pnode->end_lba, pnode->height, pnode->type); 
            pnode = (tree_node_t *) pnode->pendingList.next;
        }while (snode != pnode);
	printf("\n");
    }
    if(root->child[LEFT]){
      printf("L  ");
      treeDump(root->child[LEFT]);
    }
    if(root->child[RIGHT]){
      printf("R ");
      treeDump(root->child[RIGHT]);
    }
  }
}

/**
 * @rief update and sum are two routines of binary index tree.
 *        More details of it can be found in https://en.wikipedia.org/wiki/Fenwick_tree
 *
 * @param[in]: l -- the location we want to upate value
 * @param[in]: v -- the value we want to add
 * 
 * @retval: N/A
 */
void update(unsigned int l, unsigned int v){
  while(l <= MAX_NODES){
    tree_array[l] += v;
    
    l             += lowbit(l);
  }
}

/** 
 * @brief: the sum routine of binary index tree.
 *
 * @paran[in]: l -- indicates the s
 * 
 * @retval: return sum of array[1.... l]
 */
unsigned int sum(unsigned int l){
  unsigned int ret = 0;
  while(l>0){
    ret += tree_array[l];
    l   -= lowbit(l);
  }
  return ret;
}


/**
 * @brief process the event index overflow problem by updating event index of the AVL tree
 *
 * @param[in]  root        --- root node of AVL tree
 *
 *
 * @retval    N/A
 **/
void updateIndex(tree_node_t *root){
  if(root == NULL)
    return;

  /*update the event index of the root*/
  root->eventIndex = sum(root->eventIndex) ;
  
  /*update the event indices of root pending list*/
  if(!listEmpty(&root->pendingList)){
    tree_node_t *pnode = listGetHead(&root->pendingList, tree_node_t);
    tree_node_t *snode = root;
    do{
      pnode->eventIndex = sum(pnode->eventIndex) ;
      pnode = (tree_node_t *)pnode->pendingList.next;
    }while(snode != pnode);
  }
  
  /*left children*/
  if(root->child[LEFT])
    updateIndex(root->child[LEFT]);
  /*right children*/
  if(root->child[RIGHT])
    updateIndex(root->child[RIGHT]);
}

/**
 * @brief: from the AVL tree, obtain the event index maker in tree_array
 *
 * @param[in] root --- AVL tree node
 *
 * @retval N/A
 **/

void obtainEventInexMarker(tree_node_t *root){ 
  if(root == NULL) {  return;}
  /*root node*/
  update(root->eventIndex, 1);

  /*deal with the pending list*/
  if(!listEmpty(&root->pendingList)){
    tree_node_t *pnode = listGetHead(&root->pendingList, tree_node_t);
    tree_node_t *snode = root;
    do{
      update(pnode->eventIndex, 1);
      pnode = (tree_node_t *)pnode->pendingList.next;
    }while(snode != pnode);
  }

  /*left children*/
  if(root->child[LEFT])
    obtainEventInexMarker(root->child[LEFT]);
  
  /*right children*/
  if(root->child[RIGHT])
    obtainEventInexMarker(root->child[RIGHT]);
}

/**
 * @brief compare two AVL tree nodes are the same or not.
 *
 * @param[in] n1 -- AVL tree node one
 * @param[in] n2 -- another AVL tree node
 *
 * @retval  1 -- two nodes are equal
 *          2 -- the other case
 **/
int comNode(tree_node_t *n1, tree_node_t *n2){
  /*properly enough to compare two event indices*/
  if(n1->eventIndex == n2->eventIndex && 
     n1->start_lba  == n2->start_lba  && 
     n1->end_lba    == n2->end_lba    &&
     n1->type       == n2->type       &&
     n1->type       == n2->type )
    return 1;
  else
    return 0;
}

/**
 * @brief test whether a node is within AVL tree or not (excluding pending list)
 * 
 * @param[in] root -- the root node of the AVL tree
 * @param[in] node -- test node
 *
 * @retval  1 -- in AVL tree
 * @retval  0 -- no.
 */
int isInAVL(tree_node_t *root, tree_node_t *node){
  if(root != NULL){   
    tree_node_t *iter = root;
    while(iter != NULL){
      if(comNode(iter, node))
	 return 1;
    
      if(iter->start_lba < node->start_lba)
	iter = iter->child[RIGHT];
      else
	iter = iter->child[LEFT];
    }
    return 0;
  }
  else
    return 0;
}

/**
 * @brief Process a logical address lock request
 * 
 * @param[in] start_lba   -- the start logical block address
 * @param[in] end_lba     -- the end logical block address
 * @param[in] type        -- write event or read event
 * @param[in] queue       -- whether the node can be queued or not. If it can be queued, it is possible that we can put it at the pending list.
 *
 * @retval The insertion result.
 **/
enum NODE_INSERT_RESULT lockRequest(unsigned int start_lba, 
				    unsigned int end_lba, 
				    unsigned int type, 
				    unsigned queue, 
				    unsigned int namespaceID){
  tree_node_t *node;
  if( (node = allocNodes()) != NULL){
    node->start_lba   = start_lba;
    node->end_lba     = end_lba;
    node->type        = type;

  
    /*deal with the next_index overflows problem*/
    if(next_index[namespaceID]  + 1 >= MAX_NODES){
      memset(tree_array, 0, sizeof(tree_array));
      obtainEventInexMarker(rootArray[namespaceID]);
      updateIndex(rootArray[namespaceID]);
      next_index[namespaceID] = sum(MAX_NODES);
    }
    next_index[namespaceID]++;
    node->eventIndex = next_index[namespaceID];

    unsigned int ret = insertNode(&rootArray[0], node, queue);
    if(ret == NODE_COLLISION){
      freeNode(node);
      return ret;
    }
    printf("insert event index %d  W(%d) [%4d --%4d]\n", node->eventIndex, type, start_lba, end_lba);
    return ret;
  }
  return NODE_FAILED;
}


/**
 * @brief test whether a node is within AVL tree or not (including pending list)
 * 
 * @param[in] root -- the root node of the AVL tree
 * @param[in] node -- test node
 *
 * @retval  1 -- in AVL tree
 * @retval  0 -- no.
 */
int isInAVLWithPendingList(tree_node_t *root, tree_node_t *node){
  if(root != NULL){   
    /*check the pending list*/
    tree_node_t *prev = NULL;
    tree_node_t *iter = root;

    while(iter != NULL){
      prev = iter;
      do{
	/*check the root*/
	if(comNode(iter, node)){
	  //printf("Find ");
	  return 1;
	}
	//else
	  //printf("P ");
      }while((iter = (tree_node_t *) (&iter->list)->next) != prev);
    
      if(iter->start_lba < node->start_lba){
	//printf("R ");
	iter = iter->child[RIGHT];
      }
      else{
	//printf("L ");
	iter = iter->child[LEFT];
      }
    }
    return 0;
  }
  else
    return 0;
}

/**
 * @brief  Process a logical address lock release.
 * 
 * @param[in] node -- a node to be unlocked
 * @param[in] namespaceID -- The namespace id of the lock being released.
 *
 **/
void lockRelease(tree_node_t *node, unsigned int namespaceID){ 
  /*probably do not need in the real code. The caller has to make sure it is correct*/
   /*need to check whether the AVL tree with the pending list contains the node or not*/
  if(!isInAVLWithPendingList(rootArray[namespaceID], node)){
    printf("Wrong operation: delete a node not in the AVL tree\n");
    return;
  }
  /*probably do not need it in the real code. The caller has to make sure it is correct*/
  if(!isInAVL(rootArray[namespaceID], node)){
    printf("Wrong operation: delete a node in the pending list\n");
    return;
  }
  /**
  * Remove the node from the tree.
  **/
  removeNode(&rootArray[namespaceID], node);
  
  /*
   * check if this node has pending locks associated with it.
   * And if so, try to add each one to the tree
   */
  tree_node_t *pendingNode = listGetHead(&node->pendingList, tree_node_t);
  while(pendingNode != node){
    printf("insert node with index %2d\n", pendingNode->eventIndex);
    tree_node_t *nextNode = listGetHead(&pendingNode->pendingList, tree_node_t);
    pendingNode->child[0] = NULL;
    pendingNode->child[1] = NULL;
    
    /*
     * Reset removed node pending list pointers
     */
    listInit(&pendingNode->pendingList);
    
    insertNode(&rootArray[namespaceID], pendingNode, 1);
    
    pendingNode = nextNode;
  }
  
  /*Add the removed node back to the free list*/
  freeNode(node);
}
