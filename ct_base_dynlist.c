//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_base_dynlist.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_base.h"

#include <stdio.h>
#include <intrin.h>

static void __HCTDynListAddNode(PCTDynList pList) {
	EnterCriticalSection(&pList->lock);

	/// SUMMARY:
	/// creates a new node with appropriate element buffer and field size
	/// increments nodecount and updates list nodes accordingly
	
	PCTDynListNode node = CTAlloc(sizeof(*node));

	node->elements = CTAlloc(pList->elementSizeBytes * pList->elementsPerNode);
	node->useField = CTAlloc(pList->elementsPerNode * sizeof(*node->useField));

	if (pList->nodeLast == NULL) {
		pList->nodeFirst	= node;
		pList->nodeLast		= node;
	} else {
		pList->nodeLast->nextNode	= node;
		pList->nodeLast				= node;
	}
	
	pList->nodeCount			+= 1;
	pList->elementsTotalCount	+= pList->elementsPerNode;

	LeaveCriticalSection(&pList->lock);
}

static void __HCTDynListRemoveNode(PCTDynList list, PCTDynListNode prevNode, PCTDynListNode nodeToRemove) {

	/// SUMMARY:
	/// if (node to remove is last node)
	///		set list last node to prev node
	/// 
	/// prevNode.next = nodeToRemove.next (only if exists)
	/// 
	/// free(nodeToRemove)
	
	EnterCriticalSection(&list->lock);

	if (nodeToRemove == list->nodeLast) {
		list->nodeLast = prevNode;
	}

	if (prevNode != NULL)
		prevNode->nextNode = nodeToRemove->nextNode;

	CTFree(nodeToRemove->elements);
	CTFree(nodeToRemove->useField);
	CTFree(nodeToRemove);

	list->nodeCount				-= 1;
	list->elementsTotalCount	-= list->elementsPerNode;

	LeaveCriticalSection(&list->lock);
}

CTCALL	PCTDynList	CTDynListCreate(SIZE_T elemSize, UINT32 elemsPerNode) {
	if (elemSize == 0) {
		CTErrorSetParamValue("CTDynListCreate failed: elemSize was 0");
		return NULL;
	}
	if (elemsPerNode == 0) {
		CTErrorSetParamValue("CTDynListCreate failed: elemsPerNode was 0");
		return NULL;
	}

	PCTDynList pList = CTAlloc(sizeof(*pList));
	
	InitializeCriticalSection(&pList->lock);
	pList->elementSizeBytes = elemSize;
	pList->elementsPerNode	= elemsPerNode;

	__HCTDynListAddNode(pList);

	return pList;
}

CTCALL	BOOL		CTDynListDestroy(PCTDynList* pList) {
	if (pList == NULL) {
		CTErrorSetBadObject("CTDynListDestroy failed: pList was NULL");
		return FALSE;
	}

	PCTDynList list = *pList;

	if (list == NULL) {
		CTErrorSetBadObject("CTDynListDestroy failed: list was NULL");
		return FALSE;
	}

	EnterCriticalSection(&list->lock);

	/// loop through each node and free all buffers
	PCTDynListNode node		= list->nodeFirst;

	while (node != NULL) {
		CTFree(node->elements);
		CTFree(node->useField);

		PCTDynListNode tempNode = node;
		node = node->nextNode;

		CTFree(tempNode);
	}

	DeleteCriticalSection(&list->lock);
	CTFree(list);

	*pList = NULL;
	return TRUE;
}

CTCALL	BOOL		CTDynListClear(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTDynListClear failed: list was NULL");
		return FALSE;
	}

	EnterCriticalSection(&list->lock);

	// clear all nodes
	PCTDynListNode node = list->nodeFirst;

	while (node != NULL) {
		CTFree(node->elements);
		CTFree(node->useField);

		PCTDynListNode tempNode = node;
		node = node->nextNode;

		CTFree(tempNode);
	}

	// reset everything
	list->nodeFirst				= NULL;
	list->nodeLast				= NULL;
	list->nodeCount				= 0;
	list->elementsTotalCount	= 0;
	list->elementsUsedCount		= 0;
	
	// reset first node
	__HCTDynListAddNode(list);

	LeaveCriticalSection(&list->lock);

	return TRUE;
}

