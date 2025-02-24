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

pp = printing.RegexpCollectionPrettyPrinter("stdx")
pp.add_printer('stdx::String', '^stdx::StringBase<char,.*>$', OsStringPrinter)
printing.register_pretty_printer(None, pp, replace=True)
