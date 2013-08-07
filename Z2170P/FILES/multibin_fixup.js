//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//******************************************************************************
// multibin.js
//

//
// This script modifies the region specifier for each entry in 
// the input BIB file as specified in the input filelist. This allows 
// us to specify which files in the BIB file go into which region in
// a multibin scenario
//

//******************************************************************************
// Constants
//

var INPUT_FILE          = "ce.bib";
var OUTPUT_FILE         = "multiregion.bib";
var PAD_TEXT            = "    ";


//******************************************************************************
// Global data lists
//

// The 'fileTable' is a lookup table used to determine the proper memory region
// for a module or file. It is pre-populated with files which will always
// need to be located in the default kernel region (NK).
//
var fileTable = new ActiveXObject("Scripting.Dictionary");
fileTable.CompareMode = 1;

 
//******************************************************************************
// Global objects and variables
//
var shellObj = new ActiveXObject("WScript.Shell");
var flatReleaseDir = new String(shellObj.Environment("Process")("_FLATRELEASEDIR"));
var fileSysObj = new ActiveXObject("Scripting.FileSystemObject");

var ForReading = 1;
var ForWriting = 2;
var TristateFalse = 0;

// generate source/destination .bib file names
var sourceFile = new String(flatReleaseDir + '\\' + INPUT_FILE);;
var destFile = new String(flatReleaseDir + '\\' + OUTPUT_FILE);;

// call main()
main();

//******************************************************************************
// main()
//
function main()
{
    // sanity check to make sure we were invoked using cscript.exe, not wscript.exe
    try
    {
        WScript.StdOut.WriteLine("Starting Multibin Fixup");
        WScript.StdOut.WriteLine("");
    }
    catch(e)
    {   
        WScript.echo("FAILURE: Unable to access StdOut.\nScript must be invoked by cscript.exe");
        WScript.Quit();
    }

    // parse region table
    ParseRegionTable();

    // fixup the module and file entries to a temporary file
    DoModulesAndFiles(sourceFile, destFile);

    WScript.StdOut.WriteLine("-------------------");
    WScript.StdOut.WriteLine("Done Multibin Fixup.");
    WScript.StdOut.WriteLine("");
    
    WScript.Quit();
}

// done 

//******************************************************************************
// ParseRegionTable()
//
function ParseRegionTable()
{
    var tablefilename = WScript.Arguments(0);
    WScript.StdOut.WriteLine("Parsing Region Table from File :" + tablefilename );

    if(!fileSysObj.FileExists(tablefilename))
    {
        WScript.StdErr.WriteLine("FAILURE: no such file: " + tablefilename);
        WScript.Quit();
    }

    var tableline;
    var match;
    var currentregion;
    
    // open the file if it exists
    var fileStream = fileSysObj.OpenTextFile(tablefilename, ForReading, false, TristateFalse);
    while(!fileStream.AtEndOfStream)
    {
        // read each line
        tableline = fileStream.ReadLine();

        if(match = tableline.match(/^\s*$/))
        {
            // ignore blank lines
            continue;
        }
        else if(match = tableline.match(/^\s*(REGION)\s+(\S+)$/i))
        {
            //WScript.StdOut.WriteLine("Region Match1 = " + match[1] );
            currentregion = match[2];
            WScript.StdOut.WriteLine("Current Region = " + currentregion );
        }
        else
        {
            match = tableline.match(/^\s*(\S+)\s*$/);
            if(!fileTable.Exists(match[1]))
            {
                fileTable(match[1]) = currentregion;
            }
            WScript.StdOut.WriteLine("adding " + match[1] + " to region " + currentregion);
        }

    }
    fileStream.Close();

}

//******************************************************************************
// DoModulesAndFiles()
//
function DoModulesAndFiles(inFile, outFile)
{
    var ex, inFileStream, outFileStream;
    var currentBibSection = "";
    
    if(!fileSysObj.FileExists(inFile))
    {
        // the source file doesn't exist
        WScript.StdErr.WriteLine("FAILURE: file " + inFile + " does not exist!");
        WScript.Quit();
    }
    
    // open input and output files for ASCII text
    try
    {
        inFileStream = fileSysObj.OpenTextFile(inFile, ForReading, false, TristateFalse);
    }
    catch(ex)
    {
        WScript.StdErr.WriteLine("FAILURE: failed opening file " + inFile + " for reading!");
        WScript.Quit();
    }
    
    try
    {
        outFileStream = fileSysObj.OpenTextFile(outFile, ForWriting, true, TristateFalse);
    }
    catch(ex)
    {
        WScript.StdErr.WriteLine("FAILURE: failed opening file " + outFile + " for writing!");
        WScript.Quit();
    }
    
    while(!inFileStream.AtEndOfStream)
    {
        var inline, match;
        inline = inFileStream.ReadLine();       

        if(match = inline.match(/^\s*$/))
        {
            // ignore blank lines
            // copy the input line to the output
            outFileStream.WriteLine(inline);
        }
        else if(match = inline.match(/^\s*(;|IF|ENDIF|#)\s*/i))
        {
            // ignore comments, IF, and ENDIF statements
            // copy the input line to the output
            outFileStream.WriteLine(inline);
        }        
        else if(match = inline.match(/^\s*(MODULES|FILES|MEMORY|CONFIG)\s*$/i))
        {
            // section break: MODULES, FILES, MEMORY, or CONFIG section
            currentBibSection = match[1].toUpperCase();
            outFileStream.WriteLine(inline);
        }
        else if(currentBibSection == "MODULES" || currentBibSection == "FILES")
        {
            // match a file or module entry 
            if(match = inline.match(/^\s*(\S+)\s+(\S+)\s+(\S+)\s*(\S*)/))
            {
                var exeName = match[1].toLowerCase();
                var exePath = match[2].toLowerCase();
                var region = match[3].toUpperCase();
                var flags = match[4].toUpperCase();                 

                var newRegion = "";
                
                if(fileTable.Exists(exeName))
                {
                    // there is an entry for this file in the file table                
                    newRegion = fileTable(exeName);                 

                    outFileStream.WriteLine(PAD_TEXT + PadStringRight(exeName, 16, ' ')
                        + PAD_TEXT + PadStringRight(exePath, 64, ' ')
                        + PAD_TEXT + PadStringRight(newRegion, 12, ' ')
                        + PAD_TEXT + PadStringRight(flags, 8, ' '));
                }
                else
                {
                    // copy the input line to the output
                    outFileStream.WriteLine(inline);
                }
                
            }
            else
            {
                // failed parsing a line
                WScript.StdErr.WriteLine("unable to parse line: " + inline);             
            }           
        }
        else
        {
            // copy the input line to the output
            outFileStream.WriteLine(inline);
        }
    }
    inFileStream.Close();
    outFileStream.Close();
}


//******************************************************************************
// PadStringLeft()
//
function PadStringLeft(string, size, padChar)
{
    while(string.length < size)
    {
        string = padChar + string;
    }
    return string;
}

//******************************************************************************
// PadStringRight()
//
function PadStringRight(string, size, padChar)
{
    while(string.length < size)
    {
        string = string + padChar;
    }
    return string;
}
