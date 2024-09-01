#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef int32_t fixed;

#define PI              3.141592657

#define GLOBAL1         (1l<<16)
#define ANGLES          360
#define ANGLEQUAD       (ANGLES/4)
#define FINEANGLES      3600

const float radtoint = (float)(FINEANGLES/2/PI);

int32_t finetangent[FINEANGLES/4];
fixed   sintable[ANGLES+ANGLES/4];

void BuildTables (void)
{
    int i;
    for(i=0;i<FINEANGLES/8;i++)
    {
        double tang=tan((i+0.5)/radtoint);
        finetangent[i]=(int32_t)(tang*GLOBAL1);
        finetangent[FINEANGLES/4-1-i]=(int32_t)((1/tang)*GLOBAL1);
    }

    float angle=0;
    float anglestep=(float)(PI/2/ANGLEQUAD);
    for(i=0; i<ANGLEQUAD; i++)
    {
        fixed value=(int32_t)(GLOBAL1*sin(angle));
        sintable[i]=sintable[i+ANGLES]=sintable[ANGLES/2-i]=value;
        sintable[ANGLES-i]=sintable[ANGLES/2+i]=-value;
        angle+=anglestep;
    }
    sintable[ANGLEQUAD] = 65536;
    sintable[3*ANGLEQUAD] = -65536;
}

uint8_t atan2_tab[0x801];

static void init_atan2_tab(void) {
	float m = (float)(2 << 11) / acosf(0);
	unsigned i;
	for (i = 0; i < 0x801; i++) {
		int x = atan2f(i, 0x800) * m + 0.5f;
		atan2_tab[i] = x - i;
	}
}

int main(int argc, char **argv) {
	const char *fn = "wolf3d_tables.h";
	unsigned i, j;
	FILE *f;
	if (argc >= 2) fn = argv[1];

	BuildTables();
	init_atan2_tab();

	f = fopen(fn, "wb");
	if (!f) return 1;

#define PRINTARR(arr, n) \
	for (j = 0; j < (n); j++) { \
		fprintf(f, "%d, ", arr[j]); \
		if ((j + 1) % 8 == 0) fprintf(f, "\n"); \
	}

	fprintf(f, "int32_t finetangent[FINEANGLES/4] = {\n");
	PRINTARR(finetangent, FINEANGLES/4)
	fprintf(f, "};\n");
	fprintf(f, "fixed sintable[ANGLES+ANGLES/4] = {\n");
	PRINTARR(sintable, ANGLES+ANGLES/4)
	fprintf(f, "};\n");
	fprintf(f, "const uint8_t atan2_tab[0x801] = {\n");
	PRINTARR(atan2_tab, 0x801)
	fprintf(f, "};\n");
	fclose(f);
}

