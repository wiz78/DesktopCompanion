//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "sentenceEngine.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <random>

#if __has_include( <LittleFS.h> )
#include <LittleFS.h>
#endif

SentenceEngine::SentenceEngine()
{
    rebuildDeck();
}

bool SentenceEngine::begin()
{
#if __has_include( <LittleFS.h> )
    LittleFS.begin( true );
#endif
    return reload();
}

void SentenceEngine::initMockEnvironment( const std::string& mockFilePath )
{
    activeFilePath = mockFilePath;
    reload();
}

bool SentenceEngine::reload()
{
    isCustomActive = scanFileAndBuildIndex( activeFilePath, customIndex );
    rebuildDeck();
    return isCustomActive;
}

void SentenceEngine::clearCustomSentences()
{
#if __has_include( <LittleFS.h> )
    if( LittleFS.exists( activeFilePath.c_str() ) ) {
        LittleFS.remove( activeFilePath.c_str() );
    }
#else
    std::remove( activeFilePath.c_str() );
#endif
    customIndex.clear();
    isCustomActive = false;
    rebuildDeck();
}

bool SentenceEngine::hasCustomSentences() const
{
    return isCustomActive && !customIndex.empty();
}

size_t SentenceEngine::getSentenceCount() const
{
    if( hasCustomSentences() ) {
        return customIndex.size();
    }

    return I18n::instance().getBuiltinAphorismCount();
}

bool SentenceEngine::validateQuota( const size_t bytes ) const
{
    return bytes <= MAX_FILE_SIZE_BYTES;
}

bool SentenceEngine::getNextSentence( std::string& outSentence )
{
    const size_t totalCount = getSentenceCount();
    if( totalCount == 0 ) {
        outSentence.clear();
        return false;
    }

    if( deck.size() != totalCount ) {
        rebuildDeck();
    }

    if( deckIndex >= deck.size() ) {
        shuffleDeck();
        deckIndex = 0;
    }

    const uint16_t selectedIndex = deck[ deckIndex++ ];

    if( hasCustomSentences() ) {
        if( selectedIndex < customIndex.size() && readCustomSentence( customIndex[ selectedIndex ], outSentence ) ) {
            return true;
        }

        const size_t builtinCount = I18n::instance().getBuiltinAphorismCount();
        if( builtinCount > 0 ) {
            outSentence = I18n::instance().getBuiltinAphorism( selectedIndex % builtinCount );
            return !outSentence.empty();
        }

        return false;
    }

    outSentence = I18n::instance().getBuiltinAphorism( selectedIndex );
    return !outSentence.empty();
}

bool SentenceEngine::saveCustomJsonAtomic( const char *jsonContent, const size_t length )
{
    if( jsonContent == nullptr || length == 0 || !validateQuota( length ) ) {
        return false;
    }

    const std::string tmpPath = ( activeFilePath == SENTENCES_FILE_PATH )
                                    ? std::string( SENTENCES_TMP_PATH )
                                    : ( activeFilePath + ".tmp" );

#if __has_include( <LittleFS.h> )
    File tmpFile = LittleFS.open( tmpPath.c_str(), "w" ); if( !tmpFile ) {
        return false;
    } const size_t written = tmpFile.write( reinterpret_cast<const uint8_t *>(jsonContent), length ); tmpFile.close();
    if( written != length ) {
        LittleFS.remove( tmpPath.c_str() );
        return false;
    }
#else
    {
        std::ofstream tmpFile( tmpPath, std::ios::binary | std::ios::trunc );
        if( !tmpFile.is_open() ) {
            return false;
        }
        tmpFile.write( jsonContent, static_cast<std::streamsize>(length) );
        if( !tmpFile.good() ) {
            tmpFile.close();
            std::remove( tmpPath.c_str() );
            return false;
        }
    }
#endif

    std::vector<SentenceRef> tempIndex;
    const bool valid = scanFileAndBuildIndex( tmpPath, tempIndex );
    if( !valid || tempIndex.empty() ) {
#if __has_include( <LittleFS.h> )
        LittleFS.remove( tmpPath.c_str() );
#else
        std::remove( tmpPath.c_str() );
#endif
        return false;
    }

#if __has_include( <LittleFS.h> )
    if( LittleFS.exists( activeFilePath.c_str() ) ) {
        LittleFS.remove( activeFilePath.c_str() );
    } if( !LittleFS.rename( tmpPath.c_str(), activeFilePath.c_str() ) ) {
        LittleFS.remove( tmpPath.c_str() );
        return false;
    }
#else
    std::remove( activeFilePath.c_str() );
    if( std::rename( tmpPath.c_str(), activeFilePath.c_str() ) != 0 ) {
        std::remove( tmpPath.c_str() );
        return false;
    }
#endif

    reload();
    return true;
}

