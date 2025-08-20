#ifdef USE_CUSTOM_STRING
#include "string_utils.h"

#ifndef __LCEX__
#include <stdlib.h>
#include <stdio.h>
#else
#include "lcruntime_wrapper.h"
#endif

sstring sfromcstr(const char* cstr)
{
    sstring newstring = (sstring)malloc(sizeof(struct sstring_s));
    if (newstring == NULL)
        return NULL;

    int cstrlen = strlen(cstr) + 1;
    newstring->cstr = (char*)malloc(cstrlen);
    if (newstring->cstr == NULL)
    {
        free(newstring);
        return NULL;
    }

    newstring->length = cstrlen;
    if (cstrlen <= 1)
        *newstring->cstr = '\0';
    else
        strcpy(newstring->cstr, cstr);
    newstring->cstr[cstrlen - 1] = '\0';
    return newstring;
}

void sassigncstr(sstring string, const char* cstr)
{
    int cstrlen = strlen(cstr) + 1;
    if (string->cstr == NULL)
        string->cstr = (char*)malloc(sizeof(char)*(cstrlen));
    else if (cstrlen != string->length)
    {
        free(string->cstr);
        string->cstr = (char*)malloc(sizeof(char)*(cstrlen));
    }   

    if (string->cstr == NULL)
        return;

    string->length = cstrlen;
    if (cstrlen <= 1)
        *string->cstr = '\0';
    else
        strcpy(string->cstr, cstr);
    string->cstr[cstrlen - 1] = '\0';
    return;
}

void sassignformat(sstring string, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Calculate string size
    int size = vsnprintf(NULL, 0, format, args) + 1;

    if (string->cstr != NULL && size != string->length)
    {
        free(string->cstr);
        string->cstr = (char*)malloc(sizeof(char)*(size));
    }
    else if (string->cstr == NULL)
        string->cstr = (char*)malloc(sizeof(char)*(size));

    va_end(args);
    if (string->cstr == NULL)
        return;

    // Reset args list
    va_start(args, format);

    // Write formated string to buffer
    vsnprintf(string->cstr, size, format, args);
    string->length = size;

    // End args list
    va_end(args);
    return;
}

sstring sformat(const char* format, ...)
{
    sstring newstring = (sstring)malloc(sizeof(struct sstring_s));
    if (newstring == NULL)
        return NULL;

    va_list args;
    va_start(args, format);

    // Calculate string size
    int size = vsnprintf(NULL, 0, format, args) + 1;

    newstring->cstr = (char*)malloc(sizeof(char)*(size));

    va_end(args);
    if (newstring->cstr == NULL)
    {
        free(newstring);
        return NULL;
    }

    // Reset args list
    va_start(args, format);

    // Write formated string to buffer
    vsnprintf(newstring->cstr, size, format, args);
    newstring->length = size;

    // End args list
    va_end(args);
    return newstring;
}

void scatcstr(sstring string, const char* cstr)
{
    if (string->cstr != NULL)
    {
        int size = string->length + strlen(cstr); // string->lenght already counts '\0' character

        string->cstr = (char*)realloc(string->cstr, sizeof(char) * size);
        if (string->cstr != NULL)
        {
            strcat(string->cstr, cstr);
            string->length = size;
            string->cstr[size - 1] = '\0';
        }
    }
}

void sdestroy(sstring string)
{
    if (string->cstr != NULL)
    {
        free(string->cstr);
        string->cstr = NULL;
    }
    free(string);
    return;
}
#endif