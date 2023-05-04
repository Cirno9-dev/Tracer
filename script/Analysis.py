from lib import *

infoFile = "../output/info.log"
traceFile = "../output/trace.log"
memoryFile = "../output/memoryTrace.log"
heapFile = "../output/heapTrace.log"

def main() -> None:
    print("Analysis script for Tarcer output!")
    
    print("\nInfo:")
    info = Info(infoFile)
    print(info)
    
    print("\nTrace:")
    trace = Trace(traceFile)
    for ins in trace[:10]:
        print(ins)

    print("\nMemoryTrace:")
    memoryTrace = MemoryTrace(memoryFile)
    for memOp in memoryTrace[:10]:
        print(memOp)

if __name__ == "__main__":
    main()
