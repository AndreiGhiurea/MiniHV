'''
#                                                                                    
#       Date: 04.06.2015                                                              
#                                                                                    
#       Description: Concatenates the data from all the files from within a directory with the 
#                    specificied extensions into a single file
#                                                                                                     
#       Usage: collector.py $pathToDir $outputFile *.ext1 [*.extn]                         
'''

import sys
import os
import re

def CollectFilesFromDirectory( DirectoryPath ):
    result = []

    # collect files
    for root, dirs, files in os.walk(DirectoryPath):
        for file in files:
            result.append( os.path.join(root,file) )  
            
    return result

def ConcatenateFilesFromDirectory( DirectoryPath, OutputFile, Extensions ):
    file_list = CollectFilesFromDirectory( DirectoryPath )
    
    outFile = open( OutputFile, 'w' )
    
    for file in file_list:
        print "File [%s]" % file
        
        decorator = file.split('\\')[-1]
        lengthOfEquals = 80 - len(decorator)
        decorator = "\n%s%s%s\n" % ( ( ( lengthOfEquals / 2 ) * '=' ), decorator, ( lengthOfEquals - ( lengthOfEquals / 2 ) ) * '=' )
        lengthOfEquals = lengthOfEquals - ( lengthOfEquals / 2 )
        decorator = decorator 
        
        inFile = open(file, 'r')
        
        outFile.write( decorator )
        outFile.write( inFile.read() )
        
        inFile.close()
    
    outFile.close()
    
def UntabifyFile(PathToFile):
    f = open( PathToFile, 'r' )
    
    untabifiedBuffer = re.sub( r'\t', r'    ', f.read() )
    
    f.close()
    
    f = open( PathToFile, 'w' )

    f.write(untabifiedBuffer)
    
    f.close()
    
def UntabifyDirectory(PathToDirectory):
    for root, dirs, files in os.walk(PathToDirectory):
        for dir in dirs:
            UntabifyDirectory(dir)
        for file in files:
            UntabifyFile(os.path.join(root,file))
            
if __name__ == "__main__":
    argv = sys.argv
    argc = len(argv)

    print "Number of arguments: %d" % argc
    print "Arguments: [%s]" % argv
    
    if( argc < 4 ):
        print "Usage: %s $pathToDir $outputFile *.ext1 [*.extn]" % argv[0]
        sys.exit(1)
    
    pathToDirectory = argv[1]
    outputFile = argv[2]
    extensions = argv[3:]
    
    if not os.path.isdir(pathToDirectory):
        print "First argument [%s] is not a valid directory path" % pathToDirectory
        sys.exit(1)
    
    ConcatenateFilesFromDirectory( pathToDirectory, outputFile, extensions )
            
    sys.exit(0)