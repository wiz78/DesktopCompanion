//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "textFormatter.h"

#include <cctype>
#include <string>
#include <vector>

namespace TextFormatter
{
    namespace
    {
        [[nodiscard]] std::vector<std::string> tokenize( const std::string& text )
        {
            std::vector<std::string> tokens;
            std::string current;

            for( const char ch : text ) {
                if( std::isspace( static_cast<unsigned char>(ch) ) ) {
                    if( !current.empty() ) {
                        tokens.push_back( current );
                        current.clear();
                    }
                } else {
                    current += ch;
                }
            }

            if( !current.empty() ) {
                tokens.push_back( current );
            }

            return tokens;
        }
    } // namespace

    std::vector<std::string> wrapText( const std::string& text, const size_t maxCharsPerLine, const size_t maxLines )
    {
        (void)maxLines; // maxLines parameter allows custom interfaces, but wrapping wraps all content

        if( maxCharsPerLine == 0 ) {
            return {};
        }

        const auto tokens = tokenize( text );
        if( tokens.empty() ) {
            return {};
        }

        std::vector<std::string> lines;
        std::string currentLine;

        for( const auto& token : tokens ) {
            if( std::string word = token; word.length() <= maxCharsPerLine ) {
                if( currentLine.empty() ) {
                    currentLine = word;
                } else if( currentLine.length() + 1 + word.length() <= maxCharsPerLine ) {
                    currentLine += ' ';
                    currentLine += word;
                } else {
                    lines.push_back( currentLine );
                    currentLine = word;
                }
            } else {
                // Word is longer than maxCharsPerLine -> split/hyphenate
                if( !currentLine.empty() ) {
                    lines.push_back( currentLine );
                    currentLine.clear();
                }

                while( word.length() > maxCharsPerLine ) {
                    if( maxCharsPerLine >= 2 ) {
                        const size_t take = maxCharsPerLine - 1;
                        lines.push_back( word.substr( 0, take ) + "-" );
                        word = word.substr( take );
                    } else {
                        lines.push_back( word.substr( 0, 1 ) );
                        word = word.substr( 1 );
                    }
                }

                if( !word.empty() ) {
                    currentLine = word;
                }
            }
        }

        if( !currentLine.empty() ) {
            lines.push_back( currentLine );
        }

        return lines;
    }

    bool canFitInLines( const std::string& text, const size_t maxCharsPerLine, const size_t maxLines,
                        const size_t maxTotalChars )
    {
        if( text.length() > maxTotalChars ) {
            return false;
        }

        const auto lines = wrapText( text, maxCharsPerLine, maxLines );
        return lines.size() <= maxLines;
    }
} // namespace TextFormatter
