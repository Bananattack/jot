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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jot_source.h"

enum
{
    FILE_SOURCE_BUFFER_SIZE = 1024
};

typedef struct
{
    FILE* file;
    char buffer[FILE_SOURCE_BUFFER_SIZE];
} FileSourceHandle;

static const char* FileSourceReader(jot_Source* source, size_t* bytes_read)
{
    FileSourceHandle* handle = source->handle;
    *bytes_read = fread(handle->buffer, 1, FILE_SOURCE_BUFFER_SIZE, handle->file);
    return handle->buffer;
}

jot_Source* jot_FileSourceNew(const char* filename)
{
    jot_Source* self;
    FileSourceHandle* handle;
    
    handle = malloc(sizeof(FileSourceHandle));
    if(handle == NULL)
    {
        return NULL;
    }
    
    handle->file = fopen(filename, "rb");
    if(handle->file == NULL)
    {
        free(handle);
        return NULL;
    }
    
    self = malloc(sizeof(jot_Source));
    if(self)
    {
        self->name = filename;
        self->handle = handle;
        self->reader = FileSourceReader;
        return self;
    }
    else
    {
        fclose(handle->file);
        free(handle);
        return NULL;
    }
}

void jot_FileSourceFree(jot_Source* self)
{
    FileSourceHandle* handle = self->handle;
    fclose(handle->file);
    free(handle);
    free(self);
}
