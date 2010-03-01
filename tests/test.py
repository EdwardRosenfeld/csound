#!/usr/bin/python

# Csound5 Test Suite
# By Steven Yi<stevenyi at gmail dot com>

import os
import sys

from testUI import TestApplication
from Tkinter import *

parserType = "--new-parser"
showUIatClose = False

class Test:
    def __init__(self, fileName, description, expected=True):
        self.fileName = fileName
        self.description = ""
        self.expected = expected

def showUI(results):
    root = Tk()
    app = TestApplication(master=root)
    app.setResults(results)
    app.mainloop()
    root.destroy()

def showHelp():
    message = """Csound Test Suite by Steven Yi<stevenyi@gmail.com>

    Runs tests using new parser and shows return values of tests. Results
    are written to results.txt file.  To show the results using a UI, pass
    in the command "--show-ui" like so:

    ./test.py --show-ui
    
    The test suite defaults to using the new parser.  To use the old parser for 
    the tests, use "--old-parser" in the command like so:
    
    ./test.py --show-ui --old-parser
    
    """

    print message

def runTest():
    runArgs = "-Wdo test.wav"

    if (parserType == "--old-parser"):
        print "Testing with old parser"
    else:
        print "Testing with new parser"

    tests = [
        ["test1.csd", "Simple Test, Single Channel"],
        ["test2.csd", "Simple Test, 2 Channel"],
        ["test3.csd", "Simple Test, using i-rate variables, 2 Channel"],
        ["test4.csd", "Simple Test, using k-rate variables, 2 Channel"],
        ["test5.csd", "Simple Test, using global i-rate variables, 2 Channel"],
        ["test6.csd", "Testing Pfields"],
        ["test7.csd", "Testing expressions, no functions"],
        ["test8.csd", "Testing multi-part expressions, no functions"],
        ["test9.csd", "Unused Label (to test labels get parsed)"],
        ["test10.csd", "kgoto going to a label"],
        ["test11.csd", "if-kgoto going to a label, boolean expressions"],
        ["test12.csd", "Simple if-then statement"],
        ["test13.csd", "function call"],
        ["test14.csd", "polymorphic test, 0xffff (init)"],
        ["test15.csd", "pluck test, 0xffff (init)"],
        ["test16.csd", "Simple if-then with multiple statements in body"],
        ["test17.csd", "Simple if-then-else with multiple statements in body"],
        ["test18.csd", "if-then-elseif with no else block"],
        ["test19.csd", "if-elseif-else"],
        ["test20.csd", "if-elseif-else with inner if-elseif-else blocks"],
        ["test21.csd", "if-elseif-else with multiple elseif blocks"],
        ["test22.csd", "simple UDO"],
        ["test23.csd", "named instrument"],
##        ["test24.csd", "la_i opcodes"],
        ["test25.csd", "polymorphic test, 0xfffd (peak)"],
        ["test26.csd", "polymorphic test, 0xfffc (divz)"],
        ["test27.csd", "polymorphic test, 0xfffb (chnget)"],
        ["test28.csd", "label test"],
        ["test29.csd", "bit operations test"],
        ["test30.csd", "multi-numbered instrument test"],
        ["test31.csd", "i-rate conditional test"],
        ["test32.csd", "continuation lines test"],
        ["test33.csd", "using named instrument from score (testing score strings)"],
        ["test34.csd", "tertiary conditional expressions"],
        ["test35.csd", "*** UNKNOWN ***"],
        ["test36.csd", "opcode with all input args optional (passign)"],
    ]


    output = ""
    tempfile = "/tmp/csound_test_output.txt"
    counter = 1

    retVals = []

    testPass = 0
    testFail = 0

    for t in tests:
        filename = t[0]
        desc = t[1]

        if(os.sep == '\\'):
            command = "..\\csound.exe %s %s %s 2> %s"%(parserType, runArgs, filename, tempfile)
            retVal = os.system(command)
        else:
            command = "../csound %s %s %s &> %s"%(parserType, runArgs, filename, tempfile)
            retVal = os.system(command)

        print "Test %i: %s (%s)\nReturn Code: %i\n"%(counter, desc, filename, retVal)

        if retVal == 0:
            testPass += 1
        else:
            testFail += 1

        output += "%s\n"%("=" * 80)
        output += "Test %i: %s (%s)\nReturn Code: %i\n"%(counter, desc, filename, retVal)
        output += "%s\n\n"%("=" * 80)

        f = open(tempfile, "r")

        csOutput = ""

        for line in f:
            csOutput += line

        output += csOutput

        f.close()

        retVals.append(t + [retVal, csOutput])

        output += "\n\n"
        counter += 1

#    print output

    print "%s\n\n"%("=" * 80)
    print "Tests Passed: %i\nTests Failed: %i\n"%(testPass, testFail)


    f = open("results.txt", "w")
    f.write(output)
    f.flush()
    f.close()

    return retVals

if __name__ == "__main__":
    if(len(sys.argv) > 1):
        for arg in sys.argv:
            if (arg == "--help"):
                showHelp()
                sys.exit(0)
            elif arg == "--show-ui":
                showUIatClose = True
            elif arg == "--old-parser":
                parserType = "--old-parser"
    results = runTest()
    if (showUIatClose):
        showUI(results)
