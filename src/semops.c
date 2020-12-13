#include "header.h"

int up(int SemID)								/* up(sem) */
{
	struct sembuf op[1] = {0, 1, 0};			/* Array of one sembuf with the necessary values for up */
	if (semop(SemID, &op[0], 1) < 0)			/* up operation */
		return -1;
	return 0;
}


int down(int SemID)							/* down(sem) */
{
	struct sembuf op[1] = {0, -1, 0};			/* Array of one sembuf with the necessary values for down */
	if (semop(SemID, &op[0], 1) < 0)			/* down operation */
		return -1;
	return 0;
}
