#ifndef ALLOCATOR_H
#define ALLOCATOR_H

int *allocate(int size);
void deallocate(int *ptr);

#endif /* ALLOCATOR_H */