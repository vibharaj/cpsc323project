# lexical analyzer, parser, and semantic analyzer

CPSC 323 Programming Project 3 
Lexical Analyzer, Parser, and Semantic Analyzer in C

Teammates:
    
    1. Vibha Rajagopalan - vibha@csu.fullerton.edu

    2. Brenda Gomez - gomezb636@csu.fullerton.edu

    3. Anjali patel - anpatel8@csu.fullerton.edu

Description: 

    Our project has a Lexical Analyzer, a Parser, and a Semantic Analyzer. Our Parser takes in the output from
    the Lexical Analyzer and generates a Parse tree. Our Semantic Analyzer takes the output from the Parser and 
    makes sure that it is semantically consistent. This is also known as the third phase of
    the compiler desing process.
    
*All output files have been provided, but if you wish to test on your own, below are the 
instructions to follow using Visual Studio*

How to Run:

    1. Open up three command prompts; one for lexical analyzer, one for parser, and one for the semantic analyzer

    2. Before running, make sure to build the Lex.sln in the lexicalAnalyzer folder, parser.sln, and Semantic Analyzer.sln 
    in the parser - Copy folder to generate the executable files

    3. In first command prompt, once in parser - Copy directory:
        a. cd lexicalAnalyzer/Debug
        b. lexicalAnalyzer prog1.t lexOutput1.txt
           (lexicalAnalyzer existingTestFile.t fileToBeCreated.txt)

    4. After the above line has ran, copy the lexOutput1.txt file into the parser/x64/Debug folder in the parser -    Copy folder

    5. In second command prompt, once in parser directory:
        a. cd parser/x64/Debug
        b. parser lexOutput1.txt > parserOutput1.txt
           (parser existingFile.txt > fileToBeCreated.txt)

    6. Final output from parser can be found in "parserOutput1.txt" file in the current directory

    7. After that, copy the parserOutput1.txt file into the SemanticAnalyzer/SemanticAnalyzer/Debug folder in the SemanticAnalyzer folder
    
    8. In third command prompt, once in SemanticAnalyzer directory:
        a. cd SemanticAnalyzer/SemanticAnalyzer/Debug
        b. SemanticAnalyzer parserOutput1.txt > semanticOutput1.txt
           (SemanticAnalyzer existingFile.txt > fileToBeCreated.txt)

    9. Repeat steps 3 - 8 to run prog2.t and make lexOutput2.txt, parserOutput2.txt, and semanticOutput2.txt
    

    NOTE: 
        
        1. prog1 purposely has a variable not defined in order to catch the error in the Semantic Analyzer

        2. prog2 purposely has a type mismatch error in order to catch the error in the Semantic Analyzer


Link to github repository: https://github.com/vibharaj/cpsc323project
