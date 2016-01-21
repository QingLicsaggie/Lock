#include"lock_manager.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<limits.h>
extern list_head_t freeNodes;
extern tree_node_t *rootArray[MAX_NAMESPACE_ID];
extern next_index[MAX_NAMESPACE_ID];
extern tree_array[MAX_NODES+1];

/**
 * @brief generate range lba range. Used for unit test.
 *
 **/
void random_lba_range(int *start_lba, int *end_lba){
    *start_lba = rand()%UCHAR_MAX/2;
    *end_lba   = rand()%UCHAR_MAX;

    if(*start_lba > *end_lba){
      int temp  = *start_lba;
      *start_lba = *end_lba;
      *end_lba  = temp;
    }
}

/**
 * @brief  obtain random node.
 *
 **/
void obtain_random_node(tree_node_t *node){    
    int start_lba, end_lba;

    random_lba_range(&start_lba, &end_lba);

    node->start_lba = start_lba;
    node->end_lba   = end_lba;
    node->type = rand()%2;

    if(next_index[0] + 1 > MAX_NODES){     
      memset(tree_array, 0, sizeof(tree_array));
      obtainEventInexMarker(rootArray[0]);
      updateIndex(rootArray[0]);
      next_index[0] = sum(MAX_NODES);
    }
    next_index[0]++;
    node->eventIndex = next_index[0];
}

/**
 * @brief Test AVL balanced tree property is mained or not.
 *
 * @para[in] node -- root node of AVL tree
 * 
 * @retval 1 -- balanced
 * @retval 0 -- unbalanced.
 */
int test_AVL_balanced(tree_node_t *node){
  if(node == NULL) return 1;

  /*test root*/
  if(height(node->child[RIGHT]) - height(node->child[LEFT]) >= 2)
    return 0;
  /*test left and right sub tree*/
  if(!test_AVL_balanced(node->child[LEFT]) || !test_AVL_balanced(node->child[RIGHT])) return 0;

  return 1;
}


/**
 * @brief test insert to AVL tree 
 *
 **/
void test_insert_AVL(){
  int i;
  treeInit();
  srand(time(NULL));
 
  /*test insert node*/
  for(i = 0; i < MAX_NODES + 5; i++){
    tree_node_t *node = allocNodes();
    if(node == NULL) continue;
    
    obtain_random_node(node);
    
    printf("inserting [%d -- %d] R(%d) eventIndex %d \n", 
	   node->start_lba, 
	   node->end_lba, 
	   node->type, 
	   node->eventIndex);
   
    insertNode(&rootArray[0], node, 1);
  
    printf("AVL tree propety maintained? ");
    if(test_AVL_balanced(rootArray[0]) == 1)
      printf("Y\n");
    else
      printf("N\n");
    printf("\n");
  }
  treeDump(rootArray[0]); 
}


void test_delete_AVL(){
  int i;
  
  for(i = 0; i < MAX_NODES; i++){
    printf("delete case %d\n", i);
    int index = rand()%MAX_NODES;
   
    if(isInAVL(rootArray[0], &nodes[index])){
      /*found nodes and then to delete it*/
       printf("delete  event index %d\n", nodes[index].eventIndex);
      removeNode(&rootArray[0], &nodes[index]);

     /*
      * check if this node has pending locks associated with it.
      * And if so, try to add each one to the tree
      */
      tree_node_t *pendingNode  = (tree_node_t *)  (nodes[index].pendingList.next);
      while(pendingNode != &nodes[index]){
	printf("insert node with index %2d\n", pendingNode->eventIndex);
	tree_node_t *nextNode = (tree_node_t *)  (pendingNode->pendingList.next); 

	pendingNode->child[0] = NULL;
	pendingNode->child[1] = NULL;
    
	/*
	 * Reset removed node pending list pointers
	 */
	pendingNode->pendingList.next = 
	pendingNode->pendingList.prev = 
	&pendingNode->pendingList;
    
	insertNode(&rootArray[0], pendingNode, 1);
    
	pendingNode = nextNode;
      }
  
      /*Add the removed node back to the free list*/
      freeNode(&nodes[index]);
      
      printf("AVL tree propety maintained? ");
      if(test_AVL_balanced(rootArray[0]) == 1)
	printf("Y\n");
      else
	printf("N\n");
      printf("\n");
    }
  }
}


