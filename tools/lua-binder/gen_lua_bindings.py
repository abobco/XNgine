import sys

def generate_argless_lua_binding( definition ):
    symbols = definition.split()

    signature = ''
    for c in symbols[1]:
        if c == '(':
            break
        signature += c

    comment = ''
    reached_comment = False
    for s in symbols:
        if s == "//":
            reached_comment = True
        if reached_comment:
            comment+=s + " "


    lua_binding = """%s
int lua_%s( lua_State *L ) {
    %s();
    return 0;
} """ % (comment, signature, signature)

    header_def = 'int lua_%s( lua_State *L ); %s' % (signature, comment)

    return ( header_def, lua_binding, signature )

# ------------------------ driver code --------------------------------------------------

# get & open file
filename = ''
if  len(sys.argv) == 1:
    filename = 'in.h'
else:
    filename = sys.argv[1]
inputfile = open(filename, 'r')
functions = inputfile.readlines()

# generate bindings
bindings = []
for f in functions:
    bindings.append(generate_argless_lua_binding(f))

# write to c files
c_header = ''
c_def = ''
c_sigs = []
for b in bindings:
    c_header += b[0] + '\n'
    c_def += b[1] + '\n'
    c_sigs.append(b[2])    

c_header += '\nstatic const struct luaL_Reg bindings[] = {\n'
for signature in c_sigs:
    lua_sig = ''
    for i in range(len(signature)):
        if i > 0 and signature[i].isupper():
            lua_sig += '_' + signature[i].lower()
        else:
            lua_sig += signature[i].lower()
    c_header += '\t{ "%s", lua_%s },\n' % (lua_sig, signature)
c_header += '\t{0,0}\t// terminator\n};'


header_file = open('result.h', 'w')
c_file = open('result.c', 'w')

header_file.write(c_header)
c_file.write(c_def)