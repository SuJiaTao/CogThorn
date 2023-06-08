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

typedef struct __CTFieldPos {
	UINT64 index;
	UINT64 bitOffset;
} __CTFieldPos, * P__CTFieldPos;

__CTFieldPos __HCTIndexToFieldPos(UINT32 index) {
	__CTFieldPos rfp = {
		.index		= index >> 6,
		.bitOffset	= index & 0b111111
	};
	return rfp;
}

void __HCTDynListAddNode(PCTDynList pList) {
	EnterCriticalSection(&pList->lock);

	/// SUMMARY:
	/// creates a new node with appropriate element buffer and field size
	/// increments nodecount and updates list nodes accordingly
	
	PCTDynListNode node = CTAlloc(sizeof(*node));

	node->elements = CTAlloc(pList->elementSizeBytes * pList->elementsPerNode);

	const SIZE_T FIELD_ELEMENT_COUNT = ((pList->elementsPerNode >> 6) + 1);
	node->useField = CTAlloc(FIELD_ELEMENT_COUNT * sizeof(*node->useField));

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

void __HCTDynListRemoveNode(PCTDynList list, PCTDynListNode prevNode, PCTDynListNode nodeToRemove) {

	/// SUMMARY:
	/// if (node to remove is last node)
	///		set list last node to prev node
	/// 
	/// prevNode.next = nodeToRemove.next (only if exists)
	/// 
	/// free(nodeToRemove)
	


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

CTCALL	BOOL		CTDynListDestroy(PCTDynList list) {
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

	return TRUE;
}

CTCALL	BOOL		CTDynListClear(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTDynListClear failed: list was NULL");
		return FALSE;
	}

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
	/// checks if all nodes are used up, if so, then creates a new node
	/// searches for free element spot, increments element count, and returns it
	
	if (list->elementsUsedCount == list->elementsTotalCount) {
		__HCTDynListAddNode(list);
	}

	PVOID retPtr = NULL;

	PCTDynListNode node = list->nodeFirst;
	while (node != NULL) {
		
		for (UINT32 nodeIndex = 0; nodeIndex < list->elementsPerNode; nodeIndex++) {

			/// SUMMARY:
			/// converts nodeIndex into a bitfield index with a bit offset and then
			/// tests the bitfield. if bit is FALSE, then free index is found.
			/// increment node use and set bit to TRUE
			/// set retPtr to buffer ptr and goto completion stage
			
			__CTFieldPos fieldPos =__HCTIndexToFieldPos(nodeIndex);

			BOOL fieldState = _bittest64(node->useField + fieldPos.index, fieldPos.bitOffset);

			if (fieldState == FALSE) {

				_bittestandset64(node->useField + fieldPos.index, fieldPos.bitOffset);
				node->elementUseCount	+= 1;
				list->elementsUsedCount += 1;

				retPtr = node->elements + (nodeIndex * list->elementSizeBytes);

				LeaveCriticalSection(&list->lock);
				return retPtr;

			}
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
	///				if (node has no more elements and is NOT the first node)
	///					delete node, and update last node if applicable
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

			__CTFieldPos fieldPos = __HCTIndexToFieldPos(elementIndex);

			if (_bittest64(node->useField + fieldPos.index, fieldPos.bitOffset) == FALSE) {
				LeaveCriticalSection(&list->lock);
				CTErrorSetFunction("CTDynListRemove failed: element is already removed!");
				return FALSE;
			}
			
			_bittestandreset64(node->useField + fieldPos.index, fieldPos.bitOffset);
			node->elementUseCount	-= 1;
			list->elementsUsedCount -= 1;

			__stosb(
				node->elements + (elementIndex * list->elementSizeBytes),
				0,
				list->elementSizeBytes
			);

			if (prevNode != NULL && node->elementUseCount == 0) {
				__HCTDynListRemoveNode(
					list,
					prevNode,
					node
				);
			}

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

CTCALL	PCTIterator	CTIteratorCreate(PCTDynList list) {
	if (list == NULL) {
		CTErrorSetBadObject("CTIteratorCreate failed: list was NULL");
		return NULL;
	}

	PCTIterator iter		= CTAlloc(sizeof(*iter));
	iter->currentNode		= list->nodeFirst;
	iter->currentNodeIndex	= 0;
	iter->parent			= list;

	return iter;
}

CTCALL	BOOL		CTIteratorDestroy(PCTIterator iterator) {
	if (iterator == NULL) {
		CTErrorSetBadObject("CTIteratorDestroy failed: iterator was NULL");
		return FALSE;
	}

	CTFree(iterator);

	return TRUE;
}

CTCALL	PVOID		CTIteratorIterate(PCTIterator iterator) {

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

			__CTFieldPos fieldPos = __HCTIndexToFieldPos(iterator->currentNodeIndex);
			BOOL fieldState = _bittest64(
				iterator->currentNode->useField + fieldPos.index,
				fieldPos.bitOffset
			);

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
