extern "C" {
float mul(int a, float b) {
    return a * b;
}

void kernel(void *args) {
    int *int_base = (int *)args;
    int *a = int_base + 0;
    float *float_base = (float *)(int_base + 1);
    float *b = float_base + 0;
    float *c = float_base + 1;
    *c = mul(*a, *b);
}
} // extern "C"