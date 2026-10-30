#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

inline bool extractCommandId(
    const char *line,
    size_t length,
    size_t &commandLength,
    bool &hasId,
    uint32_t &id)
{
    commandLength = length;
    hasId = false;
    id = 0;

    size_t separator = length;
    for (size_t i = 0; i < length; ++i)
    {
        if (line[i] == ';')
        {
            if (separator != length)
                return false;
            separator = i;
        }
    }

    if (separator == length)
        return true;

    if (length - separator < 5 ||
        line[separator + 1] != 'I' ||
        line[separator + 2] != 'D' ||
        line[separator + 3] != '=' ||
        separator + 4 == length)
        return false;

    uint32_t parsedId = 0;
    for (size_t i = separator + 4; i < length; ++i)
    {
        const char digit = line[i];
        if (digit < '0' || digit > '9')
            return false;

        const uint32_t value = static_cast<uint32_t>(digit - '0');
        if (parsedId > (UINT32_MAX - value) / 10)
            return false;
        parsedId = parsedId * 10 + value;
    }

    commandLength = separator;
    hasId = true;
    id = parsedId;
    return true;
}

inline bool isStatusQuery(const char *command, size_t length)
{
    static const char query[] = "STATUS?";
    if (length != sizeof(query) - 1)
        return false;
    for (size_t i = 0; i < length; ++i)
    {
        if (command[i] != query[i])
            return false;
    }
    return true;
}

#endif
