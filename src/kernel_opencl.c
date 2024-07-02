__kernel void stress(
    __global float *a,
    __global float *b,
    __global float *c
) {
    int id = get_global_id(0);

    c[id] = a[id];
    for (int i=0;i<10000;i++) {
        c[id] = exp10(log10(c[id] * b[id])) / b[id];
    }
}

#define SIZE 100
__kernel void sort(
    __global float *a,
    __global float *b
) {
    int id = get_global_id(0);
  float val = a[id];
  int pos = 0;
  for (int i=0;i<SIZE;i++) {
    if (a[i] > val) { pos++; }
  }
  b[SIZE-pos] = val;
}

/*
#define RES_X 483
#define RES_Y 720

// #undef RES_X
// #undef RES_Y
// #define RES_X 128
// #define RES_Y 191

__kernel void flip(
    __global unsigned int *a,
    __global unsigned int *b
) {
    int id = get_global_id(0);
    int x = id % RES_X;
    int y = id / RES_X;

    int xp = RES_X - x;
    int yp = y;
    int idp = yp * RES_X + xp;

    b[id] = a[idp];
}

__kernel void ch_col(
    __global unsigned int *a,
    __global unsigned int *b
) {
    int id = get_global_id(0);
    int x = id % RES_X;
    int y = id / RES_X;

	int red = (a[id]>>16) & 0xFF;
	int green = (a[id]>>8) & 0xFF;
	int blue = (a[id]>>0) & 0xFF;

    b[id] = green | (blue<<8) | (red << 16);

	// int avg = (red + green + blue) / 3;
	// b[id] = (avg << 16 | avg <<8 | avg);
}
*/
/*
#define RES_X 512
#define RES_Y 512
#define PI 3.1415
#define SCALE_Y 0.85
__kernel void plot(
    __global unsigned int *a
) {
    int id = get_global_id(0);
    int x = id % RES_X;
    int y = id / RES_X;


	float xf = (float) (x-RES_X/2) / 75;

	float f = sin(xf);

    int yp = f / 2 * RES_Y * SCALE_Y + RES_Y/2;
	if (yp < 0) yp = 0;
	if (yp >= RES_Y) yp = RES_Y -1;
    yp = RES_Y - yp;
    int idp = yp * RES_X + x;
	for (int y = 0; y < RES_Y; y++) {
		int idp = y * RES_X + x;
		a[idp] = 0;
	}
    a[idp] = 0xFF0000;
}*/

/*
#define RES_X 483
#define RES_Y 721

__kernel void color_correct(
    __global unsigned int *a,
    __global unsigned int *b
) {
    int id = get_global_id(0);
    int x = id % RES_X;
    int y = id / RES_X;

    int xp = RES_X - x;
    int yp = y;
    int idp = yp * RES_X + xp;

	int red = a[idp]>>16 & 0xFF;
	int green = a[idp]>>8 & 0xFF;
	int blue = a[idp]>>0 & 0xFF;

	red = 255 - red;
	green = green * 1.75;
	if (green > 255) green = 255;
	blue = blue * 0.5;

    b[id] = red<<16 | green<<8 | blue;
}
*/