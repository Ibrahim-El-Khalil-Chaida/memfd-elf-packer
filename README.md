```markdown
# MEMFD ELF Packer

A compact C “packer” that embeds an ELF payload into its own binary, erases itself on disk,
then launches the embedded payload purely in memory via `memfd_create` and `fexecve`.
 Comes with a helper `run.sh` to build & embed your payload automatically.


## 📋 Structure
```
<pre> 
├── packer.c            ← The main packer source
├── payload.c           ← Example payload (must exist for run.sh)
├── run.sh              ← Build & embed script
├── README.md           ← This documentation
└── LICENSE             ← MIT License </pre>

````



## 📝 Description

1. **packer.c**  
   - Reads its own executable via `/proc/<pid>/exe`  
   - Unlinks (`unlink( argv[0] )`) to remove itself from the filesystem  
   - Scans for the second ELF header in its concatenated payload  
   - Creates an anonymous in-memory file (`memfd_create`)  
   - Writes the payload ELF into it and calls `fork()`  
   - The child process calls `fexecve()` on the in-memory FD to execute the payload  

2. **payload.c**  
   - Your standalone ELF program that you wish to hide/execute in memory  

3. **run.sh**  
   - Compiles `payload.c` with `gcc -O2` and `strip`  
   - Compiles `packer.c` and appends the stripped payload  
   - Runs `./emb` to demonstrate in-memory execution  

---

## 🔧 Prerequisites

- Linux kernel ≥ 3.17  
- `glibc` with `fexecve` support  
- Standard build tools: `gcc`, `make`, `bash`  

---

## 🚀 Build & Embed

```bash
chmod +x run.sh
./run.sh
````

This script will:

1. Build and strip `payload` (`payload.c → payload`)
2. Build the packer (`packer.c → emb`)
3. Append `payload` to `emb`
4. Execute `emb`, which unlinks itself and launches `payload` in memory

---

## ⚙️ Usage

* Modify **payload.c** with your own ELF-based logic.
* Run `./run.sh` to re-embed and test.
* The final binary `emb` will delete itself from disk at startup and run `payload` entirely in memory.

---

## 🔍 Optimization & Notes

* **Error checking**: All `read()`, `write()`, `open()` calls now verify return values.
* **Memory safety**: Uses `fstat()` to obtain size; avoids magic numbers.
* **Resource cleanup**: Closes all file descriptors and frees buffers after use.

---
