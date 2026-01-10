#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string>
#include <optional>

struct ParsedArgs
{
    std::string dataRoot;
    bool helpRequested{false};
};

std::optional<ParsedArgs> parseArguments(int argc, const char **argv);

#endif // ARG_PARSER_H
