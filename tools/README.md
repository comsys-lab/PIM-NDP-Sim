# PIM GEMV Programming Guide

This document provides a comprehensive guide for using the PIM (Processing-in-Memory) Manager library to execute GEMV (General Matrix-Vector Multiplication) operations. It covers the essential data structures, API functions, and a complete usage example.

## 1. Overview

The PIM Manager library abstracts the hardware complexities of PIM devices, allowing users to allocate memory on PIM modules and execute vector-matrix operations efficiently.

To perform a GEMV operation, the workflow generally follows these steps:
1.  **Initialization**: Registering the PIM manager and hardware channels.
2.  **Allocation**: Allocating memory buffers (`PIM_BUFFER`) on the PIM device for inputs and weights.
3.  **Data Transfer**: (Optional) Copying data from host to PIM.
4.  **Execution**: Performing the GEMV computation using `PIMgemv`.
5.  **Termination**: Freeing memory and removing the manager.

---

## 2. Data Structures

The library uses two main structures defined in `pim_manager.h`:

* **`PIM_MANAGER`**: Manages the connection to the PIM hardware, including memory mapping and channel configurations.
* **`PIM_BUFFER`**: Represents a memory block allocated on the PIM device. It contains metadata such as size, bank mode, and physical address mappings.

---

## 3. API Reference

### Initialization & Cleanup

#### `PIMregister`
Initializes the PIM Manager and registers the PIM hardware channels.

```c
PIM_MANAGER* PIMregister(int n_ch);
```

* **Parameters**:
    * `n_ch` (int): The number of PIM channels to use (e.g., 1).
* **Returns**:
    * `PIM_MANAGER*`: A pointer to the initialized manager structure. Returns `NULL` if initialization fails.

#### `PIMremove`
Releases the PIM Manager resources and unregisters the device.

```c
void PIMremove(PIM_MANAGER* manager);
```

* **Parameters**:
    * `manager` (PIM_MANAGER*): The pointer to the PIM Manager to be removed.
* **Returns**:
    * `void`

### Memory Management

#### `PIMmalloc`
Allocates a memory buffer on the PIM device. The allocation mode determines how data is distributed across banks, which is crucial for parallel processing performance.

```c
PIM_BUFFER* PIMmalloc(PIM_MANAGER* manager, int size, int mode);
```

* **Parameters**:
    * `manager` (PIM_MANAGER*): The active PIM Manager.
    * `size` (int): The total number of elements (not bytes) to allocate.
    * `mode` (int): The bank allocation mode.
        * `0` (**abk**): All Banks Mode. (Recommended for **Weights**)
        * `1` (**4bk**): 4 Banks Mode.
        * `2` (**sbk**): Single Bank Mode. (Recommended for **Input Vectors**)
* **Returns**:
    * `PIM_BUFFER*`: A pointer to the allocated PIM buffer structure.

#### `PIMfree`
Frees a previously allocated PIM buffer.

```c
void PIMfree(PIM_BUFFER *pimbf);
```

* **Parameters**:
    * `pimbf` (PIM_BUFFER*): The pointer to the PIM buffer to free.
* **Returns**:
    * `void`

#### `PIMmemcpy`
Copies data from the host memory to the PIM device buffer.

```c
int PIMmemcpy(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, int host_data_size);
```

* **Parameters**:
    * `manager` (PIM_MANAGER*): The active PIM Manager.
    * `pimbf` (PIM_BUFFER*): The destination PIM buffer.
    * `hostData` (uint16_t*): A pointer to the source data array in host memory.
    * `host_data_size` (int): The number of elements to copy.
* **Returns**:
    * `int`: Status code (typically 0 for success).

### Computation

#### `PIMgemv`
Executes the General Matrix-Vector Multiplication on the PIM device.
Equation: $Output = Input \times Weight$

```c
int PIMgemv(PIM_MANAGER *manager, 
            PIM_BUFFER *input, 
            PIM_BUFFER *weight, 
            uint16_t *output, 
            int b, int in_h, int w_h, int w_r, int w_c);
```

* **Parameters**:
    * `manager` (PIM_MANAGER*): The active PIM Manager.
    * `input` (PIM_BUFFER*): The input vector stored in PIM memory.
    * `weight` (PIM_BUFFER*): The weight matrix stored in PIM memory.
    * `output` (uint16_t*): A pointer to the host memory array where the result will be stored.
    * `b` (int): Batch size.
    * `in_h` (int): Number of input heads (for Multi-Head Attention context).
    * `w_h` (int): Number of weight heads.
    * `w_r` (int): Weight rows (corresponds to the **Input Dimension**).
    * `w_c` (int): Weight columns (corresponds to the **Output Dimension**).
* **Returns**:
    * `int`: Status code (0 for success).

---
