/*

	xkrambler's minimal list implementation v0.3
	03/Sep/2013 Pablo Rodriguez Rey (mr -at- xkr -dot- es)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation version 3 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "list.h"

// setup an existing list O(1)
void listInit(List *list) {
	list->first=NULL;
	list->last=NULL;
	list->id=NULL;
	list->count=0;
}

// creates a new list O(1)
List *listNew() {
	List *l=malloc(sizeof(List));
	listInit(l);
	return l;
}

// destroys a list O(1)
void listFree(List *l) {
	free(l);
	l=NULL;
}

// add node with an element to a list O(1)
Node *listNodeAdd(List *list, void *element) {
	Node *n=malloc(sizeof(Node));
	if (list->last) {
		n->prev=list->last;
		list->last->next=n;
	} else {
		list->first=n;
		n->prev=NULL;
	}
	n->e=element;
	n->next=NULL;
	list->last=n;
	list->count++;
	return n;
}

// delete a node from a list O(1)
int listNodeDel(List *list, Node *n) {
	if (!list || !n) return 0;
	if (n->prev && n->next) {
		n->prev->next=n->next;
		n->next->prev=n->prev;
	} else if (!n->prev && n->next) {
		list->first=n->next;
		n->next->prev=NULL;
	} else if (n->prev && !n->next) {
		list->last=n->prev;
		n->prev->next=NULL;
	} else {
		list->first=NULL;
		list->last=NULL;
	}
	free(n);
	list->count--;
	return 1;
}