bool SentenceEngine::scanFileAndBuildIndex( const std::string& path, std::vector<SentenceRef>& outIndex )
{
    outIndex.clear();

#if __has_include( <LittleFS.h> )
    File file = LittleFS.open( path.c_str(), "r" ); if( !file || file.isDirectory() ) {
        return false;
    } const size_t fileSize = file.size();
#else
    std::ifstream file( path, std::ios::binary | std::ios::ate );
    if( !file.is_open() ) {
        return false;
    }
    const std::streamsize sizeResult = file.tellg();
    if( sizeResult < 0 ) {
        return false;
    }
    const size_t fileSize = static_cast<size_t>(sizeResult);
    file.seekg( 0, std::ios::beg );
#endif

    if( fileSize == 0 || fileSize > MAX_FILE_SIZE_BYTES ) {
#if __has_include( <LittleFS.h> )
        file.close();
#endif
        return false;
    }

    enum class State
    {
        SearchArrayStart,
        InsideArray,
        InsideString,
        ArrayEnd
    };

    auto state = State::SearchArrayStart;
    bool isEscaped = false;
    uint32_t stringStartOffset = 0;
    uint32_t currentOffset = 0;

    while( true ) {
        char buffer[ 512 ];
        size_t bytesRead = 0;
#if __has_include( <LittleFS.h> )
        bytesRead = file.read( reinterpret_cast<uint8_t *>(buffer), sizeof( buffer ) );
#else
        file.read( buffer, sizeof( buffer ) );
        bytesRead = static_cast<size_t>(file.gcount());
#endif

        if( bytesRead == 0 ) {
            break;
        }

        for( size_t i = 0; i < bytesRead; ++i ) {
            const char c = buffer[ i ];
            const uint32_t byteOffset = currentOffset + i;

            if( state == State::SearchArrayStart ) {
                if( std::isspace( static_cast<unsigned char>(c) ) ) {
                    continue;
                }
                if( c == '[' ) {
                    state = State::InsideArray;
                } else {
#if __has_include( <LittleFS.h> )
                    file.close();
#endif
                    return false;
                }
            } else if( state == State::InsideArray ) {
                if( std::isspace( static_cast<unsigned char>(c) ) || c == ',' ) {
                    continue;
                }
                if( c == '"' ) {
                    state = State::InsideString;
                    isEscaped = false;
                    stringStartOffset = byteOffset + 1;
                } else if( c == ']' ) {
                    state = State::ArrayEnd;
                    break;
                }
            } else if( state == State::InsideString ) {
                if( isEscaped ) {
                    isEscaped = false;
                } else if( c == '\\' ) {
                    isEscaped = true;
                } else if( c == '"' ) {
                    const uint32_t stringEndOffset = byteOffset;
                    if( stringEndOffset >= stringStartOffset ) {
                        const uint16_t length = static_cast<uint16_t>(stringEndOffset - stringStartOffset);
                        outIndex.push_back( SentenceRef{ stringStartOffset, length } );
                    }
                    state = State::InsideArray;
                }
            }
        }

        currentOffset += static_cast<uint32_t>(bytesRead);
        if( state == State::ArrayEnd ) {
            break;
        }
    }

#if __has_include( <LittleFS.h> )
    file.close();
#endif

    if( state == State::InsideString || outIndex.empty() ) {
        outIndex.clear();
        return false;
    }

    return true;
}

void SentenceEngine::rebuildDeck()
{
    deck.clear();
    const size_t count = getSentenceCount();
    if( count == 0 ) {
        deckIndex = 0;
        return;
    }

    deck.resize( count );
    for( size_t i = 0; i < count; ++i ) {
        deck[ i ] = static_cast<uint16_t>(i);
    }

    shuffleDeck();
    deckIndex = 0;
}

