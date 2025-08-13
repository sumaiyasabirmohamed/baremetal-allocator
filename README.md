# Fixed-Size Memory Pool Allocator (Bare-Metal)

## Overview
This project implements a **simple, fixed-size memory pool allocator** designed for **bare-metal environments** where the standard `malloc()` and `free()` functions are not available.  

It provides:
- A statically allocated memory pool (`100 KB` total).
- Allocation and deallocation without dynamic heap.
- Support for multiple allocations.
- Special handling for allocating the **entire pool** in one request.
- Minimal overhead using a lightweight metadata system.

The project includes:
- **Allocator core** (`allocator.c` / `allocator.h`)
- **Test driver** (`main.c`) to validate functionality.

## Project Structure

```shell
.
├── LICENSE.md
├── out
│   └── allocator
├── README.md
└── source
    ├── allocator
    │   ├── inc
    │   │   └── allocator.h
    │   └── src
    │       └── allocator.c
    └── main.c
```

## Testing

All tests are implemented in `main.c`. Refer to the `main()` function for different test cases such as:
- Single block allocation
- Multiple small allocations
- deallocate and re-allocate behavior
- Whole pool allocation
- Allocation failure scenarios
