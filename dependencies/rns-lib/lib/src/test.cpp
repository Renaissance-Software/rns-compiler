#include <RNS/cpp_containers.h>
#include <stdlib.h>
int main(void)
{
    void* address = malloc(5000);
    assert(address);
    s64 size = 5000;
    RNS_DebugAllocator a = RNS_DebugAllocator::create(address, size);
    int* A = new(&a) int[200];
}