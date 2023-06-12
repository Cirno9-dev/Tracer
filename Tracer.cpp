#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>
#include <unistd.h>
#include <sys/stat.h>

using std::cerr;
using std::endl;
using std::string;
using std::map;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 insCount = 0;
UINT64 count = 0;

UINT64 readCount = 0;
UINT64 writeCount = 0;

BOOL flag = false;
UINT64 mallocCount = 0;
UINT64 callocCount = 0;
UINT64 freeCount = 0;

UINT64 targetSize = 0;
UINT64 minAddress = 0;
UINT64 maxAddress = 0;

string bin;
VOID* loadedAddress;
VOID* codeAddress;
map<VOID*, string> disassembleCode;

string outputDir = "./output";
string infoFile = "./output/info.log";
std::ostream* info = nullptr;
string traceFile = "./output/trace.log";
std::ostream* trace = nullptr;
string memoryTraceFile = "./output/memoryTrace.log";
std::ostream* memoryTrace = nullptr;
string heapTraceFile = "./output/heapTrace.log";
std::ostream* heapTrace = nullptr;

VOID init()
{
    if (access(outputDir.c_str(), 0) == -1) {
        mkdir(outputDir.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
    }
    info = new std::ofstream(infoFile.c_str(), std::ios_base::out);
    trace = new std::ofstream(traceFile.c_str(), std::ios_base::out);
    memoryTrace = new std::ofstream(memoryTraceFile.c_str(), std::ios_base::out);
    heapTrace = new std::ofstream(heapTraceFile.c_str(), std::ios_base::out);
}

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#if defined(TARGET_MAC)
#define MALLOC "_malloc"
#define FREE "_free"
#else
#define MALLOC "malloc"
#define CALLOC "calloc"
#define FREE "free"
#endif

/* ===================================================================== */
// Utilities
/* ===================================================================== */

INT32 Usage()
{
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

VOID RecordArg1(CHAR* name, ADDRINT arg)
{
    if (flag) {
        *heapTrace << "0x0" << endl;
    }

    if (!strcmp(name, FREE)) {
        flag = false;
        freeCount++;
        count++;
        *heapTrace << insCount << "\t" << count << "\t" << name << " [" << (VOID*)arg << "] 0x0" << endl;
    } else if (!strcmp(name, MALLOC)) {
        flag = true;
        mallocCount++;
        count ++;
        targetSize = arg;
        *heapTrace << insCount << "\t" << count << "\t" << name << " [" << arg << "] ";
    }
}

VOID RecordArg2(CHAR* name, ADDRINT arg1, ADDRINT arg2)
{
    if (flag) {
        *heapTrace << "0x0" << endl;
    }

    flag = true;
    count ++;
    callocCount++;
    targetSize = arg1 * arg2;
    *heapTrace << insCount << "\t" << count << "\t" << name << " [" << arg1 << "," << arg2 << "] ";
}

VOID RecordRet(ADDRINT ret)
{
    flag = false;
    *heapTrace << (VOID*)ret << endl;

    if (minAddress != 0) {
        if (ret < minAddress) {
            minAddress = ret;
        }
    } else {
        minAddress = ret;
    }

    if (maxAddress != 0) {
        if (ret + targetSize > maxAddress) {
            maxAddress = ret + targetSize;
        }
    } else {
        maxAddress = ret + targetSize;
    }
}

VOID Image(IMG img, VOID* v)
{
    if (IMG_IsMainExecutable(img)) {
        bin = IMG_Name(img);
        loadedAddress = (VOID*)IMG_StartAddress(img);
        codeAddress = (VOID*)IMG_LowAddress(img);

        *info << "fileName: \'" << bin << "\'" << endl;
        *info << "loadedAddress: " << loadedAddress << endl;
        *info << "codeAddress: " << codeAddress << endl;
    }

    // malloc
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn)) {
        RTN_Open(mallocRtn);
        RTN_InsertCall(
            mallocRtn, IPOINT_BEFORE, 
            (AFUNPTR)RecordArg1, 
            IARG_ADDRINT, MALLOC, 
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0, 
            IARG_END
        );
        RTN_InsertCall(
            mallocRtn, IPOINT_AFTER, 
            (AFUNPTR)RecordRet, 
            IARG_FUNCRET_EXITPOINT_VALUE, 
            IARG_END
        );
        RTN_Close(mallocRtn);
    }

    // calloc
    RTN callocRtn = RTN_FindByName(img, CALLOC);
    if (RTN_Valid(callocRtn)) {
        RTN_Open(callocRtn);
        RTN_InsertCall(
            callocRtn, IPOINT_BEFORE, 
            (AFUNPTR)RecordArg2, 
            IARG_ADDRINT, CALLOC, 
            IARG_FUNCARG_ENTRYPOINT_VALUE,  0,
            IARG_FUNCARG_ENTRYPOINT_VALUE,  1,
            IARG_END
        );
        RTN_InsertCall(
            callocRtn, IPOINT_AFTER, 
            (AFUNPTR)RecordRet, 
            IARG_FUNCRET_EXITPOINT_VALUE, 
            IARG_END
        );
        RTN_Close(callocRtn);
    }

    // free()
    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn)) {
        RTN_Open(freeRtn);
        RTN_InsertCall(
            freeRtn, IPOINT_BEFORE, 
            (AFUNPTR)RecordArg1, 
            IARG_ADDRINT, FREE, 
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0, 
            IARG_END
        );
        RTN_Close(freeRtn);
    }
}

