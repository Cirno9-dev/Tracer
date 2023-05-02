#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>

using std::cerr;
using std::endl;
using std::string;
using std::map;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 insCount = 0;

UINT64 readCount = 0;
UINT64 writeCount = 0;

BOOL flag = false;
UINT64 mallocCount = 0;
UINT64 callocCount = 0;
UINT64 freeCount = 0;

string bin;
VOID* loadedAddress;
VOID* codeAddress;
map<VOID*, string> disassembleCode;

string traceFile = "./output/trace.log";
std::ostream* trace = new std::ofstream(traceFile.c_str(), std::ios_base::out);
string memoryTraceFile = "./output/memoryTrace.log";
std::ostream* memoryTrace = new std::ofstream(memoryTraceFile.c_str(), std::ios_base::out);
string heapTraceFile = "./output/heapTrace.log";
std::ostream* heapTrace = new std::ofstream(heapTraceFile.c_str(), std::ios_base::out);

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#if defined(TARGET_MAC)
#define MALLOC "_malloc"
#define FREE "_free"
#else
#define MALLOC "malloc"
#define CALLOC "calloc"
#define REALLOC "realloc"
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

VOID RecordArg1(CHAR* name, ADDRINT size, const CONTEXT *ctxt)
{
    void *buf[2];
    PIN_LockClient();
    PIN_Backtrace(ctxt, buf, sizeof(buf) / sizeof(buf[0]));
    PIN_UnlockClient();
    // Focus on main program calls only
    ADDRINT address = VoidStar2Addrint(buf[1]);
    PIN_LockClient();
    IMG img = IMG_FindByAddress(address);
    PIN_UnlockClient();
    if (IMG_Valid(img) && IMG_IsMainExecutable(img)) {
        if (!strcmp(name, MALLOC)) {
            flag = true;
            mallocCount += 1;
            *heapTrace << insCount << "\t" << name << "(" << size << ") -> ";
        } else if (!strcmp(name, FREE)) {
            freeCount += 1;
            *heapTrace << insCount << "\t" << name << "(" << (VOID*)size << ")" << endl;
        }
    }
}

VOID RecordArg2(CHAR* name, ADDRINT n, ADDRINT size, const CONTEXT *ctxt)
{
    void *buf[2];
    PIN_LockClient();
    PIN_Backtrace(ctxt, buf, sizeof(buf) / sizeof(buf[0]));
    PIN_UnlockClient();
    // Focus on main program calls only
    ADDRINT address = VoidStar2Addrint(buf[1]);
    PIN_LockClient();
    IMG img = IMG_FindByAddress(address);
    PIN_UnlockClient();
    if (IMG_Valid(img) && IMG_IsMainExecutable(img)) {
        if (!strcmp(name, CALLOC)) {
            flag = true;
            callocCount += 1;
            *heapTrace << insCount << "\t" << name << "(" << n << ", " << size << ") -> ";
        }
    }
}

VOID RecordRet(ADDRINT ret)
{
    // The return value is recorded only when flag is true
    if (flag) {
        flag = false;
        *heapTrace << (VOID*)ret << endl;
    }
}

VOID Image(IMG img, VOID* v)
{
    if (IMG_IsMainExecutable(img)) {
        bin = IMG_Name(img);
        loadedAddress = (VOID*)IMG_StartAddress(img);
        codeAddress = (VOID*)IMG_LowAddress(img);

        *trace << "fileName: " << bin << endl;
        *trace << "loadedAddress: " << loadedAddress << endl;
        *trace << "codeAddress: " << codeAddress << endl;
        *trace << "trace: " << endl;

        *memoryTrace << "fileName: " << bin << endl;
        *memoryTrace << "loadedAddress: " << loadedAddress << endl;
        *memoryTrace << "codeAddress: " << codeAddress << endl;
        *memoryTrace << "memoryTrace: " << endl;

        *heapTrace << "fileName: " << bin << endl;
        *heapTrace << "heapTrace: " << endl;
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
            IARG_CONST_CONTEXT, 
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
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0, 
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1, 
            IARG_CONST_CONTEXT, 
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
            IARG_CONST_CONTEXT, 
            IARG_END
        );
        RTN_Close(freeRtn);
    }
}

VOID RecordIns(VOID* address, string disassemble)
{
    insCount += 1;
    *trace << insCount << "\t" << address << ": " << disassembleCode[address] << endl;
}

VOID RecordMemRead(VOID* address, VOID* targetAddress, UINT32 size)
{
    readCount += 1;
    *memoryTrace << insCount << "\t" << address << " R " << targetAddress << " " << size << endl;
}

VOID RecordMemWrite(VOID* address, VOID* targetAddress, UINT32 size)
{
    writeCount += 1;
    *memoryTrace << insCount << "\t" << address << " W " << targetAddress << " " << size << endl;
}

VOID Trace(INS ins, VOID* v)
{
    // Stake insertion for all read and write instructions, not just for the main program.
    // read memory
    if (INS_IsMemoryRead(ins)) {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, 
            (AFUNPTR)RecordMemRead, 
            IARG_INST_PTR, 
            IARG_MEMORYREAD_EA, 
            IARG_MEMORYREAD_SIZE,
            IARG_END
        );
    }
    // write memory
    if (INS_IsMemoryWrite(ins)) {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, 
            (AFUNPTR)RecordMemWrite, 
            IARG_INST_PTR, 
            IARG_MEMORYWRITE_EA, 
            IARG_MEMORYWRITE_SIZE,
            IARG_END
        );
    }

    IMG img = IMG_FindByAddress(INS_Address(ins));
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
}

int main(int argc, char* argv[])
{
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
    cerr << "Trace File: " << traceFile << endl;
    cerr << "Memory Trace File: " << memoryTraceFile << endl;
    cerr << "Heap Trace File: " << heapTraceFile << endl;
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
