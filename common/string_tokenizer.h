#ifndef LAGER_COMMON_STRING_TOKENIZER_H
#define LAGER_COMMON_STRING_TOKENIZER_H

#include <string>

/**
 * Parses a string and breaks it down into tokens separated by delimiters. The
 * tokens are added to a vector of strings.
 *
 * Based on code from
 * http://oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
 */
void TokenizeString(const std::string& str, std::vector<std::string>& tokens,
                    const std::string& delimiters) {
  // Skip delimiters at beginning.
  std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos = str.find_first_of(delimiters, last_pos);

  while (std::string::npos != pos || std::string::npos != last_pos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(last_pos, pos - last_pos));
    // Skip delimiters.  Note the "not_of"
    last_pos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, last_pos);
  }
}

#endif /* LAGER_COMMON_STRING_TOKENIZER_H */