/**
 * @brief: Test one example
*/
void test_case1(){
  treeInit();

  tree_node_t *n1,*n2, *n3, *n4, *n5, *n6;
  n1 = allocNodes();
  n2 = allocNodes();
  n3 = allocNodes();
  n4 = allocNodes();
  n5 = allocNodes();
  n6 = allocNodes();
  
 /**
  * Event 1: R [ 1- 40]
  */
  n1->eventIndex = 1;
  n1->start_lba  = 1;
  n1->end_lba    = 40;
  n1->type       = 0;
  

  /**
  * Event 2: W [ 1- 10]
  */
  n2->eventIndex = 2;
  n2->start_lba  = 1;
  n2->end_lba    = 10;
  n2->type       = 1;

 /**
  * Event 3: W [ 8- 20]
  */
  n3->eventIndex = 3;
  n3->start_lba  = 8;
  n3->end_lba    = 20;
  n3->type       = 1;

  
 /**
  * Event 4: W [ 21- 30]
  */
  n4->eventIndex = 4;
  n4->start_lba  = 21;
  n4->end_lba    = 30;
  n4->type       = 1; 

 /**
  * Event 5: W [ 31- 40]
  */
  n5->eventIndex = 5;
  n5->start_lba  = 31;
  n5->end_lba    = 40;
  n5->type       = 1;

  /**
   * Event 6: R [ 1- 40]
   */
  n6->eventIndex = 6;
  n6->start_lba  = 1;
  n6->end_lba    = 40;
  n6->type       = 0;
  
  /*insert all events, it should be a linked list
   * event 1 R[1-40] -->event 2 W[1-10] -->event 3 W[8-20] -->event 4 W[21-30] -->event 5 W[31-40] -->event 6 R[1-40]
   */
  insertNode(&rootArray[0], n1, 1);
  insertNode(&rootArray[0], n2, 1);
  insertNode(&rootArray[0], n3, 1);
  insertNode(&rootArray[0], n4, 1);
  insertNode(&rootArray[0], n5, 1);
  insertNode(&rootArray[0], n6, 1);

  treeDump(rootArray[0]);

  printf("remove n1\n");
  lockRelease(n1, 0);
 /*
  * the avl tree should be:
  *            event 4 --> event 6
  *                 / \
  *event 3 <- event 2 event 5
  */
  treeDump(rootArray[0]);

  /**
   * delete event 6, and it should be rejected.
   */
  printf("----\n\n");
  printf("remove n6\n");
  lockRelease(n6, 0);
  treeDump(rootArray[0]);
  /*
   * delete event 2, and the AVL tree should be like this
   * 
   * event 4 --> event 3 -->event 6
   *  \
   *  event 5
   */
  printf("---\n\n");
  printf("remove n2\n");
  lockRelease(n2, 0);
  treeDump(rootArray[0]);
  
  /*
   * delete event 4, and the AVL tree should be like this
   *  
   *  event 5 ->event 6
   *  /
   * event 3
   */
  printf("--\n\n");
  printf("remove n4\n");
  lockRelease(n4, 0);
  treeDump(rootArray[0]);
  
  /*
   * delete event 5, and the AVL tree should be like this
   *
   * event 3 -->event 6
   *  
   */
  printf("---\n\n");
  printf("remove n5\n");
  lockRelease(n5, 0);
  treeDump(rootArray[0]);
  
  /*
   * delete event 3, and the AVL tree should be like this.
   *
   * event 6
   */
  printf("--\n\n");
  printf("remove n3\n");
  lockRelease(n3, 0);
  treeDump(rootArray[0]);
}

/*
 *  @brief:  randomly lock MAX_NODES/2 logical block 
 *  address range requests (no pending), then randomly 
 *  queue MAX_NODES/2 logical block address range 
 *  requests, and randomly release locks.
 */
void random_test1(){
  int i, j;
   
  for(j = 0; j < 100; j++){
    treeInit();

    /*
     * to check the unoverlapped lock
     */
    for(i = 0; i < MAX_NODES/2; i++){
      unsigned start_lba = i*5;
      unsigned end_lba   = start_lba + 4;
      unsigned type      = rand()%2;
      lockRequest(start_lba, end_lba, type, 1, 0);
    }

    /*
     * to check the overlapped lock
     */
    for(i = 0; i < MAX_NODES/2; i++){  
      unsigned start_lba = i + 1;
      unsigned end_lba   = start_lba + rand()%10;
      unsigned type      = rand()%2;
      lockRequest(start_lba, end_lba, type, 1, 0);
    }
  
    // treeDump(rootArray[0]);

    /*
     * to check the lock release
     */
    for(i = 0; i < MAX_NODES; i++){
      printf("\n\ndeleting case %d with ", i+ 1);
      int index = (MAX_NODES - i - 1);
      printf("event index %d\n", nodes[index].eventIndex);
      lockRelease(&nodes[index], 0);
    }
      treeDump(rootArray[0]);
      printf("\n");
  }
}
/**
 * @brief: random test 2, unoverlapped range lock and unlock
 *
 */
void random_test2(){
  int i, j;
 
  for(j = 0; j < 2; j++){
    treeInit();
  
    /*
     *to check the lock
     */
    for(i = 0; i < MAX_NODES; i++){
      unsigned start_lba = i*5;
      unsigned end_lba   = start_lba + 4;
      unsigned type      = rand()%2;
      lockRequest(start_lba, end_lba, type, 1, 0);
    }
  
    //treeDump(rootArray[0]);

    /*
     *to check the unlock
     */
    for(i = 0; i < MAX_NODES -2; i++){
      printf("\n\ndeleting case %d with", i+ 1);
      int index = (MAX_NODES - i - 1);
      printf("  event index %d\n", nodes[index].eventIndex);
      lockRelease(&nodes[index], 0);
    }
     treeDump(rootArray[0]);

    /*
     *to check the index overflow
     */
    for(i = 0; i < MAX_NODES; i++){
      unsigned start_lba = i*5;
      unsigned end_lba   = start_lba + 4;
      unsigned type      = rand()%2;
      lockRequest(start_lba, end_lba, type, 1, 0);
      treeDump(rootArray[0]);
    }
    printf("\n");
  }
}

int main(){
  int i = 0;

  /**
   * @brief overnight running to test random insert and random delete
   */
  /*for(i = 0; i < INT_MAX; i++){
    test_insert_AVL();
    test_delete_AVL();
    }*/
  
  /*
   * @brief test the example presented in the document
   */
  
  //test1();

  //random_test1();

  random_test2();
  return 1;
}
