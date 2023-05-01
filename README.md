# Tracer

The tracer implemented using intel pin.

---

## Usage

**download**

```bash
git clone https://github.com/Cirno9-dev/Tracer.git
```

**build & run**

change makefile:

```makefile
PIN_ROOT = <YOUR PIN ROOT>
```

32 Architecture:

```bash
cd Tracer
make Target=ia32
pin -t ./obj-ia32/Tracer.so -- /bin/true
```

64 Architecture:

```bash
cd Tracer
make Target=intel64
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
===============================================

```

output/trace.log

```
fileName: <FileName>
loadedAddress: <Address>
codeAddress: <Address>
trace: 
<Address>: <instruction>
```

output/memoryTrace.log

```
fileName: <FileName>
loadedAddress: <Address>
codeAddress: <Address>
memoryTrace: 
<Address> R/W <TargetAddress> <Size>
```
