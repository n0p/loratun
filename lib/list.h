/*

	xkrambler's minimal list implementation v0.2
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

#ifndef __LIST_H
#define __LIST_H

#include <malloc.h>

// list node
struct node {
	void *e;
	struct node *ant;
	struct node *sig;
};
typedef struct node Node;

// list
typedef struct {
	Node *first;
	Node *last;
	int num;
	int uids;
} List;

// setup an existing list O(1)
void listInit(List *list);

// creates a new list O(1)
List *listNew();

// destroys a list O(1)
void listFree(List *l);

// add node with an element to a list O(1)
Node *listNodeAdd(List *list, void *element);

// delete a node from a list O(1)
int listNodeDel(List *list, Node *n);

#endif
