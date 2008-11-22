#include "sh.h"

#define NUM 10
#define gp &gr

int
main(void)
{
	TGroup gr;
	char *x[NUM];
	int i;

	galloc_new(&gr, NULL, 8  GALLOC_VST(NULL));
	for (i = 0; i < NUM; ++i) {
		x[i] = galloc(1, 4, gp);
		memcpy(x[i], "0 x", 4);
		x[i][0] += i;
		printf("%d %p %s\n", i, x[i], x[i]);
	}
	for (i = 0; i < NUM; ++i) {
		x[i] = grealloc(x[i], 1, 4096, gp);
		printf("%d %p %s\n", i, x[i], x[i]);
	}
	for (i = 0; i < NUM; ++i) {
		printf("%d %p %s\n", i, x[i], x[i]);
		gfree(x[i], gp);
	}
	galloc_del(gp);

	return (0);
}
