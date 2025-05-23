import gdb
import gdb.printing as printing

class OsStringPrinter(printing.PrettyPrinter):
    "Print a stdx::String"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        front = self.__val['mFront']
        back = self.__val['mBack']
        return front.string(length=back - front)

    def display_hint(self):
        return 'string'

class VirtualAddressPrinter(printing.PrettyPrinter):
    "Print a sm::VirtualAddress"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        address = int(self.__val['address'])
        return f"mem:0x{address:x}"

class PhysicalAddressPrinter(printing.PrettyPrinter):
    "Print a sm::PhysicalAddress"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        address = int(self.__val['address'])
        return f"ram:0x{address:x}"

class MemoryRangePrinter(printing.PrettyPrinter):
    "Print an km::AnyRange<T>"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        start = self.__val['front'].format_string()
        end = self.__val['back'].format_string()
        return f"[{start}, {end})"

class AddressMappingPrinter(printing.PrettyPrinter):
    "Print a km::AddressMapping"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        paddr = self.__val['paddr']['address']
        vaddr = self.__val['vaddr']
        size = self.__val['size']
        return f"{vaddr.format_string()} -> 0x{int(paddr):x} (0x{int(size):x})"

class AtomicFlagPrinter(printing.PrettyPrinter):
    "print a std::atomic_flag"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        return "set" if self.__val['__a_']['__a_value'] else "clear"

class AtomicPrinter(printing.PrettyPrinter):
    "print a std::atomic<T>"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        return str(self.__val['__a_']['__a_value'])

pp = printing.RegexpCollectionPrettyPrinter("stdx")
pp.add_printer('stdx::String', '^stdx::StringBase<char,.*>$', OsStringPrinter)
pp.add_printer('sm::VirtualAddress', '^sm::VirtualAddress$', VirtualAddressPrinter)
pp.add_printer('sm::PhysicalAddress', '^sm::PhysicalAddress$', PhysicalAddressPrinter)
pp.add_printer('km::AnyRange', '^km::AnyRange<.*>$', MemoryRangePrinter)
pp.add_printer('km::AddressMapping', '^km::AddressMapping$', AddressMappingPrinter)
pp.add_printer('std::atomic_flag', '^std::__1::atomic_flag$', AtomicFlagPrinter)
pp.add_printer('std::atomic', '^std::__1::atomic<.*>$', AtomicPrinter)
printing.register_pretty_printer(None, pp, replace=True)
