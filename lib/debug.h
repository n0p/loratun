/*

	xkrambler's debug v0.1
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

#ifndef __DEBUG
#define __DEBUG

// outputs the contents of a memory address in hex (WARN: no overflow protection)
void debug(unsigned char *data, int len) {
	int i;
	printf("DEBUG(%d)[ ",len);
	for (i=0; i<len; i++)
		printf("%02X ", (unsigned char)data[i]);
	printf("]\n");
}

#endif
