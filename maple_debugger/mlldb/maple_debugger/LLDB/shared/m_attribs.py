import ctypes
c_uint32 = ctypes.c_uint32

class Flags_bits( ctypes.LittleEndianStructure ):
    _fields_ = [
                ("logout",     c_uint32, 1 ),  # asByte & 1
                ("userswitch", c_uint32, 1 ),  # asByte & 2
                ("suspend",    c_uint32, 1 ),  # asByte & 4
                ("idle",       c_uint32, 1 ),  # asByte & 8
               ]

class Flags( ctypes.Union ):
    _anonymous_ = ("bit",)
    _fields_ = [
                ("bit",    Flags_bits ),
                ("asByte", c_uint32    )
               ]

flags = Flags()
flags.asByte = 0x2  # ->0010

print( "logout: %i"      % flags.bit.logout   )
# `bit` is defined as anonymous field, so its fields can also be accessed directly:
print( "logout: %i"      % flags.logout     )
print( "userswitch:  %i" % flags.userswitch )
print( "suspend   :  %i" % flags.suspend    )
print( "idle  : %i"      % flags.idle       )
