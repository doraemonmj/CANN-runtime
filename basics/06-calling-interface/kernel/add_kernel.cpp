extern "C" {
int add(int a, int b) {
    return a + b;
}

void kernel(void *args) {
    int *a = (int *)args;
    int *b = (int *)args + 1;
    int *c = (int *)args + 2;
    *c = add(*a, *b);
}
} // extern "C"