void SentenceEngine::shuffleDeck()
{
    if( deck.size() <= 1 ) {
        return;
    }

#if defined(ESP32)
    for( size_t i = deck.size() - 1; i > 0; --i ) {
        const auto j = esp_random() % ( i + 1 );
        std::swap( deck[ i ], deck[ j ] );
    }
#else
    static std::random_device rd;
    static std::mt19937 gen( rd() );
    for( size_t i = deck.size() - 1; i > 0; --i ) {
        std::uniform_int_distribution<size_t> dist( 0, i );
        const size_t j = dist( gen );
        std::swap( deck[ i ], deck[ j ] );
    }
#endif
}

bool SentenceEngine::readCustomSentence( const SentenceRef& ref, std::string& out )
{
#if __has_include( <LittleFS.h> )
    File file = LittleFS.open( activeFilePath.c_str(), "r" ); if( !file ) {
        return false;
    } if( !file.seek( ref.offset ) ) {
        file.close();
        return false;
    } std::string raw; raw.resize( ref.length ); const size_t bytesRead = file.read(
        reinterpret_cast<uint8_t *>(raw.data()), ref.length ); file.close(); if( bytesRead != ref.length ) {
        return false;
    }
#else
    std::ifstream file( activeFilePath, std::ios::binary );
    if( !file.is_open() ) {
        return false;
    }
    file.seekg( ref.offset, std::ios::beg );
    if( !file.good() ) {
        return false;
    }
    std::string raw;
    raw.resize( ref.length );
    file.read( raw.data(), ref.length );
    if( static_cast<size_t>(file.gcount()) != ref.length ) {
        return false;
    }
#endif

    out = unescapeJsonString( raw );
    return true;
}

std::string SentenceEngine::unescapeJsonString( const std::string& input )
{
    std::string result;
    result.reserve( input.size() );

    for( size_t i = 0; i < input.size(); ++i ) {
        if( input[ i ] == '\\' && ( i + 1 ) < input.size() ) {
            switch( const char next = input[ i + 1 ] ) {
                case '"':
                    result.push_back( '"' );
                    ++i;
                    break;
                case '\\':
                    result.push_back( '\\' );
                    ++i;
                    break;
                case '/':
                    result.push_back( '/' );
                    ++i;
                    break;
                case 'b':
                    result.push_back( '\b' );
                    ++i;
                    break;
                case 'f':
                    result.push_back( '\f' );
                    ++i;
                    break;
                case 'n':
                    result.push_back( '\n' );
                    ++i;
                    break;
                case 'r':
                    result.push_back( '\r' );
                    ++i;
                    break;
                case 't':
                    result.push_back( '\t' );
                    ++i;
                    break;
                case 'u':
                    if( ( i + 5 ) < input.size() ) {
                        uint32_t val = 0;
                        bool validHex = true;
                        for( size_t h = 0; h < 4; ++h ) {
                            const char hc = input[ i + 2 + h ];
                            val <<= 4U;
                            if( hc >= '0' && hc <= '9' ) {
                                val |= static_cast<uint32_t>(hc - '0');
                            } else if( hc >= 'a' && hc <= 'f' ) {
                                val |= static_cast<uint32_t>(hc - 'a' + 10);
                            } else if( hc >= 'A' && hc <= 'F' ) {
                                val |= static_cast<uint32_t>(hc - 'A' + 10);
                            } else {
                                validHex = false;
                                break;
                            }
                        }

                        if( validHex ) {
                            if( val <= 0x7F ) {
                                result.push_back( static_cast<char>(val) );
                            } else if( val <= 0x7FF ) {
                                result.push_back( static_cast<char>(0xC0 | ( ( val >> 6 ) & 0x1F )) );
                                result.push_back( static_cast<char>(0x80 | ( val & 0x3F )) );
                            } else {
                                result.push_back( static_cast<char>(0xE0 | ( ( val >> 12 ) & 0x0F )) );
                                result.push_back( static_cast<char>(0x80 | ( ( val >> 6 ) & 0x3F )) );
                                result.push_back( static_cast<char>(0x80 | ( val & 0x3F )) );
                            }
                            i += 5;
                            break;
                        }
                    }
                    result.push_back( next );
                    ++i;
                    break;
                default:
                    result.push_back( next );
                    ++i;
                    break;
            }
        } else {
            result.push_back( input[ i ] );
        }
    }

    return result;
}
