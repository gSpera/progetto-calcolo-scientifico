#define TYPE float
__kernel void test(
    __global TYPE *a,
    __global TYPE *b,
    __global TYPE *c
) {
    int id = get_global_id(0);
    c[id] = log10(exp10(a[id] * b[id]));
}
