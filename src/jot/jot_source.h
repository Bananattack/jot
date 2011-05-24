/*
    jot - Source Interface
    
    -

    Copyright (C) 2011 by Andrew G. Crowell

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
    
*/
#ifndef JOT_CHUNK_H
#define JOT_CHUNK_H

#include <stdio.h>
#include <stddef.h>

struct jot_Source;
typedef const char* jot_SourceReader(struct jot_Source* source, size_t* bytes_read);

struct jot_Source
{
    const char* name;
    void* handle;
    jot_SourceReader* reader;
};

typedef struct jot_Source jot_Source;

jot_Source* jot_FileSourceNew(const char* filename);
void jot_FileSourceFree(jot_Source* self);

#endif