CTCALL	BOOL		CTDynListLock(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTDynListLock failed: list was NULL");
		return FALSE;
	}

	EnterCriticalSection(&list->lock);

	return TRUE;
}

CTCALL	BOOL		CTDynListUnlock(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTDynListUnlock failed: list was NULL");
		return FALSE;
	}

	LeaveCriticalSection(&list->lock);

	return TRUE;
}

CTCALL	PVOID		CTDynListAdd(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTDynListUnlock failed: list was NULL");
		return NULL;
	}

	EnterCriticalSection(&list->lock);

	/// SUMMARY:
	/// if (all elements are used)
	///		add new node to list
	/// while (next node exists):
	///		loop every element in node:
	///			get index use flag
	///			if (index is used)
	///				skip
	///			set use flag to true, increment element use count
	///			get ptr of node element
	///			return ptr
	/// <should never reach here>
	/// raise error
	
	if (list->elementsUsedCount == list->elementsTotalCount) {
		__HCTDynListAddNode(list);
	}

	PVOID retPtr = NULL;

	PCTDynListNode node = list->nodeFirst;
	while (node != NULL) {
		
		for (UINT32 nodeIndex = node->seekHead; nodeIndex < list->elementsPerNode; nodeIndex++) {
			
			PBYTE fieldState = node->useField + nodeIndex;

			if (*fieldState == TRUE) continue;

			*fieldState				= TRUE;
			node->elementUseCount	+= 1;
			list->elementsUsedCount += 1;
			node->seekHead			= nodeIndex;

			retPtr = node->elements + (nodeIndex * list->elementSizeBytes);
			LeaveCriticalSection(&list->lock);

			return retPtr;

		}
		
		node = node->nextNode;
	}

	CTErrorSetFunction("CTDynListAdd failed: critical failure");
	ExitProcess(ERROR_FUNCTION_FAILED);
}

CTCALL	BOOL		CTDynListRemove(PCTDynList list, PVOID element) {
	if (list == NULL) {
		CTErrorSetBadObject("CTDynListRemove failed: list was NULL");
		return NULL;
	}
	if (element == NULL) {
		CTErrorSetParamValue("CTDynListRemove failed: element was NULL");
		return NULL;
	}

	EnterCriticalSection(&list->lock);

	/// SUMMARY:
	/// (walk through all nodes)
	/// while (node is NOT NULL)
	///		if (element lies within node)
	///			if (element is already removed)
	///				raise error
	///			else
	///				clear use flag and decrement element count and zero memory
	/// 
	///				return
	///		else
	///			set node to next node
	/// (on reached here, all nodes have been tested to no avail)
	/// raise error
	
	PCTDynListNode node		= list->nodeFirst;
	PCTDynListNode prevNode	= NULL;
	while (node != NULL) {

		if (element < (node->elements + list->elementSizeBytes * list->elementsPerNode) &&
			element >= node->elements) {

			SIZE_T elementIndex = ((PBYTE)element - node->elements) / list->elementSizeBytes;
			PBYTE  fieldState	= node->useField + elementIndex;

			if (*fieldState == FALSE) {
				LeaveCriticalSection(&list->lock);
				CTErrorSetFunction("CTDynListRemove failed: element is already removed!");
				return FALSE;
			}
			
			*fieldState				 = FALSE;
			node->elementUseCount	-= 1;
			list->elementsUsedCount -= 1;

			node->seekHead	= min(node->seekHead, elementIndex);

			__stosb(
				node->elements + (elementIndex * list->elementSizeBytes),
				0,
				list->elementSizeBytes
			);

			LeaveCriticalSection(&list->lock);
			return TRUE;
		}

		prevNode	= node;
		node		= node->nextNode;
	}

	LeaveCriticalSection(&list->lock);
	CTErrorSetFunction("CTDynListRemove failed: element could not be found in list");
	return FALSE;
}

