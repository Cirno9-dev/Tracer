class Info:
    def __init__(self, filename: str) -> None:
        self._parseFromFile(filename)
        
    def _parseFromFile(self, filename: str) -> None:
        with open(filename) as f:
            line = f.readline()
            while line:
                exec("self."+line.replace(": ", " = ").strip())
                line = f.readline()
    
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        info = ""
        for (name, value) in vars(self).items():
            info += f"{name}: {value}\n"
        return info.strip()
    
class Instruction(dict):
    def __init__(self, id: int, address: int, ins: str) -> None:
        super().__init__({
            "id": id,
            "address": address,
            "ins": ins
        })
        
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        id = self["id"]
        address = self["address"]
        ins = self["ins"]
        
        return f"{id}\t{hex(address)}: {ins}"

class Trace(list):
    def __init__(self, filename: str) -> None:
        super().__init__([])
        self._parseFromFile(filename)
    
    def _parseFromFile(self, filename: str) -> None:
        with open(filename) as f:
            line = f.readline()
            while line:
                line = line.strip()
                
                id = int(line.split("\t")[0])
                address = int(line.split("\t")[1].split(": ")[0], 16)
                ins = line.split("\t")[1].split(": ")[1]
                self.append(Instruction(id, address, ins))
                
                line = f.readline()
                
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        trace = ""
        for ins in self:
            trace += str(ins) + "\n"
        return trace.strip()
    
class MemoryOp(dict):
    def __init__(self, id: int, opId: int, address: int, op: str, targetAddress: int, size: int) -> None:
        super().__init__({
            "id": id,
            "opId": opId,
            "address": address,
            "op": op,
            "targetAddress": targetAddress,
            "size": size
        })
    
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        id = self["id"]
        opId = self["opId"]
        address = self["address"]
        op = self["op"]
        targetAddress = self["targetAddress"]
        size = self["size"]
        
        return f"{id}\t{opId}\t{hex(address)} {op} {hex(targetAddress)} {size}"
    
class MemoryTrace(list):
    def __init__(self, filename: str) -> None:
        super().__init__([])
        self._parseFromFile(filename)

    def _parseFromFile(self, filename: str) -> None:
        with open(filename) as f:
            while True:
                line = f.readline()
                if not line:
                    break
                line = line.strip()
                
                id = int(line.split("\t")[0])
                if id == 0:
                    continue
                opId = int(line.split("\t")[1])
                line = line.split("\t")[2].split(" ")
                line[0] = int(line[0], 16)
                line[2] = int(line[2], 16)
                line[3] = int(line[3])
                self.append(MemoryOp(id, opId, *line))

    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        memoryTrace = ""
        for memOp in self:
            memoryTrace += str(memOp) + "\n"
        return memoryTrace.strip()
    
class HeapOp(dict):
    def __init__(self, id: int, opId: int, func: str, args: list, retAddress: int) -> None:
        super().__init__({
            "id": id,
            "opId": opId,
            "func": func,
            "args": args,
            "retAddress": retAddress
        })
        
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        id = self["id"]
        opId = self["opId"]
        func = self["func"]
        args = self["args"]
        retAddress = self["retAddress"]
        
        argsStr = ", ".join([hex(arg) for arg in args])
        
        return f"{id}\t{opId}\t{hex(retAddress)} = {func}({argsStr})"
    
class HeapTrace(list):
    def __init__(self, filename: str) -> None:
        super().__init__([])
        self._parseFromFile(filename)

    def _parseFromFile(self, filename: str) -> None:
        with open(filename) as f:
            while True:
                line = f.readline()
                if not line:
                    break
                line = line.strip()
                
                id = int(line.split("\t")[0])
                # if id == 0 pass
                if id == 0:
                    continue
                opId = int(line.split("\t")[1])
                line = line.split("\t")[2].split(" ")
                line[1] = eval(line[1])
                line[2] = int(line[2], 16)
                # if free(0) or 0x0 = malloc() pass
                if line[0] == "free":
                    if line[1] == [0]:
                        continue
                elif line[2] == 0:
                    continue
                self.append(HeapOp(id, opId, *line))
                
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        heapTrace = ""
        for heapOp in self:
            heapTrace += str(heapOp) + "\n"
        return heapTrace.strip()
