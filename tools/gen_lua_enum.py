# Would it have been faster to manually copy these enums to a script? Maybe...
# Was this more interesting to write than that? Definitely!

import sys

def convert_next_enum( currLineNumber, lines ):
    # find the next enum start
    for i in range(currLineNumber, len(lines)):
        if '{' in lines[i]:
            currLineNumber = i+1
            break

    # remove irrelevant lines
    firstLine = lines[currLineNumber]
    lines = lines[currLineNumber+1:]
    lua_out = ""

    # get the first number of the enum
    index = 0
    start_symbols = firstLine.split()
    if len(start_symbols) > 1:
        for i in range(len(start_symbols)):
            if i == len(start_symbols) -1:
                number = start_symbols[i]  
                number = number[:-1]
                index = int(number)
                lua_out += str(index) + '\n'
            else:   
                lua_out += start_symbols[i]
    else:
        lua_out += firstLine[:-1] + '=' + str(index) + '\n'
    currLineNumber += 1
    # generate the rest of it
    for line in lines:
        currLineNumber += 1
        if '}' in line:
            break
        else:
            line = line.strip()
            index += 1
            if ',' in line:
                lua_out += line[:-1]
            else:
                lua_out += line
            lua_out += '=' + str(index) + '\n'

    return ( currLineNumber, lua_out )

# ------------------------------- driver code -------------------------------------------

# get & open file
filename = ''
if  len(sys.argv) == 1:
    filename = 'in.c'
else:
    filename = sys.argv[1]
inputfile = open(filename, 'r')

# split into lines
lines = inputfile.readlines()

lineNum = 0
lua_out = ""
while lineNum < len(lines):
    lineNum, lua_enum = convert_next_enum(lineNum, lines)
    lua_out += lua_enum + '\n'

print(lua_out)

lua_file = open('result.lua', 'w')
lua_file.write(lua_out)