VOID RecordIns(VOID* address)
{
    insCount += 1;
    *trace << insCount << "\t" << address << ": " << disassembleCode[address] << endl;
}

VOID RecordMemRead(VOID* address, VOID* targetAddress, UINT32 size)
{
    if ((UINT64)targetAddress >= minAddress && (UINT64)targetAddress <= maxAddress) {
        readCount += 1;
        count ++;
        *memoryTrace << insCount << "\t" << count << "\t" << address << " R " << targetAddress << " " << size << endl;
    }
}

VOID RecordMemWrite(VOID* address, VOID* targetAddress, UINT32 size)
{
    if ((UINT64)targetAddress >= minAddress && (UINT64)targetAddress <= maxAddress) {
        writeCount += 1;
        count ++;
        *memoryTrace << insCount << "\t" << count << "\t" << address << " W " << targetAddress << " " << size << endl;
    }
}

VOID Trace(INS ins, VOID* v)
{
    // Stake insertion for read and write instructions, not just for the main program.
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        // Read
        if (INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, 
                (AFUNPTR)RecordMemRead, 
                IARG_INST_PTR, 
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYREAD_SIZE,
                IARG_END
            );
        }
        // Write
        if (INS_MemoryOperandIsWritten(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, 
                (AFUNPTR)RecordMemWrite, 
                IARG_INST_PTR, 
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYWRITE_SIZE,
                IARG_END
            );
        }
    }

    PIN_LockClient();
    IMG img = IMG_FindByAddress(INS_Address(ins));
    PIN_UnlockClient();
    if (!IMG_Valid(img) || !IMG_IsMainExecutable(img)) {
        return;
    }

    VOID* address = (VOID*) INS_Address(ins);
    string insDisassemble = INS_Disassemble(ins);
    disassembleCode[address] = insDisassemble;

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordIns, IARG_INST_PTR,  IARG_END);
}

VOID Fini(INT32 code, VOID* v)
{
    cerr << endl;
    cerr << "===============================================" << endl;
    cerr << "Tracer analysis results: " << endl;
    cerr << "FileName: " << bin << endl;
    cerr << "Loaded address: " << loadedAddress << endl;
    cerr << "Code address: " << codeAddress << endl;
    cerr << "Number of instructions: " << insCount << endl;
    cerr << "Number of read memory instructions: " << readCount << endl;
    cerr << "Number of write memory instructions: " << writeCount << endl;
    cerr << "Number of malloc: " << mallocCount << endl;
    cerr << "Number of calloc: " << callocCount << endl;
    cerr << "Number of free: " << freeCount << endl;
    cerr << "===============================================" << endl;

    *info << "insCount: " << insCount << endl;
    *info << "readCount: " << readCount << endl;
    *info << "writeCount: " << writeCount << endl;
    *info << "mallocCount: " << mallocCount << endl;
    *info << "callocCount: " << callocCount << endl;
    *info << "freeCount: " << freeCount << endl;
}

int main(int argc, char* argv[])
{
    init();
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }
    PIN_InitSymbols();

    // Register function to be called when image load
    IMG_AddInstrumentFunction(Image, 0);

    // Register function to be called to instrument Instrumention
    INS_AddInstrumentFunction(Trace, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    cerr << "===============================================" << endl;
    cerr << "This application is instrumented by Tracer" << endl;
    cerr << "Info File: " << infoFile << endl;
    cerr << "Trace File: " << traceFile << endl;
    cerr << "Memory Trace File: " << memoryTraceFile << endl;
    cerr << "Heap Trace File: " << heapTraceFile << endl;
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
