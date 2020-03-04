'''
#                                                                                    
#       Date: 04.06.2015                                                              
#                                                                                    
#       Description: Replaces all tabs with spaces from within a directory/file.                                                                                 
#                                                                                    
#       Usage: untabify.py $path0 [... $pathn]     
#              $pathi can be either a directory or a single file                                       
'''

import sys
import os
import re

def UntabifyFile(PathToFile):
    f = open( PathToFile, 'r' )
    
    untabifiedBuffer = re.sub( r'\t', r'    ', f.read() )
    
    f.close()
    
    f = open( PathToFile, 'w' )

    f.write(untabifiedBuffer)
    
    f.close()
    
def UntabifyDirectory(PathToDirectory):
    for root, dirs, files in os.walk(PathToDirectory):
        for file in files:
            UntabifyFile(os.path.join(root,file))
            
if __name__ == "__main__":
    argv = sys.argv
    argc = len(argv)

    print "Number of arguments: %d" % argc
    print "Arguments: [%s]" % argv
    
    if( argc < 2 ):
        print "Usage: %s $path0 [... $pathn]" % argv[0]
        sys.exit(1)
    
    for arg in argv[1:]:
        if os.path.isfile(arg):
            print "File Detected"
            UntabifyFile(arg)
        elif os.path.isdir(arg):
            print "Directory Detected"
            UntabifyDirectory(arg)
        else:
            print "Invalid argument"
            sys.exit(1)
            
    sys.exit(0)