CTCALL	BOOL		CTDynListClean(PCTDynList list) {

	if (list == NULL) {
		CTErrorSetBadObject("CTDynListClean failed: list was NULL");
		return NULL;
	}

	EnterCriticalSection(&list->lock);

	/// SUMMARY:
	/// cache total amount of nodes
	/// repeat cahced amt of node times:
	///		get current amt of nodes
	///		loop all nodes:
	///			if (node is NOT first node && node has no elements)
	///				remove node
	///				break
	///			else
	///				increment good node counter
	///		if (current amt of nodes == good node counter)
	///			break
	
	UINT32 startNodeCount	= list->nodeCount;

	for (UINT32 i = 0; i < startNodeCount; i++) {

		PCTDynListNode node		= list->nodeFirst;
		PCTDynListNode prevNode = NULL;

		UINT32 currentNodeCount = list->nodeCount;
		UINT32 usedNodeCounter	= 0;

		while (node != NULL) {
			if (node != list->nodeFirst && node->elementUseCount == 0) {

				__HCTDynListRemoveNode(
					list,
					prevNode,
					node
				);
				break;

			}
			else
			{
				usedNodeCounter++;
			}

			prevNode = node;
			node	 = node->nextNode;
		}

		//if (usedNodeCounter == currentNodeCount)
		//	break;

	}

	LeaveCriticalSection(&list->lock);
	
	return TRUE;
}

CTCALL	PCTIterator	CTIteratorCreate(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTIteratorCreate failed: list was NULL");
		return NULL;
	}

	PCTIterator iter		= CTAlloc(sizeof(*iter));
	iter->currentNode		= list->nodeFirst;
	iter->currentNodeIndex	= 0;
	iter->parent			= list;

	CTDynListLock(list);

	return iter;
}

CTCALL	BOOL		CTIteratorDestroy(PCTIterator* pIterator) {
	if (pIterator == NULL) {
		CTErrorSetBadObject("CTIteratorDestroy failed: pIterator was NULL");
		return FALSE;
	}

	PCTIterator iterator = *pIterator;

	if (iterator == NULL) {
		CTErrorSetBadObject("CTIteratorDestroy failed: iterator was NULL");
		return FALSE;
	}

	CTDynListUnlock(iterator->parent);
	CTFree(iterator);

	*pIterator = NULL;
	return TRUE;
}

CTCALL	PVOID		CTIteratorIterate(PCTIterator iterator) {

	if (iterator == NULL) {
		CTErrorSetBadObject("CTIteratorIterate failed: iterator was NULL");
		return FALSE;
	}

	/// SUMMARY:
	/// while scanned node is NOT NULL:
	///		if (node index is within node):
	///			if (node is unused)
	///				increment node index and continue
	///			else
	///				return index ptr and increment node index
	///		else
	///			update to next node and reset node index
	///			(note: next node could be null)
	/// (note: on reached here, no more nodes)
	/// list iteration is complete, return NULL
	
	while (iterator->currentNode != NULL) {

		if (iterator->currentNodeIndex < iterator->parent->elementsPerNode) {

			BOOL fieldState = iterator->currentNode->useField[iterator->currentNodeIndex];

			if (fieldState == FALSE) {

				iterator->currentNodeIndex += 1;
				continue;

			} else {

				PBYTE elementPtr = 
					iterator->currentNode->elements + 
					(iterator->parent->elementSizeBytes * iterator->currentNodeIndex);

				iterator->currentNodeIndex += 1;
				return elementPtr;

			}
		} else {

			iterator->currentNode		= iterator->currentNode->nextNode;
			iterator->currentNodeIndex	= 0;

		}
	}

	return NULL;
}
