# Tracer

The tracer implemented using intel pin.

---

## Usage

**download**

```bash
git clone https://github.com/Cirno9-dev/Tracer.git
```

**build & run**

32 Architecture:

```bash
cd Tracer
PIN_ROOT=<YOUR PIN ROOT> make Target=ia32
pin -t ./obj-ia32/Tracer.so -- /bin/true
```

64 Architecture:

```bash
cd Tracer
PIN_ROOT=<YOUR PIN ROOT> make Target=intel64
pin -t ./obj-intel64/Tracer.so -- /bin/true
```

**result**

```
===============================================
This application is instrumented by Tracer
Trace File: ./output/trace.log
Memory Trace File: ./output/memoryTrace.log
===============================================

===============================================
Tracer analysis results: 
FileName: /usr/bin/true
Loaded address: 0x7f2d34349000
Code address: 0x558e7979d000
Number of instructions: 99
Number of read memory instructions: 27
Number of write memory instructions: 18
Number of malloc: 0
Number of free: 0
===============================================

```

output/info.log

```
fileName: /usr/bin/true
loadedAddress: 0x7f249023d000
codeAddress: 0x56193c649000
insCount: 99
readCount: 41522
writeCount: 15450
mallocCount: 0
callocCount: 0
freeCount: 0
```

output/trace.log

```
 <ID> <Address>: <instruction>
```

output/memoryTrace.log

```
<ID> <Address> R/W <TargetAddress> <Size>
```

output/heapTrace.log

```
<ID> <FunName>(<ARG>) -> <RETURN>
```
