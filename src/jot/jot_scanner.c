/*
    jot - Lexical Scanner
    
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

#define JOT_END_OF_STREAM (char)(-1)

enum
{
    JOT_SCAN_BUFFER_SIZE = 256,
    JOT_SCAN_MIN_TEXT_CAPACITY = 32
};

typedef enum
{
    JOT_STATE_START,
    JOT_STATE_NEWLINE,
    JOT_STATE_ZERO,
    JOT_STATE_INT,
    JOT_STATE_HEX,
    JOT_STATE_BIN,
    JOT_STATE_NUM,
    JOT_STATE_E_SIGN,
    JOT_STATE_E_VALUE,
    JOT_STATE_STR,
    JOT_STATE_STR_BACKSLASH,
    JOT_STATE_STR_HEX_HIGH,
    JOT_STATE_STR_HEX_LOW,
    JOT_STATE_IDENTIFIER,
    JOT_STATE_HASH,
    JOT_STATE_HASH_COMMENT,
    JOT_STATE_HASH_HASH_COMMENT,
    JOT_STATE_HASH_HASH_COMMENT_HASH,
    JOT_STATE_DOT,
    JOT_STATE_DOT_DOT,
    JOT_STATE_LT,
    JOT_STATE_GT,
    JOT_STATE_EQ,
    JOT_STATE_ASTERISK,
    JOT_STATE_DASH,
    JOT_STATE_EXCLAIM
} jot_ScanState;

typedef enum
{
    JOT_TOKEN_NONE,
    JOT_TOKEN_EOF,
    JOT_TOKEN_ERROR,
    JOT_TOKEN_NUM,
    JOT_TOKEN_INT,
    JOT_TOKEN_HEX,
    JOT_TOKEN_BIN,
    JOT_TOKEN_STR,
    JOT_TOKEN_IDENTIFIER,
    JOT_TOKEN_COLON,
    JOT_TOKEN_SEMICOLON,
    JOT_TOKEN_DOT,
    JOT_TOKEN_DOT_DOT,
    JOT_TOKEN_DOT_DOT_DOT,
    JOT_TOKEN_COMMA,
    JOT_TOKEN_LPAREN,
    JOT_TOKEN_RPAREN,
    JOT_TOKEN_LBRACKET,
    JOT_TOKEN_RBRACKET,
    JOT_TOKEN_LBRACE,
    JOT_TOKEN_RBRACE,
    JOT_TOKEN_ASSIGN,
    JOT_TOKEN_EXCLAIM,
    JOT_TOKEN_CMP_EQ,
    JOT_TOKEN_CMP_LT,
    JOT_TOKEN_CMP_LE,
    JOT_TOKEN_CMP_GT,
    JOT_TOKEN_CMP_GE,
    JOT_TOKEN_CMP_NE,
    JOT_TOKEN_ADD,
    JOT_TOKEN_SUB,
    JOT_TOKEN_MUL,
    JOT_TOKEN_DIV,
    JOT_TOKEN_MOD,
    JOT_TOKEN_EXP,
    JOT_TOKEN_AND,
    JOT_TOKEN_OR,
    JOT_TOKEN_XOR,
    JOT_TOKEN_TILDE,
    JOT_TOKEN_SHL,
    JOT_TOKEN_SHR,
    JOT_TOKEN_ARROW
} jot_Token;

const char* token_name[] = {
    "(none?)",
    "end-of-file",
    "error",
    "number literal",
    "integer literal",
    "hexadecimal integer literal",
    "binary integer literal",
    "string literal",
    "identifier",
    "':'",
    "';'",
    "'.'",
    "'..'",
    "'...'",
    "','",
    "'('",
    "')'",
    "'['",
    "']'",
    "'{'",
    "'}'",
    "'='",
    "'!'",
    "'=='",
    "'<'",
    "'<='",
    "'>'",
    "'>='",
    "'!='",
    "'+'",
    "'-'",
    "'*'",
    "'/'",
    "'%'",
    "'**'",
    "'&'",
    "'|'",
    "'^'",
    "'~'",
    "'<<'",
    "'>>'",
    "'->'"
};

typedef enum
{
    JOT_KEYWORD_NONE,
    JOT_KEYWORD_AND,
    JOT_KEYWORD_BREAK,
    JOT_KEYWORD_CONTINUE,
    JOT_KEYWORD_DO,
    JOT_KEYWORD_ELSE,
    JOT_KEYWORD_ELSEIF,
    JOT_KEYWORD_END,
    JOT_KEYWORD_FALSE,
    JOT_KEYWORD_FOR,
    JOT_KEYWORD_FUNC,
    JOT_KEYWORD_IF,
    JOT_KEYWORD_IN,
    JOT_KEYWORD_NIL,
    JOT_KEYWORD_NOT,
    JOT_KEYWORD_OR,
    JOT_KEYWORD_REPEAT,
    JOT_KEYWORD_THEN,
    JOT_KEYWORD_TRUE,
    JOT_KEYWORD_UNTIL,
    JOT_KEYWORD_WHILE,
    JOT_KEYWORD_VAR
} jot_Keyword;

typedef struct
{
    jot_Source* source;
    char end_of_file;
    char terminator;
    char intermediate;
    jot_ScanState previous_state;
    jot_ScanState state;
    size_t line;
    size_t comment_line;
    
    size_t position;
    size_t buffer_size;
    const char* buffer;
    
    size_t text_length;
    size_t text_capacity;
    char* text;
    
    size_t last_text_length;
    size_t last_text_capacity;
    char* last_text;
} jot_Scanner;

jot_Scanner* jot_ScannerNew(jot_Source* source)
{
    jot_Scanner* self = malloc(sizeof(jot_Scanner));
    
    self->source = source;
    self->end_of_file = 0;
    self->state = JOT_STATE_START;
    self->terminator = 0;
    self->position = 0;
    self->buffer_size = 0;
    self->line = 1;
    self->comment_line = 0;
    
    self->text_length = 0;
    self->text_capacity = JOT_SCAN_MIN_TEXT_CAPACITY;
    self->text = malloc(self->text_capacity);
    
    self->last_text_length = 0;
    self->last_text_capacity = JOT_SCAN_MIN_TEXT_CAPACITY;
    self->last_text = malloc(self->last_text_capacity);
    
    return self;
}

void jot_ScannerFree(jot_Scanner* self)
{
    free(self->last_text);
    free(self->text);
    free(self);
}

void jot_ScannerAddTextChar(jot_Scanner* self, char c)
{
    if(self->text_length == self->text_capacity)
    {
        self->text_capacity <<= 1;
        self->text = realloc(self->text, self->text_capacity);
    }
    self->text[self->text_length++] = c;
}

void jot_ScannerFlushText(jot_Scanner* self)
{
    if(self->last_text_capacity < self->text_capacity)
    {
        self->last_text_capacity = self->text_capacity;
        self->last_text = realloc(self->last_text, self->last_text_capacity);
    }
    memcpy(self->last_text, self->text, self->text_length);
    self->last_text_length = self->text_length;
    self->text_length = 0;
}

jot_Token jot_ScannerNext(jot_Scanner* self)
{
    char c;
    
    if(self->source == NULL)
    {
        return JOT_TOKEN_EOF;
    }
    do
    {
        while(self->position < self->buffer_size)
        {
            if(self->end_of_file)
            {
                c = JOT_END_OF_STREAM;
            }
            else
            {
                c = self->buffer[self->position];
            }
            
            switch(self->state)
            {
                case JOT_STATE_START:
                    switch(c)
                    {
                        case '0':
                            self->state = JOT_STATE_ZERO;
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '1': case '2': case '3':
                        case '4': case '5': case '6':
                        case '7': case '8': case '9':
                            /* '1' .. '9' */
                            self->state = JOT_STATE_INT;
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '\'':
                        case '\"':
                            self->terminator = c;
                            self->state = JOT_STATE_STR;
                            break;
                        case 'a': case 'b': case 'c': case 'd': case 'e':
                        case 'f': case 'g': case 'h': case 'i': case 'j':
                        case 'k': case 'l': case 'm': case 'n': case 'o':
                        case 'p': case 'q': case 'r': case 's': case 't':
                        case 'u': case 'v': case 'w': case 'x': case 'y':
                        case 'z': case '_': case 'A': case 'B': case 'C':
                        case 'D': case 'E': case 'F': case 'G': case 'H':
                        case 'I': case 'J': case 'K': case 'L': case 'M':
                        case 'N': case 'O': case 'P': case 'Q': case 'R':
                        case 'S': case 'T': case 'U': case 'V': case 'W':
                        case 'X': case 'Y': case 'Z':
                            /* 'a' .. 'z' | '_' | 'A' .. 'Z' */
                            self->state = JOT_STATE_IDENTIFIER;
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case ' ': case '\t':
                        case '\r': case '\n':
                            /* Ignore whitespace (lines are tracked elsewhere) */
                            break;
                        case '#':
                            self->state = JOT_STATE_HASH;
                            break;
                        case ':': self->position++; return JOT_TOKEN_COLON;
                        case ';': self->position++; return JOT_TOKEN_SEMICOLON;
                        case '.':
                            self->state = JOT_STATE_DOT;
                            break;
                        case ',': self->position++; return JOT_TOKEN_COMMA;
                        case '(': self->position++; return JOT_TOKEN_LPAREN;
                        case ')': self->position++; return JOT_TOKEN_RPAREN;
                        case '[': self->position++; return JOT_TOKEN_LBRACKET;
                        case ']': self->position++; return JOT_TOKEN_RBRACKET;
                        case '{': self->position++; return JOT_TOKEN_LBRACE;
                        case '}': self->position++; return JOT_TOKEN_RBRACE;
                        case '<':
                            self->state = JOT_STATE_LT;
                            break;
                        case '>':
                            self->state = JOT_STATE_GT;
                            break;
                        case '=':
                            self->state = JOT_STATE_EQ;
                            break;
                        case '*':
                            self->state = JOT_STATE_ASTERISK;
                            break;
                        case '-':
                            self->state = JOT_STATE_DASH;
                            break;
                        case '!':
                            self->state = JOT_STATE_EXCLAIM;
                            break;
                        case '+': self->position++; return JOT_TOKEN_ADD;
                        case '/': self->position++; return JOT_TOKEN_DIV;
                        case '%': self->position++; return JOT_TOKEN_MOD;
                        case '&': self->position++; return JOT_TOKEN_AND;
                        case '|': self->position++; return JOT_TOKEN_OR;
                        case '^': self->position++; return JOT_TOKEN_XOR;
                        case '~': self->position++; return JOT_TOKEN_TILDE;                        
                        case JOT_END_OF_STREAM: break;
                        default:
                            /* error: illegal character %c (%d) found. */
                            return JOT_TOKEN_ERROR;
                    }
                    break;
                case JOT_STATE_ZERO:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '0' .. '9' */
                            self->state = JOT_STATE_INT;
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '_':
                            /* '_' place separator -- skip over, but switch states */
                            self->state = JOT_STATE_INT;
                            break;
                        case '.':
                            /* '.' decimal point */
                            jot_ScannerAddTextChar(self, c);
                            self->state = JOT_STATE_NUM;
                            break;
                        case 'e': case 'E':
                            /* exp */
                            jot_ScannerAddTextChar(self, c);
                            self->state = JOT_STATE_E_SIGN;
                            break;
                        case 'x':
                            /* hex literal */
                            self->state = JOT_STATE_HEX;
                            break;
                        case 'b':
                            /* binary literal */
                            self->state = JOT_STATE_BIN;
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_INT;
                    }
                    break;
                case JOT_STATE_INT:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '0' .. '9' */
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '_':
                            /* '_' place separator -- skip over */
                            break;
                        case '.':
                            /* '.' decimal point */
                            jot_ScannerAddTextChar(self, c);
                            self->state = JOT_STATE_NUM;
                            break;
                        case 'e': case 'E':
                            /* exp */
                            jot_ScannerAddTextChar(self, c);
                            self->state = JOT_STATE_E_SIGN;
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_INT;
                    }
                    break;
                case JOT_STATE_NUM:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '0' .. '9' */
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '_':
                            /* '_' place separator -- skip over */
                            break;
                        case 'e': case 'E':
                            /* exp */
                            jot_ScannerAddTextChar(self, c);
                            self->state = JOT_STATE_E_SIGN;
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_NUM;
                    }
                    break;
                case JOT_STATE_E_SIGN:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                        case '+': case '-':
                            /* ('+'|'-')? digits */
                            jot_ScannerAddTextChar(self, c);
                            self->state = JOT_STATE_E_VALUE;
                            break;
                        case '_':
                            /* '_' place separator -- skip over, but switch states */
                            self->state = JOT_STATE_E_VALUE;
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_NUM;
                    }
                    break;
                case JOT_STATE_E_VALUE:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '0' .. '9' */
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '_':
                            /* '_' place separator -- skip over */
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_NUM;
                    }
                    break;
                case JOT_STATE_HEX:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                            /* '0' .. '9' | 'a' .. 'f' | 'A' .. 'F' */
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '_':
                            /* '_' place separator -- skip over */
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_HEX;
                    }
                    break;
                case JOT_STATE_BIN:
                    switch(c)
                    {
                        case '0': case '1':
                            /* '0' .. '1' */
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '_':
                            /* '_' place separator -- skip over */
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_BIN;
                    }
                    break;
                case JOT_STATE_STR:
                    switch(c)
                    {
                        case '\"':
                        case '\'':
                            if(c == self->terminator)
                            {
                                self->state = JOT_STATE_START;
                                self->position++;
                                jot_ScannerFlushText(self);
                                return JOT_TOKEN_STR;
                            }
                            else
                            {
                                jot_ScannerAddTextChar(self, c);
                            }
                            break;
                        case '\\':
                            self->state = JOT_STATE_STR_BACKSLASH;
                            break;
                        case '\r':
                        case '\n':
                            /* error: expected closing {{terminator}} but got end-of-line. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        case JOT_END_OF_STREAM:
                            /* error: expected closing {{terminator}} but got end-of-file. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        default:
                            jot_ScannerAddTextChar(self, c);
                            break;
                    }
                    break;
                case JOT_STATE_STR_BACKSLASH:
                    self->state = JOT_STATE_STR;
                    switch(c)
                    {
                        case '\\':
                        case '\'':
                        case '\"':
                            jot_ScannerAddTextChar(self, c); break;
                        case 'a': jot_ScannerAddTextChar(self, '\a'); break;
                        case 'b': jot_ScannerAddTextChar(self, '\b'); break;
                        case 'f': jot_ScannerAddTextChar(self, '\f'); break;
                        case 'n': jot_ScannerAddTextChar(self, '\n'); break;
                        case 'r': jot_ScannerAddTextChar(self, '\r'); break;
                        case 't': jot_ScannerAddTextChar(self, '\t'); break;
                        case 'v': jot_ScannerAddTextChar(self, '\v'); break;
                        case 'x':
                            self->intermediate = 0;
                            self->state = JOT_STATE_STR_HEX_HIGH;
                            break;
                        case '\r':
                        case '\n':
                            /* error: expected escape sequence but got end-of-line. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        case JOT_END_OF_STREAM:
                            /* error: expected escape sequence but got end-of-file. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        default:
                            /* error: invalid escape sequence. */
                            break;
                    }
                    break;
                case JOT_STATE_STR_HEX_HIGH:
                    self->state = JOT_STATE_STR_HEX_LOW;
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '0' .. '9' */
                            self->intermediate = (c - '0') << 4;
                            break;
                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                            /* 'a' .. 'f' */
                            self->intermediate = (c - 'a' + 10) << 4;
                            break;
                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                            /* 'A' .. 'F' */
                            self->intermediate = (c - 'A' + 10) << 4;
                            break;
                        case '\r':
                        case '\n':
                            /* error: expected hex escape sequence but got end-of-line. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        case JOT_END_OF_STREAM:
                            /* error: expected hex escape sequence but got end-of-file. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        default:
                            /* error: invalid hex digit in escape sequence. */
                            self->state = JOT_STATE_STR;
                            break;
                    }
                    break;
                case JOT_STATE_STR_HEX_LOW:
                    self->state = JOT_STATE_STR;
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '0' .. '9' */
                            self->intermediate |= c - '0';
                            jot_ScannerAddTextChar(self, self->intermediate);
                            break;
                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                            /* 'a' .. 'f' */
                            self->intermediate |= c - 'a' + 10;
                            jot_ScannerAddTextChar(self, self->intermediate);
                            break;
                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                            /* 'A' .. 'F' */
                            self->intermediate |= c - 'A' + 10;
                            jot_ScannerAddTextChar(self, self->intermediate);
                            break;
                        case '\r':
                        case '\n':
                            /* error: expected hex escape sequence but got end-of-line. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        case JOT_END_OF_STREAM:
                            /* error: expected hex escape sequence but got end-of-file. */
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_STR;
                        default:
                            /* error: invalid hex digit in escape sequence. */
                            break;
                    }
                    break;
                case JOT_STATE_IDENTIFIER:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                        case 'a': case 'b': case 'c': case 'd': case 'e':
                        case 'f': case 'g': case 'h': case 'i': case 'j':
                        case 'k': case 'l': case 'm': case 'n': case 'o':
                        case 'p': case 'q': case 'r': case 's': case 't':
                        case 'u': case 'v': case 'w': case 'x': case 'y':
                        case 'z': case '_': case 'A': case 'B': case 'C':
                        case 'D': case 'E': case 'F': case 'G': case 'H':
                        case 'I': case 'J': case 'K': case 'L': case 'M':
                        case 'N': case 'O': case 'P': case 'Q': case 'R':
                        case 'S': case 'T': case 'U': case 'V': case 'W':
                        case 'X': case 'Y': case 'Z':
                            /* '0' .. '9' | 'a' .. 'z' | '_' | 'A' .. 'Z' */
                            jot_ScannerAddTextChar(self, c);
                            break;
                        default:
                            self->state = JOT_STATE_START;
                            jot_ScannerFlushText(self);
                            return JOT_TOKEN_IDENTIFIER;
                    }
                    break;
                case JOT_STATE_HASH:
                    if(c == '#')
                    {
                        /* ## block comment ##*/
                        self->state = JOT_STATE_HASH_HASH_COMMENT;
                    }
                    else
                    {
                        /* # line comment */
                        self->state = JOT_STATE_HASH_COMMENT;
                    }
                    break;
                case JOT_STATE_HASH_COMMENT:
                    /* Ignore input (end-of-line handling done elsewhere). */
                    break;
                case JOT_STATE_HASH_HASH_COMMENT:
                    /* Handle # specially (might be a closing ##), ignore all else. */
                    if(c == '#')
                    {
                        self->state = JOT_STATE_HASH_HASH_COMMENT_HASH;
                    }
                    else if(c == JOT_END_OF_STREAM)
                    {
                        /* error: expected closing ## but got end-of-file. */
                        self->state = JOT_STATE_START;
                    }
                    break;
                case JOT_STATE_HASH_HASH_COMMENT_HASH:
                    if(c == '#')
                    {
                        /* ## closes a ## */
                        self->state = JOT_STATE_START;
                    }
                    else if(c == JOT_END_OF_STREAM)
                    {
                        /* error: expected closing ## but got end-of-file. */
                        self->state = JOT_STATE_START;
                    }
                    else
                    {
                        /* False alarm, return to previous state */
                        self->state = JOT_STATE_HASH_HASH_COMMENT;
                    }
                    break;
                case JOT_STATE_DOT:
                    switch(c)
                    {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            /* '.' followed by digits, a number */
                            self->state = JOT_STATE_NUM;
                            /* Add a decimal point and then the digit */
                            jot_ScannerAddTextChar(self, '.');
                            jot_ScannerAddTextChar(self, c);
                            break;
                        case '.':
                            /* '..' */
                            self->state = JOT_STATE_DOT_DOT;
                            break;
                        default:
                            /* '.' something else */
                            self->state = JOT_STATE_START;
                            return JOT_TOKEN_DOT;
                    }
                    break;
                case JOT_STATE_DOT_DOT:
                    if(c == '.')
                    {
                        /* '...' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_DOT_DOT_DOT;
                    }
                    else
                    {
                        /* '..' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_DOT_DOT;
                    }
                    break;
                case JOT_STATE_LT:
                    if(c == '=')
                    {
                        /* '<=' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_CMP_LE;
                    }
                    else if(c == '<')
                    {
                        /* '<<' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_SHL;
                    }
                    else
                    {
                        /* '<' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_CMP_LT;
                    }
                    break;
                case JOT_STATE_GT:
                    if(c == '=')
                    {
                        /* '>=' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_CMP_GE;
                    }
                    else if(c == '>')
                    {
                        /* '>>' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_SHR;
                    }
                    else
                    {
                        /* '>' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_CMP_GT;
                    }
                    break;
                case JOT_STATE_EQ:
                    if(c == '=')
                    {
                        /* '==' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_CMP_EQ;
                    }
                    else
                    {
                        /* '=' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_ASSIGN;
                    }
                    break;
                case JOT_STATE_ASTERISK:
                    if(c == '*')
                    {
                        /* '**' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_EXP;
                    }
                    else
                    {
                        /* '*' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_MUL;
                    }
                    break;
                case JOT_STATE_DASH:
                    if(c == '>')
                    {
                        /* '->' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_ARROW;
                    }
                    else
                    {
                        /* '-' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_SUB;
                    }
                    break;
                case JOT_STATE_EXCLAIM:
                    if(c == '=')
                    {
                        /* '!=' */
                        self->state = JOT_STATE_START;
                        self->position++;
                        return JOT_TOKEN_CMP_NE;
                    }
                    else
                    {
                        /* '!' something else */
                        self->state = JOT_STATE_START;
                        return JOT_TOKEN_EXCLAIM;
                    }
                    break;
                default:
                    break;
            }
            
            if(self->state == JOT_STATE_NEWLINE)
            {
                self->state = self->previous_state;
                if(self->state == JOT_STATE_HASH
                    || self->state == JOT_STATE_HASH_COMMENT)
                {
                    self->state = JOT_STATE_START;
                }
                /* Repeat last character if it wasn't a newline. */
                if(c != '\n')
                {
                    continue;
                }
            }
            else
            {
                if(c == '\n')
                {
                    self->line++;
                    if(self->state == JOT_STATE_HASH
                        || self->state == JOT_STATE_HASH_COMMENT)
                    {
                        self->state = JOT_STATE_START;
                    }
                }
                else if(c == '\r')
                {
                    self->line++;
                    self->previous_state = self->state;
                    self->state = JOT_STATE_NEWLINE;
                }
            }
            
            /*
                If we didn't return/continue yet, increment position.
                Returning has the effect of letting delimiter characters be read twice
                (once to know a token ended, once to start the next token).
            */
            self->position++;
        }
        
        self->position = 0;
        self->buffer = self->source->reader(self->source, &self->buffer_size);
        if(self->buffer_size == 0)
        {   
            /*
                If we've already seen EOF in the lexer then now we can return it as token.
                This allows us to handle state transitions.
            */
            if(self->end_of_file)
            {
                return JOT_TOKEN_EOF;
            }
            else
            {
                /*
                    Set the EOF flag to be ready for next pass.
                    Set the buffer to size 1 so it will read the end-of-stream marker.
                */
                self->end_of_file = 1;
                self->buffer_size = 1;
            }
        }
    } while(1);
    
    return JOT_TOKEN_EOF;
}

int main(int argc, char** argv)
{
    jot_Token token;
    jot_Source* source;
    jot_Scanner* scanner;

    source = jot_FileSourceNew("hello.txt");
    scanner = jot_ScannerNew(source);
    printf("eof %d\n", JOT_END_OF_STREAM);
    while(token = jot_ScannerNext(scanner), token != JOT_TOKEN_EOF)
    {
        printf("%d Token: %d %s", scanner->line, token, token_name[token]);
        /* printf(" [%d %d]", scanner->text_capacity, scanner->last_text_capacity); */
        switch(token)
        {
            case JOT_TOKEN_INT:
            case JOT_TOKEN_HEX:
            case JOT_TOKEN_BIN:
            case JOT_TOKEN_NUM:
            case JOT_TOKEN_STR:
            case JOT_TOKEN_IDENTIFIER:
                printf(" -- '");
                fwrite(scanner->last_text, 1, scanner->last_text_length, stdout);
                printf("'");
            default:
                printf("\n");
        }
    }
    jot_ScannerFree(scanner);
    jot_FileSourceFree(source);
    
    return 0;
}
